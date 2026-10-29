//! Typed cursors and tokens crossing into the lowered core. The core's cursors, tokens and nodes are dynamic; the
//! facade subclasses them (fn-pointer slots, see tests/subclass.rs) so a typed iterator is a core `TokenCursor`, a
//! typed token is a core `RawToken`, and a typed node travels through the core inside a core `Node`'s `Any` payload.
use crate::python::ispa_std::{lexer::TokenCursor, node::Node as CoreNode, raw_token::{RawToken as CoreToken, RawTokenMeta}};
use crate::pycoll::to_any;
use crate::{LazyIterator, Node, PyAny, RawToken, Shared, TokenIterator, TokenVariant, Upcast, VariantGet, WeakShared};
use std::marker::PhantomData;

/// A typed token as the core sees it.
pub trait BridgeToken: Clone + 'static {
    /// Position of the held alternative (the id the core addresses token types by).
    fn alt_index(&self) -> i64;
    /// Source span: (start, text, has_source, length, line, column, kind).
    fn span(&self) -> (i64, String, bool, i64, i64, i64, i64);
    /// The typed node of a `T` the token holds.
    fn node<T: Clone + Default + 'static>(&self) -> Option<Shared<CoreNode>>;
    /// The alternative index of `T`, if the token type has such an alternative.
    fn alt_of<T: 'static>() -> Option<i64>;
}
impl<V: TokenVariant> BridgeToken for RawToken<V> {
    fn alt_index(&self) -> i64 { self.index() as i64 }
    fn span(&self) -> (i64, String, bool, i64, i64, i64, i64) {
        (self.start_pos, self.text(), true, self.length, self.line, self.column(), self.id as i64)
    }
    fn node<T: Clone + Default + 'static>(&self) -> Option<Shared<CoreNode>> {
        if self.value.held().is::<T>() { Some(core_node(self.node_of::<T>())) } else { None }
    }
    fn alt_of<T: 'static>() -> Option<i64> { V::index_of::<T>().map(|index| index as i64) }
}

/// A typed iterator the core can walk.
pub trait BridgeCursor: Clone + 'static {
    type Token: BridgeToken;
    fn is_end(&self) -> bool;
    fn advance(&mut self);
    fn retreat(&mut self);
    fn distance(&self) -> i64;
    fn current(&self) -> Option<Self::Token>;
    /// The source text this cursor lexes from, when it knows it (lazy cursors do).
    fn source_origin(&self) -> Option<&'static str> { None }
}
impl<V: TokenVariant> BridgeCursor for TokenIterator<RawToken<V>> {
    type Token = RawToken<V>;
    // the empty token that closes the stream is the end
    fn is_end(&self) -> bool { self.get().map_or(true, |token| token.empty()) }
    fn advance(&mut self) { TokenIterator::advance(self) }
    fn retreat(&mut self) { TokenIterator::retreat(self) }
    fn distance(&self) -> i64 { TokenIterator::distance(self) as i64 }
    fn current(&self) -> Option<RawToken<V>> { self.get().cloned() }
}
impl<V: TokenVariant> BridgeCursor for LazyIterator<RawToken<V>> {
    type Token = RawToken<V>;
    fn is_end(&self) -> bool { self.at_end() }
    fn advance(&mut self) { LazyIterator::advance(self) }
    fn retreat(&mut self) { LazyIterator::retreat(self) }
    fn distance(&self) -> i64 { LazyIterator::distance(self) as i64 }
    fn current(&self) -> Option<RawToken<V>> { self.token() }
    fn source_origin(&self) -> Option<&'static str> { self.origin() }
}

