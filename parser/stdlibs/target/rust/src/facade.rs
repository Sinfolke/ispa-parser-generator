//! The typed layer generated parsers are written against: typed nodes and tokens, the lexer and parser base classes,
//! and the cursors between them. Parsing itself (combinators, recovery, frames) runs in the lowered core
//! (`python::ispa_std::parser_ll`); this layer keeps ASTs and tokens typed on the way in and out.
use crate::{Error, Ptr, Shared, VariantGet};
use crate::lexer::TokenEnd;
use std::marker::PhantomData;
use std::rc::Rc;

/// The generated enum standing in for `std::variant<std::monostate, T0, T1...>` of a token type.
pub trait TokenVariant: Clone + Default + 'static {
    /// 0 for the empty alternative, then 1 + the position of the held type.
    fn variant_index(&self) -> usize;
    /// The held alternative (the unit value when empty), to read it as a `T` without a `VariantGet<T>` bound.
    fn held(&self) -> &dyn std::any::Any;
    /// The position (1-based, like `variant_index`) of `T` among the alternatives; a rule type is none of them.
    fn index_of<T: 'static>() -> Option<usize>;
}

// ---------------------------------------------------------------------------------------------------------------
// nodes, tokens, results
// ---------------------------------------------------------------------------------------------------------------
/// A matched grammar element: the typed value plus where it came from.
#[derive(Clone, Default)]
pub struct Node<T> {
    pub value: T,
    pub start_pos: i64,
    pub length: i64,
    pub line: i64,
    pub column: i64,
    pub name: i64,
    pub filled: bool,
}
impl<T> Node<T> {
    pub fn create(value: T) -> Self { Self { value, start_pos: 0, length: 0, line: 0, column: 0, name: 0, filled: true } }
    pub fn set_data(&mut self, value: T) { self.value = value; self.filled = true; }
    pub fn data(&self) -> &T { &self.value }
    pub fn data_mut(&mut self) -> &mut T { &mut self.value }
    pub fn empty(&self) -> bool { !self.filled }
}

/// One lexed token: the typed value (`V`, a variant over the token types) and its source span.
#[derive(Clone, Default)]
pub struct RawToken<V> {
    pub value: V,
    pub start_pos: i64,
    pub source: Ptr,
    pub length: i64,
    pub line: i64,
    pub id: usize,
}
impl<V: TokenVariant> RawToken<V> {
    /// The empty token.
    pub fn new() -> Self { Self::default() }
    pub fn create(start_pos: i64, source: Ptr, length: i64, line: i64, id: usize, value: impl Into<V>) -> Self {
        Self { value: value.into(), start_pos, source, length, line, id }
    }
    pub fn empty(&self) -> bool { self.value.variant_index() == 0 }
    pub fn index(&self) -> usize { self.value.variant_index() }
    /// The id of the token's type.
    pub fn kind(&self) -> usize { self.id }
    pub fn text(&self) -> String { self.source.remaining().get(..self.length as usize).unwrap_or("").to_owned() }
    pub fn column(&self) -> i64 { self.source.column() as i64 }
    /// The typed node of the token's value when it holds a `T` (an empty node otherwise).
    pub fn node_of<T: Clone + Default + 'static>(&self) -> Node<T> {
        match self.value.held().downcast_ref::<T>() {
            Some(value) => {
                let mut node = Node::create(value.clone());
                node.start_pos = self.start_pos;
                node.length = self.length;
                node.line = self.line;
                node.column = self.column();
                node.name = self.id as i64;
                node
            }
            None => Node::default(),
        }
    }
    /// The typed node of the token's value when it holds a `T`.
    pub fn to_node<T: Clone + Default>(&self) -> Node<T> where V: VariantGet<T> {
        if !VariantGet::<T>::holds(&self.value) { return Node::default(); }
        let mut node = Node::create(VariantGet::<T>::get(&self.value).clone());
        node.start_pos = self.start_pos;
        node.length = self.length;
        node.line = self.line;
        node.column = self.column();
        node.name = self.id as i64;
        node
    }
}

