//! LR parsers: the driver (`python::ispa_std::lr`) runs in the lowered core; this layer is what a generated LR parser
//! is written against. The generated class supplies the tables (`LrTables`: action/goto/reduce... as associated
//! functions, the way the C++ facade finds them as statics), the value helpers it calls (`lr_token`, `lr_pack`,
//! `lr_capture_assign`, `lr_get`, `lr_node`) build and read the closed-over-everything `LRValue` the stack holds.
use crate::python::ispa_std::{lexer::TokenCursor, lr as core_lr, lr_conflicts as core_conflicts, raw_token::RawToken as CoreToken};
use crate::{typed_token, BridgeCursor, LazyIterator, LexerBase, MatchResult, Node, Ptr, Shared, TokenEnd, TypedCursor, Upcast, WeakShared};
use std::any::Any;
use std::marker::PhantomData;
use std::ops::DerefMut;
use std::rc::Rc;

// ---------------------------------------------------------------------------------------------------------------
// values
// ---------------------------------------------------------------------------------------------------------------
/// What a payload of the value stack can do besides being looked at: a rule's node can also be had as a node holding
/// a `Shared` (the AST boxes recursive rules; the grammar's field may be either).
trait Payload: 'static {
    fn any(&self) -> &dyn Any;
    fn boxed(&self) -> Option<Box<dyn Any>>;
}
impl<T: Clone + 'static> Payload for Node<T> {
    fn any(&self) -> &dyn Any { self }
    fn boxed(&self) -> Option<Box<dyn Any>> {
        Some(Box::new(Node { value: Shared::new(self.value.clone()), start_pos: self.start_pos, length: self.length, line: self.line,
                             column: self.column, name: self.name, filled: self.filled }))
    }
}
impl Payload for Captures {
    fn any(&self) -> &dyn Any { self }
    fn boxed(&self) -> Option<Box<dyn Any>> { None }
}

/// Values captured by the symbols of a production, per destination column (`repeated` marks a repetition).
pub struct Captures {
    pub columns: Vec<Vec<LRValue>>,
    pub repeated: bool,
}
impl Captures {
    fn is_empty(&self) -> bool { self.columns.iter().all(Vec::is_empty) }
}

/// An element of the LR value stack: a shifted token (its node and text), a reduced rule's node, or packed captures.
#[derive(Clone)]
pub struct LRValue {
    payload: Option<Rc<dyn Payload>>,
    token: bool,
    text: Rc<str>,
}
impl Default for LRValue {
    fn default() -> Self { Self { payload: None, token: false, text: Rc::from("") } }
}
// the lowered driver is generic over a value type that can be compared and copied; identity is all that means here
impl PartialEq for LRValue {
    fn eq(&self, other: &Self) -> bool {
        match (&self.payload, &other.payload) {
            (Some(a), Some(b)) => Rc::ptr_eq(a, b),
            (None, None) => true,
            _ => false,
        }
    }
}
impl PartialOrd for LRValue {
    fn partial_cmp(&self, _other: &Self) -> Option<std::cmp::Ordering> { None }
}
impl LRValue {
    fn of(payload: impl Payload, token: bool, text: &str) -> Self { Self { payload: Some(Rc::new(payload)), token, text: Rc::from(text) } }
    fn captures_of(captures: Captures) -> Self { Self::of(captures, false, "") }
    fn captures(&self) -> Option<&Captures> { self.payload.as_ref()?.any().downcast_ref::<Captures>() }
    /// Whether this is a shifted token.
    pub fn is_token(&self) -> bool { self.token }
    /// The text of a shifted token.
    pub fn text(&self) -> &str { &self.text }
    /// The value as an `N` (a node), directly or in its boxed form.
    fn node_as<N: Clone + 'static>(&self) -> Option<N> {
        let payload = self.payload.as_ref()?;
        if let Some(found) = payload.any().downcast_ref::<N>() { return Some(found.clone()); }
        payload.boxed()?.downcast::<N>().ok().map(|boxed| *boxed)
    }
    /// A token's value with its text (the shift hook of the generated tables does not see the text).
    fn with_text(mut self, text: String) -> Self {
        if self.token { self.text = Rc::from(text); }
        self
    }
}

