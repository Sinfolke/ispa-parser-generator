#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals, unused_mut, unused_variables, unused_parens,
         unused_labels, unused_imports, unreachable_patterns, unreachable_code, dead_code, unused_braces,
         invalid_reference_casting, clippy::all)]
//! Runtime primitives used by code emitted from ISPA's Rust backend.
//!
//! The crate intentionally has no third party dependencies so generated parsers
//! can use it from an offline Cargo build.

extern crate self as ispa_std;

use std::fmt;
use std::marker::PhantomData;
use std::sync::OnceLock;
use std::cell::RefCell;
use std::rc::Rc;

mod shared;
pub use shared::{Shared, WeakShared, Upcast, Ref, RefMut};
pub mod pycoll;
pub use pycoll::{PyAny, PyVal, PyKey};
mod primitives;
pub use primitives::{StdError, fail, attempt, failure_message};

/// Lazily built namespace-level variable. The runtime is single-threaded (objects are `Rc`), so `get` hands out the
/// place mutably: lowered Python mutates module state through it.
pub struct Lazy<T> { value: std::cell::UnsafeCell<Option<T>>, init: fn() -> T }
unsafe impl<T> Sync for Lazy<T> {}
impl<T> Lazy<T> {
    pub const fn new(init: fn() -> T) -> Self { Self { value: std::cell::UnsafeCell::new(None), init } }
    #[allow(clippy::mut_from_ref)]
    pub fn get(&self) -> &mut T {
        // SAFETY: single-threaded use only; callers never hold the place across a call that re-enters `get`.
        let slot = unsafe { &mut *self.value.get() };
        if slot.is_none() { *slot = Some((self.init)()); }
        slot.as_mut().unwrap()
    }
}
impl<T> std::ops::Deref for Lazy<T> {
    type Target = T;
    fn deref(&self) -> &T { self.get() }
}

/// A small, copyable cursor into UTF-8 source text. DFA input is classified one byte at a time.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Ord, PartialOrd)]
pub struct Ptr {
    input: &'static str,
    offset: usize,
}