#[derive(Clone, Default)]
pub struct MatchResult<T, IT> {
    pub status: bool,
    pub node: Node<T>,
    pub it: IT,
}

// ---------------------------------------------------------------------------------------------------------------
// lexer
// ---------------------------------------------------------------------------------------------------------------
/// What a parser needs to know about the lexer it is generic over.
pub trait LexerTypes {
    type Token;
    type Iterator: Clone + Default;
    type LazyIterator: Clone + Default;
}

/// Cursor over tokens that already exist.
pub struct TokenIterator<Token> { tokens: Rc<Vec<Token>>, index: usize }
impl<Token> Clone for TokenIterator<Token> { fn clone(&self) -> Self { Self { tokens: self.tokens.clone(), index: self.index } } }
impl<Token> Default for TokenIterator<Token> { fn default() -> Self { Self { tokens: Rc::new(Vec::new()), index: 0 } } }
impl<Token> TokenIterator<Token> {
    pub fn new(tokens: Rc<Vec<Token>>, index: usize) -> Self { Self { tokens, index } }
    pub fn is_end(&self) -> bool { self.index >= self.tokens.len() }
    pub fn distance(&self) -> usize { self.index }
    pub fn get(&self) -> Option<&Token> { self.tokens.get(self.index) }
    pub fn advance(&mut self) { if !self.is_end() { self.index += 1; } }
    pub fn retreat(&mut self) { self.index = self.index.saturating_sub(1); }
}

/// The lexing a lazy cursor shares with its copies: the lexer, and where in the input it has got to.
struct LazyState<Token> { lexer: *mut LexerBase<Token>, pos: Ptr, finished: bool }
/// Cursor that lexes on demand: tokens are made (through the lexer's hooks, into its token list) when a position
/// past the last one is asked for. Copies share the lexer and keep their own index.
pub struct LazyIterator<Token> { state: Option<Rc<std::cell::RefCell<LazyState<Token>>>>, index: usize }
impl<Token> Clone for LazyIterator<Token> { fn clone(&self) -> Self { Self { state: self.state.clone(), index: self.index } } }
impl<Token> Default for LazyIterator<Token> { fn default() -> Self { Self { state: None, index: 0 } } }
impl<Token: Clone + Default + 'static + TokenEnd> LazyIterator<Token> {
    pub fn new(lexer: &mut LexerBase<Token>, input: Ptr) -> Self {
        Self { state: Some(Rc::new(std::cell::RefCell::new(LazyState { lexer, pos: input, finished: false }))), index: 0 }
    }
    /// Make tokens until there is one at `index` (or the input is over).
    fn ensure(&self) -> Option<&LexerBase<Token>> {
        let shared = self.state.as_ref()?;
        let lexer_ptr = shared.borrow().lexer;   // (a place taken straight from the borrow would keep it alive)
        let lexer = unsafe { &mut *lexer_ptr };
        loop {
            if lexer.tokens.len() > self.index { break; }
            let (finished, mut pos) = { let state = shared.borrow(); (state.finished, state.pos) };
            if finished { break; }
            if pos.is_eof() { shared.borrow_mut().finished = true; lexer.on_eof(); continue; }
            let before = pos.offset();
            let token = lexer.makeToken(&mut pos);
            if pos.offset() == before { crate::fail(1, "Lazy lexer did not advance source position"); }
            shared.borrow_mut().pos = pos;
            if !token.is_end() { lexer.on_token(token); }
        }
        Some(&*lexer)
    }
    /// The text being lexed.
    pub fn origin(&self) -> Option<&'static str> {
        let state = self.state.as_ref()?;
        let lexer = unsafe { &*state.borrow().lexer };
        lexer.input
    }
    pub fn token(&self) -> Option<Token> { self.ensure()?.tokens.get(self.index).cloned() }
    pub fn at_end(&self) -> bool { self.token().map_or(true, |token| token.is_end()) }
    /// Past the last token (the end marker included).
    pub fn is_end_lazy(&self) -> bool { self.at_end() }
    pub fn distance(&self) -> usize { self.index }
    // stepping onto a position makes its token (so a lexical error shows up when the parser gets there)
    pub fn advance(&mut self) { if !self.at_end() { self.index += 1; self.ensure(); } }
    pub fn retreat(&mut self) { self.index = self.index.saturating_sub(1); }
}

