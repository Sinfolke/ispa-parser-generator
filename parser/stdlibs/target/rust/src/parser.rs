//! `LLParser<Main, Token>`: the base of generated parsers. Combinators, error recovery and frames run in the lowered
//! core (`python::ispa_std::parser_ll`); typed iterators, tokens and nodes stay typed (see bridge.rs).
use crate::python::ispa_std::{lexer::TokenCursor, match_result::MatchResult as CoreMatch, node::Node as CoreNode, parser_ll,
                              recovery as core_recovery, counter as core_counter};
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
pub trait IntoFail<IT> { fn into_handler(self, core: &Shared<parser_ll::LLParser>) -> Shared<core_recovery::FailHandler>; }
impl<IT> IntoFail<IT> for CoreFail { fn into_handler(self, _core: &Shared<parser_ll::LLParser>) -> Shared<core_recovery::FailHandler> { self.handler } }

/// The state recovery is in while a fail block runs (the typed blocks do not read it).
#[derive(Default)]
pub struct RecoveryState;
/// `custom_failure(|pos, state| ...)`: a fail block written in the grammar, kept typed until a combinator gives it the cursor type.
pub struct CustomFail<Action> { pub action: Action }
impl<IT: BridgeCursor, Action> IntoFail<IT> for CustomFail<Action>
where Action: FnMut(&mut IT, &mut RecoveryState) -> Shared<core_recovery::Decision>,
{
    fn into_handler(mut self, core: &Shared<parser_ll::LLParser>) -> Shared<core_recovery::FailHandler> {
        let mut run = move |base: Shared<TokenCursor>, _state: Shared<core_recovery::State>| {
            let mut base = base.borrow_mut();
            let cursor = unsafe { &mut *(&mut *base as *mut TokenCursor as *mut TypedCursor<IT>) };
            (self.action)(&mut cursor.it, &mut RecoveryState)
        };
        // the handler lives for the one combinator call this is passed to, and the block borrows locals of the rule
        let run: Box<dyn FnMut(Shared<TokenCursor>, Shared<core_recovery::State>) -> Shared<core_recovery::Decision> + '_> = Box::new(move |c, s| run(c, s));
        let run: Box<dyn FnMut(Shared<TokenCursor>, Shared<core_recovery::State>) -> Shared<core_recovery::Decision> + 'static> =
            unsafe { std::mem::transmute(run) };
        let mut run = run;
        core.borrow_mut().custom_failure(move |c, s| run(c, s)).upcast()
    }
}

