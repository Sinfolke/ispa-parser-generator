//! The combinators generated parsers call (`LLParserApi`), written the way the generated code writes them.
use ispa_std::recovery;
use ispa_std::{LLParser, LLParserApi, MatchResult, Node, Ptr, RawToken, TokenIterator, TokenVariant};
use std::ops::{Deref, DerefMut};
use std::rc::Rc;

#[derive(Clone, Default, Debug, PartialEq)]
struct Num(i64);
#[derive(Clone, Default, Debug, PartialEq)]
struct Word(String);
#[derive(Clone, Default, Debug, PartialEq)]
struct Pair { num: Num, word: Word }
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

type Token = RawToken<V>;
type It = TokenIterator<Token>;

#[derive(Default, Clone)]
struct Parser { base_0: LLParser<Pair, Token> }
impl Deref for Parser { type Target = LLParser<Pair, Token>; fn deref(&self) -> &Self::Target { &self.base_0 } }
impl DerefMut for Parser { fn deref_mut(&mut self) -> &mut Self::Target { &mut self.base_0 } }

fn handler(rule: &str) -> recovery::CoreFail {
    recovery::handler(&[1], &[], &[], &[], false, true, false, 32, rule, rule)
}

impl Parser {
    fn pair(&mut self, mut pos: It) -> MatchResult<Pair, It> {
        let _scope = self.recovery_frame("pair");
        let num = LLParserApi::required::<Num>(self, &mut pos, handler("num"));
        let word = LLParserApi::required::<Word>(self, &mut pos, handler("word"));
        let mut result = MatchResult::<Pair, It>::default();
        result.status = true;
        result.node.set_data(Pair { num: num.data().clone(), word: word.data().clone() });
        result.it = pos;
        result
    }
    fn pairs(&mut self, mut pos: It) -> MatchResult<Vec<Pair>, It> {
        let nodes = LLParserApi::range_optional_rule::<Pair>(self, &mut pos, |p, cursor| p.pair(cursor));
        let mut result = MatchResult::<Vec<Pair>, It>::default();
        result.status = true;
        result.node.set_data(nodes.iter().map(|node| node.data().clone()).collect());
        result.it = pos;
        result
    }
}

fn tokens() -> Vec<Token> {
    let source = Ptr::new("1 a 2 b 3");
    vec![
        Token::create(0, source, 1, 1, 1, Num(1)), Token::create(2, source.advance(2), 1, 1, 2, Word("a".into())),
        Token::create(4, source.advance(4), 1, 1, 1, Num(2)), Token::create(6, source.advance(6), 1, 1, 2, Word("b".into())),
    ]
}

#[test]
fn rules_and_tokens_compose_through_the_core() {
    let mut parser = Parser::default();
    let result = parser.pairs(TokenIterator::new(Rc::new(tokens()), 0));
    assert!(result.status);
    let pairs = result.node.data().clone();
    assert_eq!(pairs, vec![Pair { num: Num(1), word: Word("a".into()) }, Pair { num: Num(2), word: Word("b".into()) }]);
    assert_eq!(result.it.distance(), 4, "all tokens consumed");
}
