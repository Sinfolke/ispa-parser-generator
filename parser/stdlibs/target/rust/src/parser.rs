//! `LLParser<Main, Token>`: the base of generated parsers. Combinators, error recovery and frames run in the lowered
//! core (`python::ispa_std::parser_ll`); typed iterators, tokens and nodes stay typed (see bridge.rs).
use crate::python::ispa_std::{lexer::TokenCursor, match_result::MatchResult as CoreMatch, node::Node as CoreNode, parser_ll,
                              recovery as core_recovery};
use crate::{core_node, typed_node, BridgeCursor, BridgeToken, LazyIterator, LexerBase, MatchResult, Node, Ptr, Shared, TypedCursor};
use crate::{Error, Upcast};
use std::collections::HashMap;
use std::ops::DerefMut;

pub mod recovery {
    use super::*;
    pub const END_OF_INPUT: usize = usize::MAX;

    /// A failure handler of the core: what to do when a required element is missing.
    #[derive(Clone)]
    pub struct CoreFail { pub handler: Shared<core_recovery::FailHandler> }

    pub(super) fn token_set(ids: &[usize]) -> Shared<core_recovery::TokenSet> {
        let set = core_recovery::TokenSet::shared_default();
        set.borrow_mut().__init__(&ids.iter().map(|id| *id as i64).collect());
        set
    }

    /// The core handler for one call site (cached: the sets are constants of the generated parser).
    pub fn handler(expected: &[usize], continuation: &[usize], follow: &[usize], synchronization: &[usize], insertion: bool,
                   deletion: bool, replacement: bool, max_skip: i64, rule: impl AsRef<str>, expected_text: impl AsRef<str>) -> CoreFail {
        thread_local! { static CACHE: std::cell::RefCell<HashMap<String, CoreFail>> = Default::default(); }
        let key = format!("{:?}|{:?}|{:?}|{:?}|{insertion}{deletion}{replacement}{max_skip}|{}|{}", expected, continuation, follow,
                          synchronization, rule.as_ref(), expected_text.as_ref());
        if let Some(found) = CACHE.with(|cache| cache.borrow().get(&key).cloned()) { return found; }
        let policy = core_recovery::Policy::shared_default();
        policy.borrow_mut().__init__(insertion, deletion, replacement, max_skip);
        let handler = core_recovery::Handler::shared_default();
        handler.borrow_mut().__init__(&rule.as_ref().to_string(), &expected_text.as_ref().to_string(), token_set(expected),
                                      token_set(continuation), token_set(follow), token_set(synchronization), policy);
        let made = CoreFail { handler: handler.upcast() };
        CACHE.with(|cache| cache.borrow_mut().insert(key, made.clone()));
        made
    }
}
pub use recovery::CoreFail;

/// What stands for the failure behaviour of a required element.
pub trait IntoFail { fn into_handler(self) -> Shared<core_recovery::FailHandler>; }
impl IntoFail for CoreFail { fn into_handler(self) -> Shared<core_recovery::FailHandler> { self.handler } }

/// Closes the rule's recovery frame when it goes out of scope.
pub struct FrameGuard { core: Shared<parser_ll::LLParser> }
impl Drop for FrameGuard { fn drop(&mut self) { self.core.borrow_mut().leave_frame(); } }

/// Base of generated parsers.
#[repr(C)]
pub struct LLParser<Main, Token> {
    pub lexer: Shared<LexerBase<Token>>,
    pub text: Ptr,
    pub tree: Main,
    pub errors: Vec<Error>,
    core: Option<Shared<parser_ll::LLParser>>,
    pub __vt_parseFromTokens: fn(&mut LLParser<Main, Token>),
    pub __vt_lazyParse: fn(&mut LLParser<Main, Token>) -> MatchResult<Main, LazyIterator<Token>>,
}
impl<Main: Clone, Token> Clone for LLParser<Main, Token> {
    fn clone(&self) -> Self {
        Self { lexer: self.lexer.clone(), text: self.text, tree: self.tree.clone(), errors: self.errors.clone(), core: self.core.clone(),
               __vt_parseFromTokens: self.__vt_parseFromTokens, __vt_lazyParse: self.__vt_lazyParse }
    }
}
impl<Main: Default, Token: Clone + Default + 'static> Default for LLParser<Main, Token> {
    fn default() -> Self {
        Self { lexer: Shared::default(), text: Ptr::new(""), tree: Main::default(), errors: Vec::new(), core: None,
               __vt_parseFromTokens: |_| {}, __vt_lazyParse: |_| MatchResult { status: false, node: Node::default(), it: LazyIterator::default() } }
    }
}
impl<Main, Token> LLParser<Main, Token> {
    /// The core parser holding the recovery state of the current parse.
    pub fn core(&mut self) -> Shared<parser_ll::LLParser> {
        if self.core.is_none() {
            let core = parser_ll::LLParser::shared_default();
            core.borrow_mut().__init__();
            self.core = Some(core);
        }
        self.core.clone().unwrap()
    }
    pub fn parseFromTokens(&mut self) where Main: 'static, Token: 'static { (self.__vt_parseFromTokens)(self) }
    pub fn lazyParse(&mut self) -> MatchResult<Main, LazyIterator<Token>> { (self.__vt_lazyParse)(self) }
    /// Parse `text`, lexing it on demand; the tree is `self.tree`. The text is kept for the life of the program.
    pub fn parse_text(&mut self, text: &str) -> &Main where Main: Clone {
        self.text = Ptr::new(Box::leak(text.to_string().into_boxed_str()));
        let result = self.lazyParse();
        self.tree = result.node.data().clone();
        &self.tree
    }