/// `ctx` of a fail block: reads see the typed cursor, repairs and diagnostics run in the core's `FailContext`.
pub struct FailContext<'a, IT: BridgeCursor> {
    pub pos: &'a mut IT,
    pub ast: (),
    pub rule: String,
    pub failed_rule: String,
    cursor: Shared<TypedCursor<IT>>,
    core: Shared<parser_ll::FailContext<()>>,
}
impl<'a, IT: BridgeCursor> FailContext<'a, IT> {
    fn push(&mut self) { self.cursor.borrow_mut().it = self.pos.clone(); }
    fn pull(&mut self) { *self.pos = self.cursor.borrow().it.clone(); }
    pub fn at(&mut self, spelling: impl AsRef<str>) -> bool { self.at_ahead(spelling, 0) }
    pub fn at_ahead(&mut self, spelling: impl AsRef<str>, lookahead: i64) -> bool {
        self.push();
        self.core.borrow_mut().at(&spelling.as_ref().to_string(), lookahead)
    }
    pub fn at_token(&mut self, name: impl AsRef<str>) -> bool { self.at_token_ahead(name, 0) }
    pub fn at_token_ahead(&mut self, name: impl AsRef<str>, lookahead: i64) -> bool {
        self.push();
        self.core.borrow_mut().at_token(&name.as_ref().to_string(), lookahead)
    }
    pub fn peek(&mut self) -> Shared<parser_ll::ActionToken> { self.push(); self.core.borrow_mut().peek(0) }
    pub fn peek_ahead(&mut self, lookahead: i64) -> Shared<parser_ll::ActionToken> { self.push(); self.core.borrow_mut().peek(lookahead) }
    pub fn take(&mut self) -> Shared<parser_ll::ActionToken> { self.push(); let token = self.core.borrow_mut().take(); self.pull(); token }
    pub fn prev(&mut self) -> Shared<parser_ll::ActionToken> { self.push(); self.core.borrow_mut().prev() }
    pub fn mark(&mut self, name: impl AsRef<str>) { self.push(); self.core.borrow_mut().mark(&name.as_ref().to_string()) }
    pub fn restore(&mut self, name: impl AsRef<str>) { self.core.borrow_mut().restore(&name.as_ref().to_string()); self.pull(); }
    pub fn commit(&mut self, name: impl AsRef<str>) { self.core.borrow_mut().commit(&name.as_ref().to_string()) }
    pub fn error(&mut self, message: impl AsRef<str>) { self.error_at(message, "", -1, -1) }
    pub fn error_at(&mut self, message: impl AsRef<str>, code: impl AsRef<str>, offset: i64, length: i64) {
        self.push();
        self.core.borrow_mut().error(&message.as_ref().to_string(), &code.as_ref().to_string(), offset, length);
        self.pull();
    }
    pub fn note(&mut self, message: impl AsRef<str>) { self.core.borrow_mut().note(&message.as_ref().to_string()) }
    pub fn sync(&mut self, boundaries: &[&str]) {
        self.push();
        self.core.borrow_mut().sync(&boundaries.iter().map(|b| b.to_string()).collect());
        self.pull();
    }
    pub fn skip(&mut self) { self.push(); self.core.borrow_mut().skip(); self.pull(); }
    pub fn insert(&mut self) { self.core.borrow_mut().insert() }
    pub fn replace(&mut self) { self.push(); self.core.borrow_mut().replace(); self.pull(); }
    pub fn retry(&mut self) { self.core.borrow_mut().retry() }
    pub fn abort(&mut self) { self.core.borrow_mut().abort() }
    fn finish(&mut self, before: &IT) -> Shared<core_recovery::Decision> {
        self.push();
        let marker: Shared<TokenCursor> = TypedCursor::handle(before.clone(), None).upcast();
        let decision = self.core.borrow_mut().finish(marker);
        self.pull();
        decision
    }
}

impl<Main, Token> LLParser<Main, Token> {
    /// `ctx` for a fail block running at `pos`; `token_name` spells the ids of tokens.
    pub fn fail_context<'a, IT: BridgeCursor, R>(&mut self, pos: &'a mut IT, _ast: R, start: IT, rule: impl AsRef<str>, failed_rule: impl AsRef<str>,
                                                  token_name: fn(usize) -> Ptr) -> FailContext<'a, IT> {
        let core = self.core();
        let names = TOKEN_NAMES.with(|cache| cache.borrow_mut().entry(token_name as usize).or_insert_with(|| spell_names(token_name)).clone());
        core.borrow_mut().set_token_names(names, -1);
        let cursor = TypedCursor::handle(pos.clone(), None);
        let start_cursor: Shared<TokenCursor> = TypedCursor::handle(start, None).upcast();
        let context = core.borrow_mut().fail_context(cursor.clone().upcast(), (), start_cursor, &rule.as_ref().to_string(),
                                                      &failed_rule.as_ref().to_string());
        FailContext { pos, ast: (), rule: rule.as_ref().to_string(), failed_rule: failed_rule.as_ref().to_string(), cursor, core: context }
    }
    /// The decision a fail block ends with.
    pub fn custom_failure_decision<IT: BridgeCursor>(&mut self, before: IT, ctx: &mut FailContext<IT>) -> Shared<core_recovery::Decision> {
        ctx.finish(&before)
    }
    /// A fail block, typed until a combinator gives it the cursor type.
    pub fn custom_failure<IT: BridgeCursor, Action>(&self, _cursor: &IT, action: Action) -> CustomFail<Action>
    where Action: FnMut(&mut IT, &mut RecoveryState) -> Shared<core_recovery::Decision> { CustomFail { action } }
}