/// A target of a capture: how a value of the stack becomes the AST field of that type (port of ispastdlib's `lr_convert`).
pub trait LrConvert: Sized {
    fn lr_convert(value: &LRValue) -> Option<Self>;
}

/// The conversions shared by every target: empty captures are the default, one captured value converts as the target.
fn from_captures<T: LrConvert + Default>(value: &LRValue) -> Option<T> {
    let captures = value.captures()?;
    if captures.is_empty() { return Some(T::default()); }
    let mut selected = None;
    for item in captures.columns.iter().flatten() {
        if selected.is_some() { return None; }
        selected = Some(item);
    }
    T::lr_convert(selected?)
}
impl<T: Clone + Default + 'static> LrConvert for Node<T> {
    fn lr_convert(value: &LRValue) -> Option<Self> { value.node_as::<Node<T>>().or_else(|| from_captures(value)) }
}
impl LrConvert for String {
    fn lr_convert(value: &LRValue) -> Option<Self> { if value.token { Some(value.text.to_string()) } else { from_captures(value) } }
}
impl LrConvert for char {
    fn lr_convert(value: &LRValue) -> Option<Self> {
        if value.token { let mut chars = value.text.chars(); return match (chars.next(), chars.next()) { (Some(c), None) => Some(c), _ => None }; }
        from_captures(value)
    }
}
impl LrConvert for bool { fn lr_convert(value: &LRValue) -> Option<Self> { from_captures(value) } }
impl LrConvert for i64 { fn lr_convert(value: &LRValue) -> Option<Self> { from_captures(value) } }
impl<T: LrConvert + Default> LrConvert for Vec<T> {
    fn lr_convert(value: &LRValue) -> Option<Self> {
        let captures = value.captures()?;
        if captures.is_empty() { return Some(Vec::new()); }
        let mut out = Vec::new();
        if captures.repeated && captures.columns.len() > 1 {
            // columns captured side by side: one element per row
            let rows = captures.columns[0].len();
            if captures.columns.iter().any(|column| column.len() != rows) { return None; }
            for row in 0..rows {
                let item = Captures { columns: captures.columns.iter().map(|column| vec![column[row].clone()]).collect(), repeated: false };
                out.push(T::lr_convert(&LRValue::captures_of(item))?);
            }
            return Some(out);
        }
        for item in captures.columns.iter().flatten() {
            match T::lr_convert(item) {
                Some(element) => out.push(element),
                None => out.extend(Vec::<T>::lr_convert(item)?),
            }
        }
        Some(out)
    }
}
macro_rules! tuple_convert {
    ($count:expr; $($name:ident $index:tt),+) => {
        impl<$($name: LrConvert + Default),+> LrConvert for ($($name,)+) {
            fn lr_convert(value: &LRValue) -> Option<Self> {
                let captures = value.captures()?;
                if captures.is_empty() { return Some(Default::default()); }
                if captures.columns.len() != $count { return None; }
                Some(($($name::lr_convert(&LRValue::captures_of(Captures { columns: vec![captures.columns[$index].clone()], repeated: false }))?,)+))
            }
        }
    };
}
tuple_convert!(1; A 0);
tuple_convert!(2; A 0, B 1);
tuple_convert!(3; A 0, B 1, C 2);
tuple_convert!(4; A 0, B 1, C 2, D 3);
tuple_convert!(5; A 0, B 1, C 2, D 3, E 4);
tuple_convert!(6; A 0, B 1, C 2, D 3, E 4, F 5);
tuple_convert!(7; A 0, B 1, C 2, D 3, E 4, F 5, G 6);
tuple_convert!(8; A 0, B 1, C 2, D 3, E 4, F 5, G 6, H 7);