impl Ptr {
    pub fn new(input: &'static str) -> Self { Self { input, offset: 0 } }
    pub fn from_static(input: &'static str) -> Self { Self::new(input) }
    pub fn offset(self) -> usize { self.offset }
    pub fn is_eof(self) -> bool { self.offset >= self.input.len() }
    pub fn get(self) -> Option<u8> { self.input.as_bytes().get(self.offset).copied() }
    pub fn inc(&mut self) -> Self {
        if self.get().is_some() { self.offset += 1; }
        *self
    }
    pub fn advance(mut self, count: usize) -> Self { for _ in 0..count { self.inc(); } self }
    pub fn offset_by(self, count: i64) -> Self {
        Self { offset: self.offset.saturating_add_signed(count as isize).min(self.input.len()), ..self }
    }
    pub fn starts_with(self, text: &str) -> bool { self.input.get(self.offset..).is_some_and(|s| s.starts_with(text)) }
    pub fn remaining(self) -> &'static str { self.input.get(self.offset..).unwrap_or("") }
    pub fn line(self) -> usize { self.input.as_bytes()[..self.offset.min(self.input.len())].iter().filter(|b| **b == b'\n').count() + 1 }
    pub fn column(self) -> usize {
        self.input.as_bytes()[..self.offset.min(self.input.len())].iter().rev()
            .take_while(|b| **b != b'\n').count() + 1
    }
}

impl From<&'static str> for Ptr { fn from(value: &'static str) -> Self { Self::new(value) } }
impl std::ops::Add<&i64> for Ptr {
    type Output = Self;
    fn add(self, count: &i64) -> Self { self.offset_by(*count) }
}
impl std::ops::Add<i64> for Ptr {
    type Output = Self;
    fn add(self, count: i64) -> Self { self.offset_by(count) }
}
pub fn string_range(start: Ptr, end: Ptr) -> String {
    assert!(std::ptr::eq(start.input, end.input) && start.offset <= end.offset);
    String::from_utf8_lossy(&start.input.as_bytes()[start.offset..end.offset]).into_owned()
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Span<'a, T> { values: &'a [T] }
impl<'a, T> Span<'a, T> {
    pub fn from_slice(values: &'a [T]) -> Self { Self { values } }
    pub fn as_slice(self) -> &'a [T] { self.values }
    pub fn len(self) -> usize { self.values.len() }
    pub fn is_empty(self) -> bool { self.values.is_empty() }
}
impl<'a, T> IntoIterator for Span<'a, T> {
    type Item = &'a T;
    type IntoIter = std::slice::Iter<'a, T>;
    fn into_iter(self) -> Self::IntoIter { self.values.iter() }
}

#[derive(Clone, Debug, Default, PartialEq)]
pub struct Node<T> { pub value: T, pub start: usize, pub end: usize }
impl<T: Default> Node<T> {
    pub fn create(value: T) -> Self { Self { value, start: 0, end: 0 } }
    pub fn set_data(&mut self, value: T) { self.value = value; }
    pub fn data(&self) -> &T { &self.value }
}

#[derive(Clone, Debug, Default, PartialEq)]
pub struct RawToken<T = ()> { pub value: Option<T>, pub start: usize, pub end: usize, pub kind: usize }
impl<T> RawToken<T> {
    pub fn new(value: T, start: usize, end: usize) -> Self { Self { value: Some(value), start, end, kind: 0 } }
    pub fn create<V: Into<T>>(start: i64, _source: Ptr, length: i64, _line: i64, kind: usize, value: V) -> Self {
        let start = start.max(0) as usize;
        Self { value: Some(value.into()), start, end: start.saturating_add(length.max(0) as usize), kind }
    }
    pub fn empty(&self) -> bool { self.value.is_none() }
}

#[derive(Clone, Debug, PartialEq)]
pub struct MatchResult<T, I = usize> { pub status: bool, pub node: Node<T>, pub it: I }
impl<T: Default, I: Default> Default for MatchResult<T, I> {
    fn default() -> Self { Self { status: false, node: Node::create(T::default()), it: I::default() } }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Error { message: String }
impl Error { pub fn new(message: impl Into<String>) -> Self { Self { message: message.into() } } }
impl fmt::Display for Error { fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { f.write_str(&self.message) } }
impl std::error::Error for Error {}

pub fn starts_with(pos: Ptr, value: &str) -> bool { pos.starts_with(value) }
pub fn skip_spaces<T>(pos: &mut Ptr) {
    let _ = std::marker::PhantomData::<T>;
    while let Some(ch) = pos.get() { if !ch.is_ascii_whitespace() { break; } pos.inc(); }
}
pub fn concat<T: AsRef<str>>(values: &[T]) -> String { values.iter().map(AsRef::as_ref).collect() }

pub trait VariantGet<T> { fn get(&self) -> &T; fn get_mut(&mut self) -> &mut T; fn holds(&self) -> bool { true } }
pub fn bad_variant_access() -> ! { panic!("ISPA variant access failed") }

/// Conversion hook used where the language neutral IR permits compatible values.
pub fn assign_compatible<T: Clone>(target: &mut T, source: &T) { *target = source.clone(); }

/// A lazy lexer. The callback must advance the cursor for every token it returns.
#[derive(Clone)]
pub struct LexerBase<T> {
    cursor: Ptr,
    finished: bool,
    scan: Rc<RefCell<dyn FnMut(&mut Ptr) -> Result<Option<T>, Error>>>,
}
impl<T> Default for LexerBase<T> {
    fn default() -> Self { Self::new("", |_| Ok(None)) }
}
impl<T> LexerBase<T> {
    pub fn new(input: &'static str, scan: impl FnMut(&mut Ptr) -> Result<Option<T>, Error> + 'static) -> Self {
        Self { cursor: Ptr::new(input), finished: false, scan: Rc::new(RefCell::new(scan)) }
    }
    pub fn cursor(&self) -> Ptr { self.cursor }
    pub fn next_token(&mut self) -> Result<Option<T>, Error> {
        if self.finished { return Ok(None); }
        if self.cursor.is_eof() { self.finished = true; return Ok(None); }
        let before = self.cursor;
        let token = (self.scan.borrow_mut())(&mut self.cursor)?;
        if token.is_some() && self.cursor.offset() == before.offset() {
            return Err(Error::new(format!("lexer did not advance at line {}, column {}", before.line(), before.column())));
        }
        if token.is_none() { self.finished = true; }
        Ok(token)
    }
    pub fn collect_tokens(mut self) -> Result<Vec<T>, Error> {
        let mut tokens = Vec::new();
        while let Some(token) = self.next_token()? { tokens.push(token); }
        Ok(tokens)
    }
}
impl<T> Iterator for LexerBase<T> {
    type Item = Result<T, Error>;
    fn next(&mut self) -> Option<Self::Item> {
        match self.next_token() { Ok(Some(token)) => Some(Ok(token)), Ok(None) => None, Err(error) => { self.finished = true; Some(Err(error)) } }
    }
}
pub trait LexerTypes { type Token; type Iterator; }
/// Cursor operations required by generated parser entry points.
pub trait TokenStream {
    type Cursor;
    type LazyCursor;