thread_local! { static TOKEN_NAMES: std::cell::RefCell<HashMap<usize, Shared<parser_ll::TokenNames>>> = Default::default(); }
/// The core's table of token names, asking the generated function (token ids are hashes, there is no table to index).
#[repr(C)]
struct FnNames { base_0: parser_ll::TokenNames, spell: fn(usize) -> Ptr }
impl Upcast<parser_ll::TokenNames> for FnNames {
    unsafe fn up_ptr(this: *mut Self) -> *mut parser_ll::TokenNames { std::ptr::addr_of_mut!((*this).base_0) }
}
fn spell_names(token_name: fn(usize) -> Ptr) -> Shared<parser_ll::TokenNames> {
    Shared::new_with(|owner: &crate::WeakShared<FnNames>| {
        let mut names = FnNames { base_0: parser_ll::TokenNames::default(), spell: token_name };
        names.base_0.__bind_owner(&owner.upcast());
        names.base_0.__init__(&Vec::new());
        names.base_0.__vt_lookup = |base, id| {
            let this = unsafe { &mut *(base as *mut parser_ll::TokenNames as *mut FnNames) };
            let spell = this.spell;
            // the generated function panics for an id that is not a token
            std::panic::catch_unwind(|| spell(id as usize).remaining().to_string()).unwrap_or_default()
        };
        names
    }).upcast()
}

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
    /// Counters bound in the rule being parsed disappear when the guard does.
    pub fn counter_scope(&mut self) -> CounterScopeGuard { CounterScopeGuard { scope: self.core().borrow_mut().counter_scope() } }
    /// Bind `name` to how many elements `value` captured; a name bound twice has to agree.
    pub fn bind_counter(&mut self, name: impl AsRef<str>, value: impl CountOf) {
        self.core().borrow_mut().bind_counter_count(&name.as_ref().to_string(), value.count_of());
    }
    /// A matched result made from an action's value (the rule short-circuits on an unwinding recovery).
    pub fn action_result<R, IT>(&mut self, value: R, pos: IT) -> MatchResult<R, IT> {
        MatchResult { status: true, node: Node::create(value), it: pos }
    }
}

/// A repetition count: fixed, or the one bound to a name earlier (`A+ => n`, then `B+=n`).
pub struct CountSpec;
impl CountSpec {
    pub fn named(name: impl AsRef<str>, minimum: i64) -> Shared<core_counter::CountSpec> {
        let spec = core_counter::CountSpec::shared_default();
        spec.borrow_mut().__init__(&name.as_ref().to_string(), false, 0, minimum);
        spec
    }
    pub fn fixed(value: impl Into<i64>, minimum: i64) -> Shared<core_counter::CountSpec> {
        let value = value.into();
        if value < 0 { crate::fail(2, "a repetition count cannot be negative"); }
        let spec = core_counter::CountSpec::shared_default();
        spec.borrow_mut().__init__(&String::new(), true, value, if value < minimum { minimum } else { value });
        spec
    }
}
/// How many elements a captured value holds, for `bind_counter`.
pub trait CountOf { fn count_of(&self) -> i64; }
impl<T> CountOf for Vec<T> { fn count_of(&self) -> i64 { self.len() as i64 } }
impl<T> CountOf for Node<T> { fn count_of(&self) -> i64 { i64::from(!self.empty()) } }
/// The counters bound inside a rule are dropped with it.
pub struct CounterScopeGuard { scope: Shared<parser_ll::CounterScope> }
impl Drop for CounterScopeGuard { fn drop(&mut self) { self.scope.borrow_mut().restore(); } }

/// The combinators generated rules call. `Self` is the generated parser; its `LLParser` base holds the core.
pub trait LLParserApi<IT: BridgeCursor> {
    fn required<T: Clone + Default + 'static>(&mut self, pos: &mut IT, fail: impl IntoFail<IT>) -> Node<T>;
    fn required_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                   fail: impl IntoFail<IT>) -> Node<T>;
    fn optional<T: Clone + Default + 'static>(&mut self, pos: &mut IT) -> Node<T>;
    fn optional_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Node<T>;
    fn range<T: Clone + Default + 'static>(&mut self, pos: &mut IT, fail: impl IntoFail<IT>) -> Vec<Node<T>>;
    fn range_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                fail: impl IntoFail<IT>) -> Vec<Node<T>>;
    fn range_optional<T: Clone + Default + 'static>(&mut self, pos: &mut IT) -> Vec<Node<T>>;
    fn range_optional_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>)
        -> Vec<Node<T>>;
    fn counted<T: Clone + Default + 'static>(&mut self, pos: &mut IT, spec: Shared<core_counter::CountSpec>) -> Vec<Node<T>>;
    fn counted_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, spec: Shared<core_counter::CountSpec>,
                                                  rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Vec<Node<T>>;
}

