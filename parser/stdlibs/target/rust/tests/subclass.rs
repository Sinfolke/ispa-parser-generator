//! Hand-written Rust subclasses a lowered class (a token cursor over plain kinds) and the lowered LL parser drives it:
//! the mechanism the typed facade is built on (fn-pointer slots filled by thunks, `repr(C)` base at offset 0).
use ispa_std::python::ispa_std::{lexer::TokenCursor, parser_ll, raw_token, recovery};
use ispa_std::{Shared, Upcast, WeakShared};
use std::rc::Rc;

#[repr(C)]
#[derive(Default)]
struct KindCursor {
    base_0: TokenCursor,
    kinds: Rc<Vec<i64>>,
    index: usize,
}

impl Upcast<TokenCursor> for KindCursor {
    unsafe fn up_ptr(this: *mut Self) -> *mut TokenCursor { std::ptr::addr_of_mut!((*this).base_0) }
}

impl KindCursor {
    fn this(base: &mut TokenCursor) -> &mut KindCursor { unsafe { &mut *(base as *mut TokenCursor as *mut KindCursor) } }

    fn handle(kinds: Rc<Vec<i64>>, index: usize) -> Shared<KindCursor> {
        Shared::new_with(|owner: &WeakShared<KindCursor>| {
            let mut cursor = KindCursor { base_0: TokenCursor::default(), kinds, index };
            cursor.base_0.__bind_owner(&owner.upcast());
            cursor.base_0.__vt_isEnd = |base| { let this = KindCursor::this(base); this.index >= this.kinds.len() };
            cursor.base_0.__vt_copy = |base| { let this = KindCursor::this(base); KindCursor::handle(this.kinds.clone(), this.index).upcast() };
            cursor.base_0.__vt_next = |base| {
                let this = KindCursor::this(base);
                if this.index < this.kinds.len() { this.index += 1; }
                this.base_0.__ispa_owner.upgrade().expect("owned")
            };
            cursor.base_0.__vt_current = |base| {
                let this = KindCursor::this(base);
                match this.kinds.get(this.index) {
                    Some(&kind) => {
                        let token = raw_token::RawToken::shared_default();
                        token.borrow_mut().__init__(raw_token::RawTokenMeta::blank(), kind);
                        token
                    }
                    None => raw_token::RawToken::empty_token(),
                }
            };
            cursor
        })
    }
}

#[test]
fn lowered_parser_consumes_a_token_from_a_hand_written_cursor() {
    let cursor: Shared<TokenCursor> = KindCursor::handle(Rc::new(vec![7, 8]), 0).upcast();
    assert!(!cursor.borrow_mut().isEnd());
    assert_eq!(cursor.borrow_mut().current().borrow_mut().index(), 7);
    let copy = cursor.borrow_mut().copy();
    cursor.borrow_mut().next();
    assert_eq!(cursor.borrow_mut().current().borrow_mut().index(), 8);
    assert_eq!(copy.borrow_mut().current().borrow_mut().index(), 7, "copy keeps its own position");
    cursor.borrow_mut().next();
    assert!(cursor.borrow_mut().isEnd());
}

#[test]
fn lowered_parser_requires_a_token_from_a_hand_written_cursor() {
    let cursor: Shared<TokenCursor> = KindCursor::handle(Rc::new(vec![7, 8]), 0).upcast();
    let parser = parser_ll::LLParser::shared_default();
    parser.borrow_mut().__init__();
    let handler = recovery::FailHandler::shared_default();
    handler.borrow_mut().__init__();
    let node = parser.borrow_mut().required_token(cursor.clone(), 7, handler);
    assert!(!node.is_null());
    assert_eq!(cursor.borrow_mut().current().borrow_mut().index(), 8, "the matched token was consumed");
}