    fn token_cursor(&mut self) -> Self::Cursor;
    fn lazy_token_cursor(&mut self, input: Ptr) -> Self::LazyCursor;
    fn reset_token_stream(&mut self);
}
/// Token stream with transactional rule calls and useful failure positions.
#[derive(Clone)]
pub struct LLParser<AST, T> { tokens: Vec<T>, position: usize, errors: Vec<Error>, ast: PhantomData<AST> }
impl<AST, T> Default for LLParser<AST, T> {
    fn default() -> Self { Self { tokens: Vec::new(), position: 0, errors: Vec::new(), ast: PhantomData } }
}
impl<AST, T> LLParser<AST, T> {
    pub fn new(tokens: Vec<T>) -> Self { Self { tokens, position: 0, errors: Vec::new(), ast: PhantomData } }
    pub fn errors(&self) -> &[Error] { &self.errors }
    pub fn position(&self) -> usize { self.position }
    pub fn is_eof(&self) -> bool { self.position == self.tokens.len() }
    pub fn peek(&self) -> Option<&T> { self.tokens.get(self.position) }
    pub fn checkpoint(&self) -> usize { self.position }
    pub fn restore(&mut self, checkpoint: usize) { assert!(checkpoint <= self.tokens.len()); self.position = checkpoint; }
    pub fn take_if(&mut self, predicate: impl FnOnce(&T) -> bool) -> Option<&T> {
        if self.peek().is_some_and(predicate) { self.position += 1; self.tokens.get(self.position - 1) } else { None }
    }
    pub fn required<R>(&mut self, expected: &str, rule: impl FnOnce(&mut Self) -> Result<Option<R>, Error>) -> Result<R, Error> {
        let start = self.checkpoint();
        match rule(self) {
            Ok(Some(value)) => Ok(value),
            Ok(None) => { self.restore(start); Err(Error::new(format!("expected {expected} at token {start}"))) },
            Err(error) => { self.restore(start); Err(error) },
        }
    }
    pub fn optional<R>(&mut self, rule: impl FnOnce(&mut Self) -> Result<Option<R>, Error>) -> Result<Option<R>, Error> {
        let start = self.checkpoint();
        match rule(self) { Ok(Some(value)) => Ok(Some(value)), Ok(None) => { self.restore(start); Ok(None) }, Err(error) => { self.restore(start); Err(error) } }
    }
    pub fn repeated<R>(&mut self, minimum: usize, maximum: Option<usize>, mut rule: impl FnMut(&mut Self) -> Result<Option<R>, Error>) -> Result<Vec<R>, Error> {
        let origin = self.checkpoint();
        let mut values = Vec::new();
        while maximum.map_or(true, |limit| values.len() < limit) {
            let before = self.checkpoint();
            match rule(self) {
                Ok(Some(value)) => {
                    if self.position == before { self.restore(origin); return Err(Error::new("repeated rule did not advance")); }
                    values.push(value);
                }
                Ok(None) => { self.restore(before); break; }
                Err(error) => { self.restore(origin); return Err(error); }
            }
        }
        if values.len() < minimum { self.restore(origin); Err(Error::new(format!("expected at least {minimum} repetitions at token {origin}"))) } else { Ok(values) }
    }
    /// Record a diagnostic, then skip tokens until a synchronization token or EOF.
    /// The synchronization token remains available to the caller.
    pub fn recover_until(&mut self, error: Error, synchronize: impl Fn(&T) -> bool) -> usize {
        self.errors.push(error);
        let start = self.position;
        while let Some(token) = self.peek() {
            if synchronize(token) { break; }
            self.position += 1;
        }
        self.position - start
    }
}
pub trait LLParserApi { type Iterator; }
pub struct LRParser<T>(PhantomData<T>);
pub struct LRGeneralizedParser<T>(PhantomData<T>);
pub struct ASTPrinter<'a> { out: &'a mut dyn fmt::Write, branches: Vec<bool> }
impl<'a> ASTPrinter<'a> {
    pub fn new(out: &'a mut dyn fmt::Write) -> Self { Self { out, branches: Vec::new() } }
    pub fn up(&mut self, has_more_siblings: bool) { self.branches.push(has_more_siblings); }
    pub fn down(&mut self) { self.branches.pop(); }
    fn prefix(&mut self, last: bool) {
        for branch in self.branches.iter().take(self.branches.len().saturating_sub(1)) {
            let _ = self.out.write_str(if *branch { "│   " } else { "    " });
        }
        if !self.branches.is_empty() { let _ = self.out.write_str(if last { "└── " } else { "├── " }); }
    }
    pub fn node(&mut self, name: &str, last: bool) { self.prefix(last); let _ = writeln!(self.out, "{name}"); }
    pub fn node_with_value<T: fmt::Debug>(&mut self, name: &str, value: &T, last: bool) {
        self.prefix(last);
        let _ = writeln!(self.out, "{name}: {value:?}");
    }
    pub fn subtree<T: fmt::Debug>(&mut self, name: &str, value: &T, last: bool) {
        self.node(name, last);
        self.up(!last);
        self.node_with_value("value", value, true);
        self.down();
    }
}

pub fn lr_get<T: Default>() -> T { T::default() }
pub fn lr_token<T>(value: T) -> T { value }
pub fn lr_pack<T>(value: T) -> Vec<T> { vec![value] }
pub fn lr_node<T>(value: T) -> T { value }
pub fn lr_capture_assign<T: Clone>(target: &mut T, value: &T) { *target = value.clone(); }
pub fn lr_parse<T: Default>() -> T { T::default() }
pub fn lr_parse_generalized<T: Default>() -> T { T::default() }

pub mod recovery {
    pub const END_OF_INPUT: usize = usize::MAX;
    #[derive(Clone, Debug)]
    pub struct Policy {
        pub insertion: bool,
        pub deletion: bool,
        pub replacement: bool,
        pub max_skip: usize,
    }
    #[derive(Clone, Debug)]
    pub struct Handler {
        pub expected: Vec<usize>,
        pub continuation: Vec<usize>,
        pub follow: Vec<usize>,
        pub synchronization: Vec<usize>,
        pub policy: Policy,
        pub rule: String,
        pub expected_text: String,
    }
    pub fn handler(expected: &[usize], continuation: &[usize], follow: &[usize], synchronization: &[usize],
                   insertion: bool, deletion: bool, replacement: bool, max_skip: usize,
                   rule: &str, expected_text: &str) -> Handler {
        Handler { expected: expected.to_vec(), continuation: continuation.to_vec(),
                  follow: follow.to_vec(), synchronization: synchronization.to_vec(),
                  policy: Policy { insertion, deletion, replacement, max_skip },
                  rule: rule.to_owned(), expected_text: expected_text.to_owned() }
    }
}

pub mod debug {
    #[derive(Clone, Copy, Debug, Default)] pub struct Loc { pub file: &'static str, pub line: usize, pub column: usize }
    impl Loc { pub fn new(file: &'static str, line: usize, column: usize) -> Self { Self { file, line, column } } }
    #[derive(Default)] pub struct Snapshot;
    impl Snapshot { pub fn new() -> Self { Self } pub fn add<T: ?Sized>(self, _: &'static str, _: &T) -> Self { self } }
    pub trait DebugValue: std::fmt::Debug {}
    impl<T: std::fmt::Debug> DebugValue for T {}
    pub trait DebugFields {
        fn ispa_debug_type(&self) -> &'static str;
        fn ispa_debug_fields(&self, field: &mut dyn FnMut(&str, &dyn DebugValue));
    }
    pub struct RuleScope; pub struct ProductionScope;
    impl RuleScope { pub fn new(_: &str, _: Loc) -> Self { Self } }
    impl ProductionScope { pub fn new(_: &str, _: Loc) -> Self { Self } }
    pub fn enabled() -> bool { false }
    pub fn line(_: Loc, _: Snapshot, _: &str) {}
    pub fn value<T: ?Sized + std::fmt::Debug>(_: &str, _: &T, _: Loc) {}
    pub fn lexer_capture<T: ?Sized + std::fmt::Debug>(_: &str, _: &T) {}
    pub fn active_location() -> Loc { Loc::default() }
}

pub mod dfa {
    pub mod api {
        use crate::Span;
        pub const NULL_STATE: usize = usize::MAX;
        pub const DYNAMIC_FAIL_STATE: usize = usize::MAX - 1;
        pub type CharToClass = [u8; 256];
        #[derive(Clone, Copy, Debug, Default)] pub struct State<T = ()>(pub T);
        #[derive(Clone, Copy, Debug)] pub struct Table<const S: usize, const C: usize>(pub [[usize; C]; S]);
        #[derive(Clone, Copy, Debug)] pub struct AcceptTable<const S: usize>(pub [usize; S]);
        #[derive(Clone, Debug, Default)]
        pub struct Captures { pub registers: Vec<Option<usize>>, pub history: Vec<(usize, Option<usize>)>, pub source: &'static str, pub start: usize }
        impl Captures {
            pub fn view(&self, capture: usize) -> Option<&str> {
                let begin = *self.registers.get(2 * capture)?.as_ref()?;
                let end = *self.registers.get(2 * capture + 1)?.as_ref()?;
                self.source.get(self.start + begin..self.start + end)
            }
            pub fn text(&self, capture: usize) -> Option<String> { self.view(capture).map(str::to_owned) }
            pub fn is_set(&self, capture: usize) -> bool { self.view(capture).is_some() }
            pub fn list(&self, capture: usize) -> Vec<&str> {
                let mut positions = Vec::new();
                let mut node = self.registers.get(2 * capture).copied().flatten();
                while let Some(index) = node {
                    if let Some(&(pos, prev)) = self.history.get(index) { positions.push(pos); node = prev; } else { break; }
                }
                positions.reverse();
                positions.chunks_exact(2).filter_map(|pair| self.source.get(self.start + pair[0]..self.start + pair[1])).collect()
            }
        }
        #[derive(Clone, Copy, Debug, Default)] pub struct DfaDebug;
        impl DfaDebug {
            pub fn new(_: &'static str, _: &'static str, _: i64, _: i64, _: i64,
                       _: i64, _: i64, _: i64, _: u8) -> Self { Self }
        }
        #[derive(Clone, Copy, Debug, Default)] pub struct TdfaLayout<const REGISTERS: usize, const GROUPS: usize>;
        #[derive(Clone, Copy, Debug, Default)] pub struct SpanCharState<'a, T>(pub usize, pub usize, pub Span<'a, T>);
        impl<'a, T> SpanCharState<'a, T> { pub fn new(a: usize, b: usize, c: Span<'a,T>) -> Self { Self(a,b,c) } }
        #[derive(Clone, Copy, Debug, Default)] pub struct SpanMultiTableState<'a, T>(pub usize, pub usize, pub Span<'a,T>);
        impl<'a, T> SpanMultiTableState<'a, T> { pub fn new(a: usize, b: usize, c: Span<'a,T>) -> Self { Self(a,b,c) } }
        #[derive(Clone, Copy, Debug, Default)] pub struct SpanCharTable<T, U>(pub std::marker::PhantomData<(T,U)>);
        impl<T,U> SpanCharTable<T,U> { pub fn from_slice(_: &[U]) -> Self { Self(std::marker::PhantomData) } }
        #[derive(Clone, Copy, Debug, Default)] pub struct SpanMultiTable<'a, T>(pub Span<'a,T>);
        impl<'a,T> SpanMultiTable<'a,T> { pub fn new(v: Span<'a,T>) -> Self { Self(v) } }
        pub fn dynamic_match(cursor: &mut crate::Ptr, alternatives: &[&str], negative: bool, repeat: bool) -> bool {
            let excluded = |pos: crate::Ptr| alternatives.iter().any(|value| !value.is_empty() && pos.starts_with(value));
            if negative {
                if repeat {
                    while !cursor.is_eof() && !excluded(*cursor) { cursor.inc(); }
                    return true;
                }
                if cursor.is_eof() || excluded(*cursor) { return false; }
                cursor.inc();
                return true;
            }
            if let Some(value) = alternatives.iter().find(|value| !value.is_empty() && cursor.starts_with(value)) {
                *cursor = cursor.advance(value.chars().count());
                true
            } else { false }
        }
    }
    /// Execute one TDFA token. A semantic state is numbered after the transition
    /// and action states. The callback returns a next state and optional token.
    pub fn run<T, F, const S: usize, const C: usize, const A: usize>(
        cursor: &mut crate::Ptr, table: &api::Table<S, C>, classes: &api::CharToClass,
        actions: &[[usize; 4]; A], registers: usize, mut semantic: F,
    ) -> Result<Option<T>, crate::Error>
    where F: FnMut(usize, &api::Captures, usize, crate::Ptr, usize, usize, &mut crate::Ptr) -> (usize, Option<T>) {
        let start = *cursor;
        let mut captures = api::Captures { registers: vec![None; registers], history: Vec::new(), source: start.remaining(), start: 0 };
        let mut state = 0usize;
        let mut steps = 0usize;
        let mut last_offset = cursor.offset();
        while state != api::NULL_STATE {
            if steps > 100 && cursor.offset() == last_offset { return Err(crate::Error::new("DFA zero-width transition cycle")); }
            if cursor.offset() != last_offset { last_offset = cursor.offset(); steps = 0; }
            steps += 1;
            if state < S {
                let byte = cursor.remaining().as_bytes().first().copied().unwrap_or(0);
                let class = classes[byte as usize] as usize;
                if class >= C { break; }
                let next = table.0[state][class];
                if next == api::NULL_STATE { break; }
                state = next;
                if state < S && !cursor.is_eof() { cursor.inc(); }
            } else if state < S + A {
                let [op, dst, src, next] = actions[state - S];
                if dst >= captures.registers.len() { return Err(crate::Error::new("DFA register out of range")); }
                let here = cursor.offset() - start.offset();
                let source = captures.registers.get(src).copied().flatten();
                captures.registers[dst] = match op {
                    1 => Some(here), 2 => Some(here + 1), 3 => source,
                    4 | 5 => {
                        captures.history.push((here + usize::from(op == 5), source));
                        Some(captures.history.len() - 1)
                    }
                    _ => return Err(crate::Error::new("unknown DFA register action")),
                };
                state = next;
            } else {
                let (next, token) = semantic(state - S - A, &captures, start.offset(), start,
                    cursor.offset() - start.offset(), start.line(), cursor);
                if next == api::DYNAMIC_FAIL_STATE { return Ok(None); }
                if next == api::NULL_STATE { return Ok(token); }
                state = next;
            }
        }
        Ok(None)
    }
    /// Walk a byte-class DFA and return the last accepting state. State zero is the start.
    /// A transition to NULL_STATE rejects the current byte.
    pub fn decide<const S: usize, const C: usize>(table: &api::Table<S, C>, pos: crate::Ptr, _: &bool) -> usize {
        let mut state = 0;
        let mut accepted = api::NULL_STATE;
        for byte in pos.remaining().bytes() {
            let class = byte as usize;
            if class >= C || state >= S { break; }
            let next = table.0[state][class];
            if next == api::NULL_STATE || next >= S { break; }
            state = next;
            accepted = state;
        }
        accepted
    }
}

#[cfg(test)]
mod execution_tests {
    use super::*;

    #[test]
    fn token_stream_interface_resets_and_creates_cursors() {
        #[derive(Default)]
        struct Stream { tokens: Vec<u8> }
        impl TokenStream for Stream {
            type Cursor = usize;
            type LazyCursor = Ptr;
            fn token_cursor(&mut self) -> usize { self.tokens.len() }
            fn lazy_token_cursor(&mut self, input: Ptr) -> Ptr { input }
            fn reset_token_stream(&mut self) { self.tokens.clear(); }
        }
        let mut stream = Stream { tokens: vec![1, 2] };
        assert_eq!(stream.token_cursor(), 2);
        stream.reset_token_stream();
        assert_eq!(stream.token_cursor(), 0);
        assert_eq!(stream.lazy_token_cursor(Ptr::new("é")).get(), Some(0xc3));
    }

    #[test]
    fn lexer_and_parser_execute_and_backtrack() {
        let lexer = LexerBase::new("a b", |pos| {
            while pos.get() == Some(b' ') { pos.inc(); }
            if let Some(ch) = pos.get() { pos.inc(); Ok(Some(ch)) } else { Ok(None) }
        });
        let mut parser = LLParser::<(), u8>::new(lexer.collect_tokens().unwrap());
        assert_eq!(parser.optional(|p| Ok(p.take_if(|ch| *ch == b'z').copied())).unwrap(), None);
        assert_eq!(parser.position(), 0);
        assert_eq!(parser.required("a", |p| Ok(p.take_if(|ch| *ch == b'a').copied())).unwrap(), b'a');
        assert_eq!(parser.repeated(1, None, |p| Ok(p.take_if(|ch| *ch == b'b').copied())).unwrap(), vec![b'b']);
        assert!(parser.is_eof());
    }

    #[test]
    fn cursor_classifies_each_utf8_byte() {
        let mut cursor = Ptr::new("é\n!");
        let start = cursor;
        assert_eq!(cursor.get(), Some(0xc3));
        cursor.inc();
        assert_eq!(cursor.get(), Some(0xa9));
        assert_eq!(cursor.column(), 2);
        cursor.inc();
        assert_eq!(cursor.get(), Some(b'\n'));
        assert_eq!(string_range(start, cursor), "é");
        cursor.inc();
        assert_eq!((cursor.line(), cursor.column(), cursor.get()), (2, 1, Some(b'!')));
    }

    #[test]
    fn lexer_rejects_non_advancing_callbacks() {
        let mut lexer = LexerBase::new("x", |_| Ok(Some('x')));
        assert!(lexer.next_token().unwrap_err().to_string().contains("did not advance"));
    }

    #[test]
    fn dfa_runs_semantic_accept_and_captures() {
        let mut rows = [[dfa::api::NULL_STATE; 256]; 2];
        rows[0][b'a' as usize] = 1;
        rows[1][b'b' as usize] = 2;
        let table = dfa::api::Table(rows);
        let mut classes = [0u8; 256];
        for (index, class) in classes.iter_mut().enumerate() { *class = index as u8; }
        let mut cursor = Ptr::new("ab!");
        let token = dfa::run(&mut cursor, &table, &classes, &[], 0,
            |state, _, _, _, _, _, cursor| {
                assert_eq!(state, 0);
                cursor.inc();
                (dfa::api::NULL_STATE, Some("ab"))
            }).unwrap();
        assert_eq!(token, Some("ab"));
        assert_eq!(cursor.remaining(), "!");
    }

    #[test]
    fn recovery_keeps_synchronization_token() {
        let mut parser = LLParser::<(), char>::new(vec!['x', 'y', ';', 'z']);
        assert_eq!(parser.recover_until(Error::new("unexpected token"), |token| *token == ';'), 2);
        assert_eq!(parser.peek(), Some(&';'));
        assert_eq!(parser.errors().len(), 1);
    }
}

// The Python runtime (parser/stdlibs/python/ispa_std) lowered to Rust by ispa itself: `python::ispa_std::*`.
include!("lowered.rs");
