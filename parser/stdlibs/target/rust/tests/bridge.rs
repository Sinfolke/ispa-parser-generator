//! A typed token stream walked by the lowered LL parser: typed nodes go through the core and come back typed.
use ispa_std::python::ispa_std::{parser_ll, recovery};
use ispa_std::{node_maker, typed_node, Ptr, RawToken, Shared, TokenIterator, TokenVariant, TypedCursor, VariantGet};
use std::rc::Rc;

#[derive(Clone, Default, Debug, PartialEq)]
struct Num(i64);
#[derive(Clone, Default, Debug, PartialEq)]
struct Word(String);
#[derive(Clone, Default)]
enum V { #[default] Empty, N(Num), W(Word) }

impl TokenVariant for V {
    fn variant_index(&self) -> usize { match self { V::Empty => 0, V::N(_) => 1, V::W(_) => 2 } }
    fn held(&self) -> &dyn std::any::Any { match self { V::Empty => &(), V::N(x) => x, V::W(x) => x } }
    fn index_of<T: 'static>() -> Option<usize> {
        let id = std::any::TypeId::of::<T>();
        if id == std::any::TypeId::of::<Num>() { Some(1) } else if id == std::any::TypeId::of::<Word>() { Some(2) } else { None }
    }
}
impl From<Num> for V { fn from(v: Num) -> Self { V::N(v) } }
impl From<Word> for V { fn from(v: Word) -> Self { V::W(v) } }
impl VariantGet<Num> for V {
    fn holds(&self) -> bool { matches!(self, V::N(_)) }
    fn get(&self) -> &Num { match self { V::N(x) => x, _ => ispa_std::bad_variant_access() } }
    fn get_mut(&mut self) -> &mut Num { match self { V::N(x) => x, _ => ispa_std::bad_variant_access() } }
}
impl VariantGet<Word> for V {
    fn holds(&self) -> bool { matches!(self, V::W(_)) }
    fn get(&self) -> &Word { match self { V::W(x) => x, _ => ispa_std::bad_variant_access() } }
    fn get_mut(&mut self) -> &mut Word { match self { V::W(x) => x, _ => ispa_std::bad_variant_access() } }
}

type Token = RawToken<V>;

fn cursor<T: Clone + Default + 'static>(tokens: Vec<Token>) -> Shared<ispa_std::python::ispa_std::lexer::TokenCursor> {
    TypedCursor::handle(TokenIterator::new(Rc::new(tokens), 0), Some(node_maker::<Token, T>())).upcast()
}

#[test]
fn a_typed_token_comes_back_as_a_typed_node() {
    let source = Ptr::new("42 abc");
    let tokens = vec![Token::create(0, source, 2, 1, 5, Num(42)), Token::create(3, source.advance(3), 3, 1, 6, Word("abc".into()))];
    let at = cursor::<Num>(tokens);
    let parser = parser_ll::LLParser::shared_default();
    parser.borrow_mut().__init__();
    let handler = recovery::FailHandler::shared_default();
    handler.borrow_mut().__init__();
    let node = parser.borrow_mut().required_token(at.clone(), V::index_of::<Num>().unwrap() as i64, handler);
    let typed = typed_node::<Num>(&node);
    assert_eq!(typed.data(), &Num(42));
    assert_eq!((typed.start_pos, typed.length), (0, 2));
    assert_eq!(at.borrow_mut().current().borrow_mut().index(), 2, "the number was consumed");
}