/// Base of generated lexers. `makeToken` and the hooks are dispatch slots the generated `Lexer` fills in.
#[repr(C)]
pub struct LexerBase<Token> {
    pub tokens: Vec<Token>,
    pub errors: Vec<String>,
    pub line: i64,
    pub external_boundary: bool,
    pub bound: i64,
    pub(crate) input: Option<&'static str>,
    pub __vt_clone_: fn(&LexerBase<Token>) -> Shared<LexerBase<Token>>,
    pub __vt_init: fn(&mut LexerBase<Token>),
    pub __vt_on_token: fn(&mut LexerBase<Token>, Token),
    pub __vt_on_eof: fn(&mut LexerBase<Token>),
    pub __vt_makeToken: fn(&mut LexerBase<Token>, &mut Ptr) -> Token,
}
impl<Token: Clone + 'static> Clone for LexerBase<Token> {
    fn clone(&self) -> Self {
        Self { tokens: self.tokens.clone(), errors: self.errors.clone(), line: self.line, external_boundary: self.external_boundary,
               bound: self.bound, input: self.input, __vt_clone_: self.__vt_clone_, __vt_init: self.__vt_init, __vt_on_token: self.__vt_on_token,
               __vt_on_eof: self.__vt_on_eof, __vt_makeToken: self.__vt_makeToken }
    }
}
impl<Token: Clone + Default + 'static> Default for LexerBase<Token> {
    fn default() -> Self {
        Self { tokens: Vec::new(), errors: Vec::new(), line: 1, external_boundary: false, bound: -1, input: None,
               __vt_clone_: |lexer| Shared::new(lexer.clone()),
               __vt_init: |_| {}, __vt_on_token: |lexer, token| lexer.append_token(token), __vt_on_eof: |_| {},
               __vt_makeToken: |_, _| Token::default() }
    }
}
impl<Token> LexerTypes for LexerBase<Token> {
    type Token = Token;
    type Iterator = TokenIterator<Token>;
    type LazyIterator = LazyIterator<Token>;
}
impl<Token: Clone + Default + 'static> LexerBase<Token> {
    /// Source queries of token actions (`ctx.next_char`, the line of an emitted token), by position.
    pub fn __line(&self, at: Ptr) -> i64 { at.line() as i64 }
    pub fn __next_char(&self, at: Ptr) -> String { at.get().map(|byte| (byte as char).to_string()).unwrap_or_default() }
    pub fn __line_prefix(&self, at: Ptr) -> String {
        let text = at.text().as_bytes();
        let end = at.offset().min(text.len());
        let begin = text[..end].iter().rposition(|byte| *byte == b'\n' || *byte == b'\r').map_or(0, |i| i + 1);
        text[begin..end].iter().map(|byte| *byte as char).collect()
    }
    pub fn append_token(&mut self, token: Token) { self.tokens.push(token); }
    pub fn clone_(&self) -> Shared<LexerBase<Token>> { (self.__vt_clone_)(self) }
    pub fn init(&mut self) { (self.__vt_init)(self) }
    pub fn on_token(&mut self, token: Token) { (self.__vt_on_token)(self, token) }
    pub fn on_eof(&mut self) { (self.__vt_on_eof)(self) }
    pub fn makeToken(&mut self, pos: &mut Ptr) -> Token { (self.__vt_makeToken)(self, pos) }
}