// ---------------------------------------------------------------------------------------------------------------
// the helpers generated reductions call
// ---------------------------------------------------------------------------------------------------------------
/// A shifted terminal: its node (the text is added when the driver shifts it, see `LRAdapter`).
pub fn lr_token<T: Clone + 'static>(node: Node<T>) -> LRValue { LRValue::of(node, true, "") }
/// The value of a reduced rule.
pub fn lr_node<T: Clone + 'static>(value: T) -> LRValue { LRValue::of(Node::create(value), false, "") }
/// The node a stack element holds.
pub fn lr_get<T: Clone + Default + 'static>(values: &[LRValue], index: usize) -> Node<T> {
    values.get(index).and_then(LRValue::node_as::<Node<T>>).unwrap_or_default()
}
/// Gather the stack elements `indices` into `count` columns (`offsets` say which; `splice` elements are packs to merge).
pub fn lr_pack(values: &[LRValue], indices: &[usize], offsets: &[usize], splice: &[usize], count: usize, repeated: bool) -> LRValue {
    let mut columns: Vec<Vec<LRValue>> = vec![Vec::new(); count];
    for (i, index) in indices.iter().enumerate() {
        let value = &values[*index];
        if splice[i] != 0 {
            let Some(child) = value.captures() else { crate::fail(4, "LR splice of a value that is not a pack") };
            for (c, column) in child.columns.iter().enumerate() { columns[offsets[i] + c].extend(column.iter().cloned()); }
        } else {
            columns[offsets[i]].push(value.clone());
        }
    }
    LRValue::captures_of(Captures { columns, repeated })
}
/// Fill an AST field from the stack element `index` (or one column of the pack it is, when `column` is not `usize::MAX`).
pub fn lr_capture_assign<T: LrConvert>(target: &mut T, values: &[LRValue], index: usize, column: usize) {
    let value = &values[index];
    let converted = if column == usize::MAX {
        T::lr_convert(value)
    } else {
        let Some(captures) = value.captures() else { crate::fail(4, "LR capture of a value that is not a pack") };
        T::lr_convert(&LRValue::captures_of(Captures { columns: vec![captures.columns[column].clone()], repeated: captures.repeated }))
    };
    match converted {
        Some(converted) => *target = converted,
        None => crate::fail(3, format!("LR capture payload does not match its declared AST field type: {}", std::any::type_name::<T>())),
    }
}

// ---------------------------------------------------------------------------------------------------------------
// the tables of a generated parser
// ---------------------------------------------------------------------------------------------------------------
/// The static callbacks of a generated LR parser. `lr_action` is signed (shift = state + 1, reduce = -rule - 1, accept 0);
/// the optional tables are present when their `HAS_*` constant says so.
pub trait LrTables {
    type Token: Clone + 'static;
    fn lr_k() -> usize;
    fn lr_action(state: usize, window: &mut Vec<usize>) -> i64;
    fn lr_shift(token: &mut Self::Token) -> LRValue;
    fn lr_length(rule: usize) -> usize;
    fn lr_reduce(rule: usize, values: &mut Vec<LRValue>) -> LRValue;
    fn lr_lhs(rule: usize) -> usize;
    fn lr_goto(state: usize, lhs: usize) -> usize;

    const HAS_IS_SPACE: bool = false;
    fn lr_is_space(_token: &mut Self::Token) -> bool { false }
    const HAS_SPACE: bool = false;
    fn lr_space(_rule: usize, _gap: usize) -> bool { false }
    const HAS_EXPECTED: bool = false;
    fn lr_expected(_state: usize) -> Vec<usize> { Vec::new() }
    const HAS_CONTEXT: bool = false;
    fn lr_context(_state: usize) -> String { String::new() }
    const HAS_TOKEN_NAME: bool = false;
    fn lr_token_name(_token: usize) -> String { String::new() }
    const HAS_RULE_LABEL: bool = false;
    fn lr_rule_label(_rule: usize) -> String { String::new() }
    const HAS_CONFLICT: bool = false;
    fn lr_conflict(_state: usize, _window: &mut Vec<usize>) -> Vec<i64> { Vec::new() }
}

const ABSENT: usize = usize::MAX;
fn unsigned(id: i64) -> usize { if id < 0 { ABSENT } else { id as usize } }
fn signed(id: usize) -> i64 { if id == ABSENT { -1 } else { id as i64 } }