    pub fn unwinding(&mut self) -> bool { self.core().borrow_mut().unwinding() }
    pub fn recovery_frame(&mut self, rule: impl AsRef<str>) -> FrameGuard {
        let core = self.core();
        core.borrow_mut().enter_frame(&rule.as_ref().to_string());
        FrameGuard { core }
    }
    /// Where recovery may resume inside this rule: one claim (a label and a set of token ids) per set.
    pub fn recovery_at(&mut self, sets: &[&[usize]], expecting: impl AsRef<str>, labels: &[String]) {
        let claims: Vec<Shared<core_recovery::ContextClaim>> = sets.iter().zip(labels.iter().chain(std::iter::repeat(&String::new()))).map(|(set, label)| {
            let claim = core_recovery::ContextClaim::shared_default();
            claim.borrow_mut().__init__(label, recovery::token_set(set));
            claim
        }).collect();
        self.core().borrow_mut().recovery_at(&expecting.as_ref().to_string(), &claims);
    }
    /// Which alternative to continue in after a failed choice; the cursor ends where recovery left it.
    pub fn recover_choice<IT: BridgeCursor>(&mut self, sets: &[&[usize]], pos: &mut IT, rule: impl AsRef<str>, expected: impl AsRef<str>) -> i64 {
        let (mut first, mut inner) = (Vec::new(), Vec::new());
        for (index, set) in sets.iter().enumerate() {
            (if index % 2 == 0 { &mut first } else { &mut inner }).push(recovery::token_set(set));
        }
        let cursor = TypedCursor::handle(pos.clone(), None);
        let chosen = self.core().borrow_mut().recover_choice(cursor.clone().upcast(), &rule.as_ref().to_string(),
                                                            &expected.as_ref().to_string(), &first, &inner);
        *pos = cursor.borrow().it.clone();
        chosen
    }
    /// A matched result made from an action's value (the rule short-circuits on an unwinding recovery).
    pub fn action_result<R, IT>(&mut self, value: R, pos: IT) -> MatchResult<R, IT> {
        MatchResult { status: true, node: Node::create(value), it: pos }
    }
}

/// The combinators generated rules call. `Self` is the generated parser; its `LLParser` base holds the core.
pub trait LLParserApi<IT: BridgeCursor> {
    fn required<T: Clone + Default + 'static>(&mut self, pos: &mut IT, fail: impl IntoFail) -> Node<T>;
    fn required_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                   fail: impl IntoFail) -> Node<T>;
    fn optional<T: Clone + Default + 'static>(&mut self, pos: &mut IT) -> Node<T>;
    fn optional_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Node<T>;
    fn range<T: Clone + Default + 'static>(&mut self, pos: &mut IT, fail: impl IntoFail) -> Vec<Node<T>>;
    fn range_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                fail: impl IntoFail) -> Vec<Node<T>>;
    fn range_optional<T: Clone + Default + 'static>(&mut self, pos: &mut IT) -> Vec<Node<T>>;
    fn range_optional_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>)
        -> Vec<Node<T>>;
}

/// The same combinators for repetitions that skip a space token between their elements (`range::<T, Space>`).
pub trait LLParserSpaced<IT: BridgeCursor>: LLParserApi<IT> {
    fn range_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, fail: impl IntoFail) -> Vec<Node<T>>;
    fn range_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                                   fail: impl IntoFail) -> Vec<Node<T>>;
    fn range_optional_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT) -> Vec<Node<T>>;
    fn range_optional_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT,
                                                                           rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Vec<Node<T>>;
}

fn bridge<IT: BridgeCursor, T: Clone + Default + 'static>(it: IT, token: bool) -> Shared<TypedCursor<IT>> {
    TypedCursor::handle(it, if token { Some(crate::node_maker::<IT::Token, T>()) } else { None })
}
fn token_id<IT: BridgeCursor, T: 'static>() -> i64 { <IT::Token as BridgeToken>::alt_of::<T>().unwrap_or(-1) }
fn take<IT: BridgeCursor>(cursor: &Shared<TypedCursor<IT>>, pos: &mut IT) { *pos = cursor.borrow().it.clone(); }
fn typed_nodes<T: Clone + Default + 'static>(nodes: Vec<Shared<CoreNode>>) -> Vec<Node<T>> { nodes.iter().map(typed_node::<T>).collect() }