/// A typed node as the core's `Node`: the core carries it, only the facade looks inside.
pub fn core_node<T: Clone + 'static>(node: Node<T>) -> Shared<CoreNode> {
    let carrier = CoreNode::shared_default();
    let (start, length, line, column, name) = (node.start_pos, node.length, node.line, node.column, node.name);
    carrier.borrow_mut().__init__(start, &String::new(), false, length, line, column, name, false, to_any(node));
    carrier
}
/// The typed node a core `Node` carries (an empty node when it carries none, or another type).
pub fn typed_node<T: Clone + Default + 'static>(node: &Shared<CoreNode>) -> Node<T> {
    if node.is_null() { return Node::default(); }
    node.borrow()._data.downcast_ref::<Node<T>>().cloned().unwrap_or_default()
}

/// The token at a cursor, as the core's `RawToken`; `to_node` makes the typed node of the cursor's target type.
#[repr(C)]
pub struct TypedRawToken {
    base_0: CoreToken,
    node: PyAny,
}
impl Upcast<CoreToken> for TypedRawToken {
    unsafe fn up_ptr(this: *mut Self) -> *mut CoreToken { std::ptr::addr_of_mut!((*this).base_0) }
}
impl TypedRawToken {
    fn this(base: &mut CoreToken) -> &mut TypedRawToken { unsafe { &mut *(base as *mut CoreToken as *mut TypedRawToken) } }

    fn handle(meta: Shared<RawTokenMeta>, type_id: i64, node: PyAny) -> Shared<CoreToken> {
        Shared::new_with(|owner: &WeakShared<TypedRawToken>| {
            let mut token = TypedRawToken { base_0: CoreToken::default(), node };
            token.base_0.__bind_owner(&owner.upcast());
            token.base_0.__init__(meta, type_id);
            token.base_0.__vt_to_node = |base, _| {
                let this = TypedRawToken::this(base);
                match this.node.downcast_ref::<CarriedNode>() {
                    Some(carried) => carried.0.clone(),
                    None => CoreNode::blank(),
                }
            };
            token
        }).upcast()
    }
}
/// Token metadata whose text is the token's own, not a slice of a source string the core would have to hold.
#[repr(C)]
struct TypedMeta {
    base_0: RawTokenMeta,
    text: String,
}
impl Upcast<RawTokenMeta> for TypedMeta {
    unsafe fn up_ptr(this: *mut Self) -> *mut RawTokenMeta { std::ptr::addr_of_mut!((*this).base_0) }
}
impl TypedMeta {
    fn this(base: &mut RawTokenMeta) -> &mut TypedMeta { unsafe { &mut *(base as *mut RawTokenMeta as *mut TypedMeta) } }

    fn handle(start: i64, text: String, length: i64, line: i64, column: i64, kind: i64) -> Shared<RawTokenMeta> {
        Shared::new_with(|owner: &WeakShared<TypedMeta>| {
            let mut meta = TypedMeta { base_0: RawTokenMeta::default(), text };
            meta.base_0.__bind_owner(&owner.upcast());
            meta.base_0.__init__(start, &String::new(), false, length, line, column, kind);
            meta.base_0.__vt_has_source = |_| true;
            meta.base_0.__vt_text = |base| TypedMeta::this(base).text.clone();
            meta
        }).upcast()
    }
}

/// A node already wrapped for the core, kept as the token's payload.
#[derive(Clone)]
struct CarriedNode(Shared<CoreNode>);

/// How a cursor makes the typed node of the type it was bridged for.
pub type MakeNode<Token> = fn(&Token) -> Option<Shared<CoreNode>>;

/// Make the typed nodes of `T` out of tokens.
pub fn node_maker<Token: BridgeToken, T: Clone + Default + 'static>() -> MakeNode<Token> { |token| token.node::<T>() }