/// How a parser obtains cursors over a lexer.
pub trait TokenStream {
    type Cursor;
    type LazyCursor;
    fn token_cursor(&mut self) -> Self::Cursor;
    fn lazy_token_cursor(&mut self, input: Ptr) -> Self::LazyCursor;
    fn reset_token_stream(&mut self);
}
impl<Token: Clone + Default + 'static + TokenEnd> TokenStream for Shared<LexerBase<Token>> {
    type Cursor = TokenIterator<Token>;
    type LazyCursor = LazyIterator<Token>;
    fn token_cursor(&mut self) -> TokenIterator<Token> { TokenIterator::new(Rc::new(self.borrow().tokens.clone()), 0) }
    fn lazy_token_cursor(&mut self, input: Ptr) -> LazyIterator<Token> { self.borrow_mut().begin_lazy(input) }
    fn reset_token_stream(&mut self) { self.borrow_mut().tokens.clear(); }
}
impl<L, Token: Clone + Default + 'static + TokenEnd> TokenStream for L where L: std::ops::DerefMut<Target = LexerBase<Token>> {
    type Cursor = TokenIterator<Token>;
    type LazyCursor = LazyIterator<Token>;
    fn token_cursor(&mut self) -> TokenIterator<Token> { TokenIterator::new(Rc::new(self.tokens.clone()), 0) }
    fn lazy_token_cursor(&mut self, input: Ptr) -> LazyIterator<Token> { self.begin_lazy(input) }
    fn reset_token_stream(&mut self) { self.tokens.clear(); }
}

// ---------------------------------------------------------------------------------------------------------------
// parser
// ---------------------------------------------------------------------------------------------------------------
/// The id of the token a cursor is at (`END_OF_INPUT` past the last).
pub fn lookahead_kind<IT: TokenCursorKind>(cursor: IT) -> usize { cursor.kind() }
pub trait TokenCursorKind { fn kind(&self) -> usize; }
impl<V: TokenVariant> TokenCursorKind for TokenIterator<RawToken<V>> {
    fn kind(&self) -> usize { self.get().filter(|token| !token.empty()).map_or(crate::recovery::END_OF_INPUT, |token| token.id) }
}
impl<V: TokenVariant> TokenCursorKind for LazyIterator<RawToken<V>> {
    fn kind(&self) -> usize { self.token().filter(|token| !token.empty()).map_or(crate::recovery::END_OF_INPUT, |token| token.id) }
}



// ---------------------------------------------------------------------------------------------------------------
// skipping
// ---------------------------------------------------------------------------------------------------------------
/// `skip_spaces::<T>(&mut cursor)`: in source text white space; in a token stream a token holding `T`.
pub trait SkipSpaces { fn skip_space<T: 'static>(&mut self); }
impl SkipSpaces for Ptr {
    fn skip_space<T: 'static>(&mut self) { while self.get().is_some_and(|byte| byte.is_ascii_whitespace()) { self.inc(); } }
}
impl<V: TokenVariant> SkipSpaces for TokenIterator<RawToken<V>> {
    fn skip_space<T: 'static>(&mut self) { if self.get().is_some_and(|token| token.value.held().is::<T>()) { self.advance(); } }
}
impl<V: TokenVariant> SkipSpaces for LazyIterator<RawToken<V>> {
    fn skip_space<T: 'static>(&mut self) { if self.token().is_some_and(|token| token.value.held().is::<T>()) { self.advance(); } }
}
pub fn skip_spaces<T: 'static>(pos: &mut impl SkipSpaces) { pos.skip_space::<T>() }

/// Everything the generated rules need of the iterator they are generic over.
pub trait ParserCursor: crate::BridgeCursor + Default + TokenCursorKind + SkipSpaces + Advance {}
impl<C: crate::BridgeCursor + Default + TokenCursorKind + SkipSpaces + Advance> ParserCursor for C {}

/// `++pos`: step to the next token; the new position is returned.
pub trait Advance: Sized { fn inc(&mut self) -> Self; }
impl<V: TokenVariant> Advance for TokenIterator<RawToken<V>> { fn inc(&mut self) -> Self { self.advance(); self.clone() } }
impl<V: TokenVariant> Advance for LazyIterator<RawToken<V>> { fn inc(&mut self) -> Self { self.advance(); self.clone() } }