/// The generated tables as the core's `LRGrammar` (a subclass whose dispatch slots call the associated functions).
#[repr(C)]
struct LRAdapter<G: LrTables> {
    base_0: core_lr::LRGrammar<LRValue>,
    _tables: PhantomData<G>,
}
impl<G: LrTables + 'static> Upcast<core_lr::LRGrammar<LRValue>> for LRAdapter<G> {
    unsafe fn up_ptr(this: *mut Self) -> *mut core_lr::LRGrammar<LRValue> { std::ptr::addr_of_mut!((*this).base_0) }
}
impl<G: LrTables + 'static> LRAdapter<G> {
    fn token_of(raw: &Shared<CoreToken>) -> G::Token {
        typed_token::<G::Token>(raw).unwrap_or_else(|| crate::fail(4, "LR: the token the driver shifted is not one of the parser's"))
    }

    fn handle() -> Shared<core_lr::LRGrammar<LRValue>> {
        Shared::new_with(|owner: &WeakShared<LRAdapter<G>>| {
            let mut adapter = LRAdapter::<G> { base_0: core_lr::LRGrammar::<LRValue>::default(), _tables: PhantomData };
            adapter.base_0.__bind_owner(&owner.upcast());
            let grammar = &mut adapter.base_0;
            grammar.__vt_lr_k = |_| G::lr_k() as i64;
            grammar.__vt_lr_action = |_, state, window| {
                let mut converted: Vec<usize> = window.iter().map(|id| unsigned(*id)).collect();
                G::lr_action(state as usize, &mut converted)
            };
            grammar.__vt_lr_shift = |_, raw| {
                let mut token = Self::token_of(&raw);
                let text = raw.borrow_mut().text();
                G::lr_shift(&mut token).with_text(text)
            };
            grammar.__vt_lr_length = |_, rule| G::lr_length(rule as usize) as i64;
            grammar.__vt_lr_reduce = |_, rule, rhs| {
                let mut values = rhs.clone();
                G::lr_reduce(rule as usize, &mut values)
            };
            grammar.__vt_lr_lhs = |_, rule| G::lr_lhs(rule as usize) as i64;
            grammar.__vt_lr_goto = |_, state, lhs| G::lr_goto(state as usize, lhs as usize) as i64;
            grammar.__vt_has_is_space = |_| G::HAS_IS_SPACE;
            grammar.__vt_lr_is_space = |_, raw| { let mut token = Self::token_of(&raw); G::lr_is_space(&mut token) };
            grammar.__vt_has_space = |_| G::HAS_SPACE;
            grammar.__vt_lr_space = |_, rule, gap| G::lr_space(rule as usize, gap as usize);
            grammar.__vt_has_expected = |_| G::HAS_EXPECTED;
            grammar.__vt_lr_expected = |_, state| G::lr_expected(state as usize).into_iter().map(signed).collect();
            grammar.__vt_has_context = |_| G::HAS_CONTEXT;
            grammar.__vt_lr_context = |_, state| G::lr_context(state as usize);
            grammar.__vt_has_token_name = |_| G::HAS_TOKEN_NAME;
            grammar.__vt_lr_token_name = |_, id| G::lr_token_name(unsigned(id));
            grammar.__vt_has_rule_label = |_| G::HAS_RULE_LABEL;
            grammar.__vt_lr_rule_label = |_, rule| G::lr_rule_label(rule as usize);
            grammar.__vt_lr_conflict = |_, state, window| {
                if !G::HAS_CONFLICT { return Vec::new(); }
                let mut converted: Vec<usize> = window.iter().map(|id| unsigned(*id)).collect();
                G::lr_conflict(state as usize, &mut converted)
            };
            adapter
        }).upcast()
    }
}

// ---------------------------------------------------------------------------------------------------------------
// the parser base
// ---------------------------------------------------------------------------------------------------------------
/// Base of generated LR parsers (deterministic, and generalized: one driver per mode).
#[repr(C)]
pub struct LRParser<Main, Token> {
    pub lexer: Shared<LexerBase<Token>>,
    pub text: Ptr,
    pub tree: Main,
    core: Option<Shared<core_lr::LRParser>>,
    generalized: Option<Shared<core_conflicts::LRGeneralizedParser>>,
    trailing: Vec<(String, String)>,
    pub __vt_parseFromTokens: fn(&mut LRParser<Main, Token>),
    pub __vt_lazyParse: fn(&mut LRParser<Main, Token>) -> MatchResult<Main, LazyIterator<Token>>,
}
/// A generalized parser is the same base: it also has `lr_parse_generalized`.
pub type LRGeneralizedParser<Main, Token> = LRParser<Main, Token>;