/// A core `TokenCursor` over a typed iterator.
#[repr(C)]
pub struct TypedCursor<IT: BridgeCursor> {
    base_0: TokenCursor,
    pub it: IT,
    make_node: Option<MakeNode<IT::Token>>,
    cached: Option<(i64, Shared<CoreToken>)>,
    _target: PhantomData<IT>,
}
impl<IT: BridgeCursor> Upcast<TokenCursor> for TypedCursor<IT> {
    unsafe fn up_ptr(this: *mut Self) -> *mut TokenCursor { std::ptr::addr_of_mut!((*this).base_0) }
}
impl<IT: BridgeCursor> TypedCursor<IT> {
    fn this(base: &mut TokenCursor) -> &mut Self { unsafe { &mut *(base as *mut TokenCursor as *mut Self) } }

    /// A handle to a cursor at `it`; `make_node` makes the typed node of the type being matched, if it is a token's.
    pub fn handle(it: IT, make_node: Option<MakeNode<IT::Token>>) -> Shared<TypedCursor<IT>> {
        Shared::new_with(|owner: &WeakShared<TypedCursor<IT>>| {
            let mut cursor = TypedCursor { base_0: TokenCursor::default(), it, make_node, cached: None, _target: PhantomData };
            cursor.base_0.__bind_owner(&owner.upcast());
            cursor.install();
            cursor
        })
    }

    fn me(this: &mut Self) -> Shared<TokenCursor> { this.base_0.__ispa_owner.upgrade().expect("cursor handle") }

    fn install(&mut self) {
        let base = &mut self.base_0;
        base.__vt_isEnd = |base| Self::this(base).it.is_end();
        base.__vt_has_source = |base| Self::this(base).it.source_origin().is_some();
        base.__vt_sourceOrigin = |base| Self::this(base).it.source_origin().unwrap_or_default().to_string();
        base.__vt_getErrors = |_| Vec::new();
        base.__vt_sourcePosition = |_| -1;
        base.__vt_consumeExternal = |_, _| {};
        base.__vt_resumePosition = |_| -1;
        base.__vt_advanceCharacters = |_, _| false;
        base.__vt_distance = |base| Self::this(base).it.distance();
        base.__vt_copy = |base| {
            let this = Self::this(base);
            let copy = Self::handle(this.it.clone(), this.make_node);
            copy.borrow_mut().cached = this.cached.clone();
            copy.upcast()
        };
        base.__vt_assign = |base, other| {
            let this = Self::this(base);
            let mut other = other.borrow_mut();
            let other = Self::this(&mut other);
            this.it = other.it.clone();
            this.cached = other.cached.clone();
        };
        base.__vt_next = |base| { let this = Self::this(base); if !this.it.is_end() { this.it.advance(); } Self::me(this) };
        base.__vt_prev = |base| { let this = Self::this(base); this.it.retreat(); Self::me(this) };
        base.__vt_advance_by = |base, count| {
            let this = Self::this(base);
            for _ in 0..count { if this.it.is_end() { break; } this.it.advance(); }
            Self::me(this)
        };
        base.__vt_plus = |base, count| {
            let this = Self::this(base);
            let moved = Self::handle(this.it.clone(), this.make_node);
            moved.borrow_mut().base_0.advance_by(count);
            moved.upcast()
        };
        base.__vt_minus = |base, other| {
            let this = Self::this(base);
            let mut other = other.borrow_mut();
            this.it.distance() - Self::this(&mut other).it.distance()
        };
        base.__vt_current = |base| {
            let this = Self::this(base);
            let distance = this.it.distance();
            if let Some((at, token)) = &this.cached { if *at == distance { return token.clone(); } }
            let Some(token) = this.it.current() else { return CoreToken::empty_token() };
            let (start, text, has_source, length, line, column, kind) = token.span();
            let _ = has_source;
            let meta = TypedMeta::handle(start, text, length, line, column, kind);
            let node = match this.make_node.and_then(|make| make(&token)) {
                Some(node) => to_any(CarriedNode(node)),
                None => PyAny::default(),
            };
            let raw = TypedRawToken::handle(meta, token.alt_index(), node);
            this.cached = Some((distance, raw.clone()));
            raw
        };
    }
}
