//! Running a lexer's DFA: the generated tables are handed to the lowered interpreter
//! (`python::ispa_std::dfa_run::run`); the typed token a semantic action builds stays on this side.
use crate::dfa::api::{Captures, CharToClass, TdfaLayout, NULL_STATE};
use crate::python::ispa_std::{dfa_api, dfa_run, raw_token::RawToken as CoreToken, refs, utility::Pair};
use crate::{LexerBase, Ptr, Shared};
use std::cell::RefCell;
use std::collections::HashMap;

thread_local! {
    static TABLES: RefCell<HashMap<usize, Shared<dfa_run::DfaTables>>> = Default::default();
    static SOURCE: RefCell<Option<(usize, usize, Shared<dfa_api::SourceText>)>> = Default::default();
}

fn core_tables(key: usize, table: &[Vec<i64>], classes: &CharToClass, actions: &[Vec<i64>], registers: usize) -> Shared<dfa_run::DfaTables> {
    if let Some(found) = TABLES.with(|tables| tables.borrow().get(&key).cloned()) { return found; }
    let layout = dfa_api::TdfaLayout::shared_default();
    layout.borrow_mut().__init__(registers as i64, 0);
    let index = dfa_api::DebugIndex::shared_default();
    index.borrow_mut().__init__();
    let made = dfa_run::DfaTables::shared_default();
    made.borrow_mut().__init__(&table.to_vec(), classes, &actions.to_vec(), layout, &Vec::new(), index);
    TABLES.with(|tables| tables.borrow_mut().insert(key, made.clone()));
    made
}

fn core_source(text: &'static str) -> Shared<dfa_api::SourceText> {
    let key = (text.as_ptr() as usize, text.len());
    if let Some(found) = SOURCE.with(|source| source.borrow().as_ref().filter(|(ptr, len, _)| (*ptr, *len) == key).map(|(_, _, source)| source.clone())) {
        return found;
    }
    let source = dfa_api::SourceText::shared_default();
    source.borrow_mut().__init__(&text.to_string());
    SOURCE.with(|cache| *cache.borrow_mut() = Some((key.0, key.1, source.clone())));
    source
}

impl<Token: Clone + Default + 'static> LexerBase<Token> {
    /// Scan one token at `pos` with a generated DFA; `pos` ends where the scan stopped. The semantic action gets
    /// (state, captures, start offset, start, length, line, cursor) and returns the next state and the token.
    pub fn lookup<const S: usize, const A: usize, const R: usize, const O: usize>(
        &mut self, table: &[Vec<i64>; S], classes: &CharToClass, actions: &[Vec<i64>; A], _layout: TdfaLayout<R, O>,
        mut semantic: impl FnMut(i64, &Captures, i64, Ptr, i64, i64, &mut Ptr) -> (i64, Token), pos: &mut Ptr,
    ) -> Token {
        let text = pos.text();
        if pos.is_eof() { return Token::default(); }
        let tables = core_tables(table.as_ptr() as usize, table, classes, actions, R);
        let source = core_source(text);
        let cursor = refs::Cursor::shared_default();
        cursor.borrow_mut().__init__(pos.offset() as i64, self.line);
        let carrier = CoreToken::empty_token();
        let mut accepted: Option<Token> = None;
        {
            let adapter = |index: i64, captures: Shared<dfa_api::Captures>, first: i64, length: i64, line: i64, cur: Shared<refs::Cursor>| {
                let typed = Captures { inner: captures };
                let start = Ptr::at(text, first.max(0) as usize);
                let mut at = Ptr::at(text, cur.borrow().pos.max(0) as usize);
                let (next, token) = semantic(index, &typed, first, start, length, line, &mut at);
                cur.borrow_mut().pos = at.offset() as i64;
                if next == NULL_STATE { accepted = Some(token); }
                let pair = Pair::<i64, Shared<CoreToken>>::shared_default();
                pair.borrow_mut().__init__(next, carrier.clone());
                pair
            };
            dfa_run::run(source, cursor.clone(), pos.offset() as i64, tables, adapter, carrier.clone());
        }
        *pos = Ptr::at(text, cursor.borrow().pos.max(0) as usize);
        self.line = cursor.borrow().line;
        accepted.unwrap_or_default()
    }
}
