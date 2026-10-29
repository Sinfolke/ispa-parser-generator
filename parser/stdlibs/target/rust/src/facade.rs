//! The typed layer generated parsers are written against: typed nodes and tokens, the lexer and parser base classes,
//! and the cursors between them. Parsing itself (combinators, recovery, frames) runs in the lowered core
//! (`python::ispa_std::parser_ll`); this layer keeps ASTs and tokens typed on the way in and out.
use crate::{Error, Ptr, Shared, VariantGet};
use std::marker::PhantomData;
use std::rc::Rc;

/// The generated enum standing in for `std::variant<std::monostate, T0, T1...>` of a token type.
pub trait TokenVariant: Clone + Default + 'static {
    /// 0 for the empty alternative, then 1 + the position of the held type.
    fn variant_index(&self) -> usize;
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
    pub fn create(start_pos: i64, source: Ptr, length: i64, line: i64, id: usize, value: impl Into<V>) -> Self {
        Self { value: value.into(), start_pos, source, length, line, id }
    }
    pub fn empty(&self) -> bool { self.value.variant_index() == 0 }
    pub fn index(&self) -> usize { self.value.variant_index() }
    pub fn text(&self) -> String { self.source.remaining().get(..self.length as usize).unwrap_or("").to_owned() }
    pub fn column(&self) -> i64 { self.source.column() as i64 }
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

/// Cursor that lexes on demand from source text.
pub struct LazyIterator<Token> { _token: PhantomData<Token>, index: usize }
impl<Token> Clone for LazyIterator<Token> { fn clone(&self) -> Self { Self { _token: PhantomData, index: self.index } } }
impl<Token> Default for LazyIterator<Token> { fn default() -> Self { Self { _token: PhantomData, index: 0 } } }

/// Base of generated lexers. `makeToken` and the hooks are dispatch slots the generated `Lexer` fills in.
#[repr(C)]
pub struct LexerBase<Token> {
    pub tokens: Vec<Token>,
    pub errors: Vec<String>,
    pub line: i64,
    pub external_boundary: bool,
    pub bound: i64,
    pub __vt_clone_: fn(&LexerBase<Token>) -> Shared<LexerBase<Token>>,
    pub __vt_init: fn(&mut LexerBase<Token>),
    pub __vt_on_token: fn(&mut LexerBase<Token>, Token),
    pub __vt_on_eof: fn(&mut LexerBase<Token>),
    pub __vt_makeToken: fn(&mut LexerBase<Token>, &Ptr) -> Token,
}
impl<Token: Clone + 'static> Clone for LexerBase<Token> {
    fn clone(&self) -> Self {
        Self { tokens: self.tokens.clone(), errors: self.errors.clone(), line: self.line, external_boundary: self.external_boundary,
               bound: self.bound, __vt_clone_: self.__vt_clone_, __vt_init: self.__vt_init, __vt_on_token: self.__vt_on_token,
               __vt_on_eof: self.__vt_on_eof, __vt_makeToken: self.__vt_makeToken }
    }
}
impl<Token: Clone + Default + 'static> Default for LexerBase<Token> {
    fn default() -> Self {
        Self { tokens: Vec::new(), errors: Vec::new(), line: 1, external_boundary: false, bound: -1,
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
    pub fn append_token(&mut self, token: Token) { self.tokens.push(token); }
    pub fn clone_(&self) -> Shared<LexerBase<Token>> { (self.__vt_clone_)(self) }
    pub fn init(&mut self) { (self.__vt_init)(self) }
    pub fn on_token(&mut self, token: Token) { (self.__vt_on_token)(self, token) }
    pub fn on_eof(&mut self) { (self.__vt_on_eof)(self) }
    pub fn makeToken(&mut self, pos: &Ptr) -> Token { (self.__vt_makeToken)(self, pos) }
}

/// How a parser obtains cursors over a lexer.
pub trait TokenStream {
    type Cursor;
    type LazyCursor;
    fn token_cursor(&mut self) -> Self::Cursor;
    fn lazy_token_cursor(&mut self, input: Ptr) -> Self::LazyCursor;
    fn reset_token_stream(&mut self);
}
impl<L, Token: Clone + Default + 'static> TokenStream for L where L: std::ops::DerefMut<Target = LexerBase<Token>> {
    type Cursor = TokenIterator<Token>;
    type LazyCursor = LazyIterator<Token>;
    fn token_cursor(&mut self) -> TokenIterator<Token> { TokenIterator::new(Rc::new(self.tokens.clone()), 0) }
    fn lazy_token_cursor(&mut self, _input: Ptr) -> LazyIterator<Token> { unimplemented!("lazy lexing") }
    fn reset_token_stream(&mut self) { self.tokens.clear(); }
}

// ---------------------------------------------------------------------------------------------------------------
// parser
// ---------------------------------------------------------------------------------------------------------------
/// The id of the token a cursor is at (`END_OF_INPUT` past the last).
pub fn lookahead_kind<IT: TokenCursorKind>(cursor: IT) -> usize { cursor.kind() }
pub trait TokenCursorKind { fn kind(&self) -> usize; }
impl<V: TokenVariant> TokenCursorKind for TokenIterator<RawToken<V>> {
    fn kind(&self) -> usize { self.get().map_or(crate::recovery::END_OF_INPUT, |token| token.id) }
}
impl<V: TokenVariant> TokenCursorKind for LazyIterator<RawToken<V>> {
    fn kind(&self) -> usize { unimplemented!("lazy lexing") }
}

pub trait LLParserApi { type Iterator; }

/// Base of generated parsers.
#[repr(C)]
pub struct LLParser<Main, Token> {
    pub lexer: Shared<LexerBase<Token>>,
    pub text: Ptr,
    pub tree: Main,
    pub errors: Vec<Error>,
    pub __vt_parseFromTokens: fn(&mut LLParser<Main, Token>),
    pub __vt_lazyParse: fn(&mut LLParser<Main, Token>) -> MatchResult<Main, LazyIterator<Token>>,
}