/// The same combinators for repetitions that skip a space token between their elements (`range::<T, Space>`).
pub trait LLParserSpaced<IT: BridgeCursor>: LLParserApi<IT> {
    fn range_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, fail: impl IntoFail<IT>) -> Vec<Node<T>>;
    fn range_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                                   fail: impl IntoFail<IT>) -> Vec<Node<T>>;
    fn range_optional_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT) -> Vec<Node<T>>;
    fn range_optional_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT,
                                                                           rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Vec<Node<T>>;
    fn counted_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, spec: Shared<core_counter::CountSpec>) -> Vec<Node<T>>;
    fn counted_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, spec: Shared<core_counter::CountSpec>,
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
    fn required<T: Clone + Default + 'static>(&mut self, pos: &mut IT, fail: impl IntoFail<IT>) -> Node<T> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), true);
        let node = core.borrow_mut().required_token(cursor.clone().upcast(), token_id::<IT, T>(), fail.into_handler(&core));
        take(&cursor, pos);
        typed_node::<T>(&node)
    }
    fn required_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                   fail: impl IntoFail<IT>) -> Node<T> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), false);
        let callback = rule_callback::<Self, IT, T>(self as *mut Self, rec);
        let node = core.borrow_mut().required_rule(cursor.clone().upcast(), &String::new(), callback, fail.into_handler(&core));
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
    fn range<T: Clone + Default + 'static>(&mut self, pos: &mut IT, fail: impl IntoFail<IT>) -> Vec<Node<T>> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), true);
        let nodes = core.borrow_mut().range_token(cursor.clone().upcast(), token_id::<IT, T>(), fail.into_handler(&core));
        take(&cursor, pos);
        typed_nodes::<T>(nodes)
    }
    fn range_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                fail: impl IntoFail<IT>) -> Vec<Node<T>> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), false);
        let callback = rule_callback::<Self, IT, T>(self as *mut Self, rec);
        let nodes = core.borrow_mut().range_rule(cursor.clone().upcast(), &String::new(), callback, fail.into_handler(&core));
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
    fn counted<T: Clone + Default + 'static>(&mut self, pos: &mut IT, spec: Shared<core_counter::CountSpec>) -> Vec<Node<T>> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), true);
        let nodes = core.borrow_mut().counted_token(cursor.clone().upcast(), token_id::<IT, T>(), spec);
        take(&cursor, pos);
        typed_nodes::<T>(nodes)
    }
    fn counted_rule<T: Clone + Default + 'static>(&mut self, pos: &mut IT, spec: Shared<core_counter::CountSpec>,
                                                  rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Vec<Node<T>> {
        let core = self.core();
        let cursor = bridge::<IT, T>(pos.clone(), false);
        let callback = rule_callback::<Self, IT, T>(self as *mut Self, rec);
        let nodes = core.borrow_mut().counted_rule(cursor.clone().upcast(), callback, spec);
        take(&cursor, pos);
        typed_nodes::<T>(nodes)
    }
}

impl<P, IT, Main, Token> LLParserSpaced<IT> for P
where
    P: DerefMut<Target = LLParser<Main, Token>>,
    IT: BridgeCursor,
{
    fn range_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, fail: impl IntoFail<IT>) -> Vec<Node<T>> {
        self.use_space_token::<IT, S>();
        LLParserApi::range::<T>(self, pos, fail)
    }
    fn range_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>,
                                                                   fail: impl IntoFail<IT>) -> Vec<Node<T>> {
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
    fn counted_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, spec: Shared<core_counter::CountSpec>) -> Vec<Node<T>> {
        self.use_space_token::<IT, S>();
        LLParserApi::counted::<T>(self, pos, spec)
    }
    fn counted_rule_spaced<T: Clone + Default + 'static, S: 'static>(&mut self, pos: &mut IT, spec: Shared<core_counter::CountSpec>,
                                                                     rec: impl FnMut(&mut Self, IT) -> MatchResult<T, IT>) -> Vec<Node<T>> {
        self.use_space_token::<IT, S>();
        LLParserApi::counted_rule::<T>(self, pos, spec, rec)
    }
}
impl<Main, Token> LLParser<Main, Token> {
    fn use_space_token<IT: BridgeCursor, S: 'static>(&mut self) {
        if let Some(id) = <IT::Token as BridgeToken>::alt_of::<S>() { self.core().borrow_mut().set_space_token(id); }
    }
}