impl<Main: Clone, Token> Clone for LRParser<Main, Token> {
    fn clone(&self) -> Self {
        Self { lexer: self.lexer.clone(), text: self.text, tree: self.tree.clone(), core: self.core.clone(), generalized: self.generalized.clone(),
               trailing: self.trailing.clone(), __vt_parseFromTokens: self.__vt_parseFromTokens, __vt_lazyParse: self.__vt_lazyParse }
    }
}
impl<Main: Default, Token: Clone + Default + 'static> Default for LRParser<Main, Token> {
    fn default() -> Self {
        Self { lexer: Shared::default(), text: Ptr::new(""), tree: Main::default(), core: None, generalized: None, trailing: Vec::new(),
               __vt_parseFromTokens: |_| {}, __vt_lazyParse: |_| MatchResult { status: false, node: Node::default(), it: LazyIterator::default() } }
    }
}
impl<Main, Token> LRParser<Main, Token> {
    pub fn parseFromTokens(&mut self) where Main: 'static, Token: 'static { (self.__vt_parseFromTokens)(self) }
    pub fn lazyParse(&mut self) -> MatchResult<Main, LazyIterator<Token>> { (self.__vt_lazyParse)(self) }
    /// Parse the tokens of `self.lexer`; the tree is `self.tree`.
    pub fn parse_tokens(&mut self) -> &Main where Main: 'static, Token: 'static {
        self.trailing.clear();
        self.parseFromTokens();
        &self.tree
    }
    /// Parse `text`, lexing it on demand; the tree is `self.tree`. The text is kept for the life of the program.
    pub fn parse_text(&mut self, text: &str) -> &Main where Main: Clone, Token: Clone + Default + 'static + TokenEnd {
        self.trailing.clear();
        self.text = Ptr::new(Box::leak(text.to_string().into_boxed_str()));
        let result = self.lazyParse();
        let at_end = result.it.is_end_lazy();
        if !result.status || !at_end {
            self.trailing.push(("main".into(), if result.status { "unexpected trailing input" } else { "could not match main rule" }.into()));
        }
        if result.status { self.tree = result.node.data().clone(); }
        &self.tree
    }
    /// What the driver reported for the last parse: (rule, message) of each diagnostic.
    pub fn diagnostics(&mut self) -> Vec<(String, String)> {
        let mut found = Vec::new();
        let errors = match (&self.generalized, &self.core) {
            (Some(core), _) => core.borrow_mut().getErrors().clone(),
            (None, Some(core)) => core.borrow_mut().getErrors().clone(),
            (None, None) => Vec::new(),
        };
        for error in errors { found.push((error.borrow().rule.clone(), error.borrow().message.clone())); }
        found.extend(self.trailing.iter().cloned());
        found
    }
    fn deterministic_core(&mut self) -> Shared<core_lr::LRParser> {
        self.core.get_or_insert_with(|| { let core = core_lr::LRParser::shared_default(); core.borrow_mut().__init__(); core }).clone()
    }
    fn generalized_core(&mut self) -> Shared<core_conflicts::LRGeneralizedParser> {
        self.generalized.get_or_insert_with(|| { let core = core_conflicts::LRGeneralizedParser::shared_default(); core.borrow_mut().__init__(); core }).clone()
    }
}

/// `lr_parse::<Tables>(pos)` of a generated parser: run the core's driver over the tokens at `pos`.
pub trait LRParserApi<IT: BridgeCursor, Main> {
    fn lr_parse<G: LrTables + 'static>(&mut self, pos: IT) -> MatchResult<Main, IT>;
    fn lr_parse_generalized<G: LrTables + 'static>(&mut self, pos: IT) -> MatchResult<Main, IT>;
}

fn result_of<IT: BridgeCursor, Main: Clone + Default + 'static>(result: Shared<crate::python::ispa_std::match_result::MatchResult>) -> MatchResult<Main, IT> {
    let result = result.borrow();
    let mut cursor = result.it.borrow_mut();
    let at = unsafe { &*(&mut *cursor as *mut TokenCursor as *mut TypedCursor<IT>) }.it.clone();
    let value = result.node.borrow()._data.downcast_ref::<LRValue>().cloned();
    let node = value.and_then(|value| value.node_as::<Node<Main>>());
    match node {
        Some(node) => MatchResult { status: result.status, node, it: at },
        None => crate::fail(3, "LR parse did not end in the parser's main rule"),
    }
}