/// A typed rule as the core's callback: the typed match travels back inside a `Node`.
fn rule_callback<'a, P: 'a, IT: BridgeCursor, T: Clone + Default + 'static>(
    parser: *mut P, mut rec: impl FnMut(&mut P, IT) -> MatchResult<T, IT> + 'a,
) -> impl FnMut(Shared<TokenCursor>) -> Shared<CoreMatch> + 'a {
    move |base: Shared<TokenCursor>| {
        let it = {
            let mut base = base.borrow_mut();
            unsafe { &*(&mut *base as *mut TokenCursor as *mut TypedCursor<IT>) }.it.clone()
        };
        let matched = rec(unsafe { &mut *parser }, it);
        let result = CoreMatch::shared_default();
        let out: Shared<TokenCursor> = bridge::<IT, T>(matched.it, false).upcast();
        let node = if matched.status { core_node(matched.node) } else { CoreNode::blank() };
        result.borrow_mut().__init__(matched.status, node, out);
        result
    }
}

impl<P, IT, Main, Token> LLParserApi<IT> for P
where
    P: DerefMut<Target = LLParser<Main, Token>>,
    IT: BridgeCursor,
{
    fn required<T: Clone + Default + 'static>(&mut self, pos: &mut IT, fail: impl IntoFail) -> Node<T> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), true);
        let node = core.borrow_mut().required_token(cursor.clone().upcast(), token_id::<IT, T>(), fail.into_handler());
        take(&cursor, pos);
        typed_node::<T>(&node)
    }
    fn required_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                   fail: impl IntoFail) -> Node<T> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), false);
        let callback = rule_callback::<Self, IT, T>(self as *mut Self, rec);
        let node = core.borrow_mut().required_rule(cursor.clone().upcast(), &String::new(), callback, fail.into_handler());
        take(&cursor, pos);
        typed_node::<T>(&node)
    }
    fn optional<T: Clone + Default + 'static>(&mut self, pos: &mut IT) -> Node<T> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), true);
        let node = core.borrow_mut().optional_token(cursor.clone().upcast(), token_id::<IT, T>());
        take(&cursor, pos);
        typed_node::<T>(&node)
    }
    fn optional_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Node<T> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), false);
        let callback = rule_callback::<Self, IT, T>(self as *mut Self, rec);
        let node = core.borrow_mut().optional_rule(cursor.clone().upcast(), callback);
        take(&cursor, pos);
        typed_node::<T>(&node)
    }
    fn range<T: Clone + Default + 'static>(&mut self, pos: &mut IT, fail: impl IntoFail) -> Vec<Node<T>> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), true);
        let nodes = core.borrow_mut().range_token(cursor.clone().upcast(), token_id::<IT, T>(), fail.into_handler());
        take(&cursor, pos);
        typed_nodes::<T>(nodes)
    }
    fn range_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                fail: impl IntoFail) -> Vec<Node<T>> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), false);
        let callback = rule_callback::<Self, IT, T>(self as *mut Self, rec);
        let nodes = core.borrow_mut().range_rule(cursor.clone().upcast(), &String::new(), callback, fail.into_handler());
        take(&cursor, pos);
        typed_nodes::<T>(nodes)
    }
    fn range_optional<T: Clone + Default + 'static>(&mut self, pos: &mut IT) -> Vec<Node<T>> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), true);
        let nodes = core.borrow_mut().range_optional_token(cursor.clone().upcast(), token_id::<IT, T>());
        take(&cursor, pos);
        typed_nodes::<T>(nodes)
    }
    fn range_optional_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>)
        -> Vec<Node<T>> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), false);
        let callback = rule_callback::<Self, IT, T>(self as *mut Self, rec);
        let nodes = core.borrow_mut().range_optional_rule(cursor.clone().upcast(), callback);
        take(&cursor, pos);
        typed_nodes::<T>(nodes)
    }
}

impl<P, IT, Main, Token> LLParserSpaced<IT> for P
where
    P: DerefMut<Target = LLParser<Main, Token>>,
    IT: BridgeCursor,
{
    fn range_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, fail: impl IntoFail) -> Vec<Node<T>> {
        self.use_space_token::<IT, S>();
        LLParserApi::range::<T>(self, pos, fail)
    }
    fn range_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                                   fail: impl IntoFail) -> Vec<Node<T>> {
        self.use_space_token::<IT, S>();
        LLParserApi::range_rule::<T>(self, pos, rec, fail)
    }
    fn range_optional_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT) -> Vec<Node<T>> {
        self.use_space_token::<IT, S>();
        LLParserApi::range_optional::<T>(self, pos)
    }
    fn range_optional_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT,
                                                                           rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Vec<Node<T>> {
        self.use_space_token::<IT, S>();
        LLParserApi::range_optional_rule::<T>(self, pos, rec)
    }
}
impl<Main, Token> LLParser<Main, Token> {
    fn use_space_token<IT: BridgeCursor, S: 'static>(&mut self) {
        if let Some(id) = <IT::Token as BridgeToken>::alt_of::<S>() { self.core().borrow_mut().set_space_token(id); }
    }
}
