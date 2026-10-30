#![allow(non_snake_case, non_camel_case_types, non_upper_case_globals, unused_mut, unused_variables, unused_parens,
         unused_labels, unused_imports, unreachable_patterns, unreachable_code, dead_code, unused_braces,
         invalid_reference_casting, unused_must_use, clippy::all)]
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
mod facade;
pub use facade::*;
mod bridge;
pub use bridge::*;
mod parser;
pub use parser::*;
mod lr;
pub use lr::*;
mod printer;
pub use printer::*;
pub(crate) mod lexer;
pub use lexer::TokenEnd;
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
    pub fn at(input: &'static str, offset: usize) -> Self { Self { input, offset } }
    pub fn text(self) -> &'static str { self.input }
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
    pub fn line(self) -> usize { self.position().0 }
    pub fn column(self) -> usize { self.position().1 }

    fn position(self) -> (usize, usize) {
        // Inputs have static lifetimes, so pointer/length identifies immutable text.
        // Index each source once: scanning its prefix for every token is quadratic
        // for long lines, and metadata is read repeatedly while parsing.
        thread_local! {
            static LINES: std::cell::RefCell<std::collections::HashMap<(usize, usize), Vec<usize>>> =
                Default::default();
        }
        LINES.with(|cache| {
            let mut cache = cache.borrow_mut();
            let starts = cache.entry((self.input.as_ptr() as usize, self.input.len())).or_insert_with(|| {
                let mut starts = vec![0];
                starts.extend(self.input.bytes().enumerate().filter_map(|(i, byte)| (byte == b'\n').then_some(i + 1)));
                starts
            });
            let offset = self.offset.min(self.input.len());
            let line = starts.partition_point(|start| *start <= offset) - 1;
            (line + 1, offset - starts[line] + 1)
        })
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

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Error { message: String }
impl Error { pub fn new(message: impl Into<String>) -> Self { Self { message: message.into() } } }
impl fmt::Display for Error { fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result { f.write_str(&self.message) } }
impl std::error::Error for Error {}

pub fn starts_with(pos: Ptr, value: &str) -> bool { pos.starts_with(value) }

pub trait VariantGet<T> { fn get(&self) -> &T; fn get_mut(&mut self) -> &mut T; fn holds(&self) -> bool { true } }
pub fn bad_variant_access() -> ! { panic!("ISPA variant access failed") }

/// Conversion hook used where the language neutral IR permits compatible values.
/// `target = source` where the two may be different but compatible types (a node of a rule into a boxed field, an
/// alternative into the variant that holds it...). The generated code adds the pairs it needs.
pub trait AssignFrom<S> { fn assign_from(&mut self, source: S); }
impl<T> AssignFrom<T> for T { fn assign_from(&mut self, source: T) { *self = source; } }
impl<T: Clone> AssignFrom<Node<T>> for T {
    fn assign_from(&mut self, source: Node<T>) { if !source.empty() { *self = source.value; } }
}
impl<T: 'static> AssignFrom<Node<T>> for Node<Shared<T>> {
    fn assign_from(&mut self, source: Node<T>) { if !source.empty() { let mut node = Node::create(Shared::new(source.value)); node.start_pos = source.start_pos; node.length = source.length; node.line = source.line; node.column = source.column; node.name = source.name; *self = node; } }
}
/// `Vec<T> = Vec<U>`, converting each element.
pub fn assign_list<T: Default + AssignFrom<U>, U>(target: &mut Vec<T>, source: Vec<U>) {
    *target = source.into_iter().map(|item| { let mut converted = T::default(); converted.assign_from(item); converted }).collect();
}
/// `Vec<T> = Vec<U>` with a conversion for each element (groups).
pub fn assign_list_with<T, U>(target: &mut Vec<T>, source: Vec<U>, convert: impl FnMut(U) -> T) { *target = source.into_iter().map(convert).collect(); }
pub fn assign_compatible<T: AssignFrom<S>, S>(target: &mut T, source: S) { target.assign_from(source) }

pub mod debug {
    #[derive(Clone, Copy, Debug, Default)] pub struct Loc { pub file: &'static str, pub line: usize, pub column: usize }
    impl Loc { pub fn new(file: &'static str, line: usize, column: usize) -> Self { Self { file, line, column } } }
    #[derive(Default)] pub struct Snapshot;
    impl Snapshot { pub fn new() -> Self { Self } pub fn add<T: ?Sized>(self, _: &'static str, _: &T) -> Self { self } }
    pub trait DebugValue {}
    impl<T: ?Sized> DebugValue for T {}
    pub trait DebugFields {
        fn ispa_debug_type(&self) -> &'static str;
        fn ispa_debug_fields(&self, field: &mut dyn FnMut(&str, &dyn DebugValue));
    }
    pub struct RuleScope; pub struct ProductionScope;
    impl RuleScope { pub fn new(_: &str, _: Loc) -> Self { Self } }
    impl ProductionScope { pub fn new(_: &str, _: Loc) -> Self { Self } }
    pub fn enabled() -> bool { false }
    pub fn line(_: Loc, _: Snapshot, _: &str) {}
    pub fn value<T: ?Sized>(_: &str, _: &T, _: Loc) {}
    pub fn lexer_capture<T: ?Sized>(_: &str, _: &T) {}
    pub fn active_location() -> Loc { Loc::default() }
}

pub mod dfa {
    pub mod api {
        use crate::Span;
        pub const NULL_STATE: i64 = -1;
        pub const DYNAMIC_FAIL_STATE: i64 = -3;
        pub const LEXER_DELETE_STATE: i64 = -2;
        /// Byte -> character class.
        pub type CharToClass = Vec<i64>;
        /// Transition table: one row of next states per state, one column per character class.
        pub type Table<const STATES: usize, const CLASSES: usize> = [Vec<i64>; STATES];
        pub type AcceptTable<const STATES: usize> = [i64; STATES];
        #[derive(Clone, Copy, Debug, Default)] pub struct State<T = ()>(pub T);
        /// What a scan captured, read while its semantic action runs.
        pub struct Captures { pub(crate) inner: crate::Shared<crate::python::ispa_std::dfa_api::Captures> }
        impl Captures {
            pub fn text(&self, capture: i64) -> String { self.inner.borrow_mut().text(capture) }
            pub fn view(&self, capture: i64) -> String { self.inner.borrow_mut().view(capture) }
            pub fn is_set(&self, capture: i64) -> bool { self.inner.borrow_mut().is_set(capture) }
            pub fn begin(&self, capture: i64) -> i64 { self.inner.borrow_mut().begin(capture) }
            pub fn end(&self, capture: i64) -> i64 { self.inner.borrow_mut().end(capture) }
            pub fn character(&self, capture: i64) -> u8 { self.inner.borrow_mut().character(capture).bytes().next().unwrap_or(0) }
            pub fn list_texts(&self, capture: i64) -> Vec<String> { self.inner.borrow_mut().list_texts(capture) }
            pub fn list_positions(&self, capture: i64) -> Vec<i64> { self.inner.borrow_mut().list_positions(capture) }
            pub fn line(&self, capture: i64) -> i64 { self.inner.borrow_mut().line(capture) }
            pub fn column(&self, capture: i64) -> i64 { self.inner.borrow_mut().column(capture) }
        }
        #[derive(Clone, Copy, Debug, Default)] pub struct DfaDebug;
        impl DfaDebug {
            pub fn new(_: &'static str, _: &'static str, _: i64, _: i64, _: i64,
                       _: i64, _: i64, _: i64, _: u8) -> Self { Self }
        }
        /// Registers and outputs of a lexer's tagged DFA.
        #[derive(Clone, Copy, Debug, Default)] pub struct TdfaLayout<const REGISTERS: usize, const OUTPUTS: usize>;
        impl<const R: usize, const O: usize> TdfaLayout<R, O> { pub const REGISTERS: usize = R; pub const OUTPUTS: usize = O; }
        #[derive(Clone, Copy, Debug, Default)] pub struct SpanCharState<'a, T>(pub usize, pub usize, pub Span<'a, T>);
        impl<'a, T> SpanCharState<'a, T> { pub fn new(a: usize, b: usize, c: Span<'a,T>) -> Self { Self(a,b,c) } }
        #[derive(Clone, Copy, Debug, Default)] pub struct SpanMultiTableState<'a, T>(pub usize, pub usize, pub Span<'a,T>);
        impl<'a, T> SpanMultiTableState<'a, T> { pub fn new(a: usize, b: usize, c: Span<'a,T>) -> Self { Self(a,b,c) } }
        #[derive(Clone, Copy, Debug, Default)] pub struct SpanCharTable<T, U>(pub std::marker::PhantomData<(T,U)>);
        impl<T,U> SpanCharTable<T,U> { pub fn from_slice(_: &[U]) -> Self { Self(std::marker::PhantomData) } }
        #[derive(Clone, Copy, Debug, Default)] pub struct SpanMultiTable<'a, T>(pub Span<'a,T>);
        impl<'a,T> SpanMultiTable<'a,T> { pub fn new(v: Span<'a,T>) -> Self { Self(v) } }
        /// Match one of `alternatives` (or, negated, anything but them: one character, or a run for `*`) at the cursor.
        pub fn dynamic_match(cursor: &mut crate::Ptr, alternatives: impl AsRef<[String]>, negative: bool, quantifier: impl AsRef<str>) -> bool {
            let source = crate::lexer::source_text(cursor.text());
            let found = crate::python::ispa_std::dfa_api::dynamic_match(&source.borrow().text, cursor.offset() as i64, &alternatives.as_ref().to_vec(), negative,
                                                                         &quantifier.as_ref().to_string());
            if found < 0 { return false; }
            *cursor = crate::Ptr::at(cursor.text(), found as usize);
            true
        }
    }
}

#[cfg(test)]
mod execution_tests {
    use super::*;

    #[test]
    fn source_positions_match_byte_offsets_in_any_order() {
        for text in ["", "abc", "\n", "é\r\nline\n\nlast\n", "different source"] {
            for offset in (0..=text.len() + 2).rev() {
                let prefix = &text.as_bytes()[..offset.min(text.len())];
                let expected_line = prefix.iter().filter(|byte| **byte == b'\n').count() + 1;
                let expected_column = prefix.iter().rev().take_while(|byte| **byte != b'\n').count() + 1;
                let cursor = Ptr::at(text, offset);
                assert_eq!((cursor.line(), cursor.column()), (expected_line, expected_column));
            }
        }
        // Alternate sources after indexing both; offsets and byte columns must not
        // come from the most recently visited source.
        assert_eq!(Ptr::at("abc\ndef", 6).column(), 3);
        assert_eq!(Ptr::at("no newline", 6).column(), 7);
        assert_eq!(Ptr::at("abc\ndef", 6).line(), 2);
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


}

// The Python runtime (parser/stdlibs/python/ispa_std) lowered to Rust by ispa itself: `python::ispa_std::*`.
include!("lowered.rs");