impl<P, IT, Main, Token> LRParserApi<IT, Main> for P
where
    P: DerefMut<Target = LRParser<Main, Token>>,
    IT: BridgeCursor,
    Main: Clone + Default + 'static,
{
    fn lr_parse<G: LrTables + 'static>(&mut self, pos: IT) -> MatchResult<Main, IT> {
        let core = self.deterministic_core();
        let cursor = TypedCursor::carrying(pos);
        let result = core.borrow_mut().lr_parse::<LRValue>(LRAdapter::<G>::handle(), cursor.upcast());
        result_of::<IT, Main>(result)
    }
    fn lr_parse_generalized<G: LrTables + 'static>(&mut self, pos: IT) -> MatchResult<Main, IT> {
        let core = self.generalized_core();
        let cursor = TypedCursor::carrying(pos);
        let result = core.borrow_mut().lr_parse_generalized::<LRValue>(LRAdapter::<G>::handle(), cursor.upcast());
        result_of::<IT, Main>(result)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Clone, Default, Debug, PartialEq)]
    struct Item(i32);
    fn item(value: i32) -> LRValue { lr_node(Item(value)) }

    #[test]
    fn repeated_captures_fill_a_vector_in_order() {
        let values = vec![item(1), item(2), item(3)];
        let pack = lr_pack(&values, &[0, 2], &[0, 0], &[0, 0], 1, true);
        let mut target: Vec<Node<Item>> = Vec::new();
        lr_capture_assign(&mut target, &[pack], 0, 0);
        assert_eq!(target.iter().map(|node| node.value.0).collect::<Vec<_>>(), vec![1, 3]);
    }

    #[test]
    fn a_pack_spliced_into_another_merges_its_columns() {
        let inner = lr_pack(&[item(1), item(2)], &[0, 1], &[0, 1], &[0, 0], 2, true);
        let outer = lr_pack(&[item(9), inner], &[0, 1], &[0, 0], &[0, 1], 2, true);
        let mut firsts: Vec<Node<Item>> = Vec::new();
        let mut seconds: Vec<Node<Item>> = Vec::new();
        lr_capture_assign(&mut firsts, &[outer.clone()], 0, 0);
        lr_capture_assign(&mut seconds, &[outer], 0, 1);
        assert_eq!((firsts.len(), seconds.len()), (2, 1));
        assert_eq!(seconds[0].value, Item(2));
    }

    #[test]
    fn one_value_fills_a_node_and_a_boxed_node() {
        let mut plain: Node<Item> = Node::default();
        lr_capture_assign(&mut plain, &[item(5)], 0, usize::MAX);
        assert_eq!(plain.value, Item(5));
        let mut boxed: Node<Shared<Item>> = Node::default();
        lr_capture_assign(&mut boxed, &[item(6)], 0, usize::MAX);
        assert_eq!(boxed.value.borrow().0, 6);
    }

    #[test]
    fn empty_captures_are_the_default_and_a_tuple_takes_one_column_each() {
        let empty = lr_pack(&[], &[], &[], &[], 2, false);
        let mut pair: (Node<Item>, Vec<Node<Item>>) = Default::default();
        lr_capture_assign(&mut pair, &[empty], 0, usize::MAX);
        assert!(pair.0.empty() && pair.1.is_empty());
        let filled = lr_pack(&[item(1), item(2)], &[0, 1], &[0, 1], &[0, 0], 2, false);
        lr_capture_assign(&mut pair, &[filled], 0, usize::MAX);
        assert_eq!((pair.0.value.0, pair.1[0].value.0), (1, 2));
    }

    #[test]
    fn a_shifted_token_is_text_for_strings_and_chars() {
        let token = lr_token(Node::create(Item(0))).with_text("x".into());
        let (mut text, mut letter) = (String::new(), ' ');
        lr_capture_assign(&mut text, &[token.clone()], 0, usize::MAX);
        lr_capture_assign(&mut letter, &[token], 0, usize::MAX);
        assert_eq!((text.as_str(), letter), ("x", 'x'));
    }
}
