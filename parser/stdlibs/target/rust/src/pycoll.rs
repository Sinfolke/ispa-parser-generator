//! Receiver-polymorphic collection operations for lowered Python code.
//! The IR carries no receiver type, so `x in c` and `c.pop(i)` dispatch on the Rust type here.
use std::collections::{BTreeMap, HashMap, HashSet};
use std::hash::Hash;

pub trait PyContains<K: ?Sized> { fn py_contains(&self, key: &K) -> bool; }
impl<T: PartialEq> PyContains<T> for Vec<T> { fn py_contains(&self, key: &T) -> bool { self.contains(key) } }
impl<K: Eq + Hash, V> PyContains<K> for HashMap<K, V> { fn py_contains(&self, key: &K) -> bool { self.contains_key(key) } }
impl<K: Ord, V> PyContains<K> for BTreeMap<K, V> { fn py_contains(&self, key: &K) -> bool { self.contains_key(key) } }
impl<K: Eq + Hash> PyContains<K> for HashSet<K> { fn py_contains(&self, key: &K) -> bool { self.contains(key) } }
impl PyContains<String> for String { fn py_contains(&self, key: &String) -> bool { self.contains(key.as_str()) } }

pub trait PyPop<K> { type Item; fn py_pop(&mut self, key: K) -> Self::Item; }
impl<T> PyPop<i64> for Vec<T> {
    type Item = T;
    fn py_pop(&mut self, index: i64) -> T {
        let len = self.len() as i64;
        let at = if index < 0 { index + len } else { index };
        if at < 0 || at >= len { crate::fail(1, "pop index out of range"); }
        self.remove(at as usize)
    }
}
impl<K: Eq + Hash, V> PyPop<K> for HashMap<K, V> {
    type Item = V;
    fn py_pop(&mut self, key: K) -> V { self.remove(&key).unwrap_or_else(|| crate::fail(5, "key not found")) }
}

// One-unit strings: the lexer classifies single bytes 0..255, which a Rust String holds as the Latin-1 character.
pub fn char_code(ch: &String) -> i64 { ch.chars().next().map_or(0, |c| (c as u32).min(255) as i64) }
pub fn char_from_code(code: &i64) -> String { if (0..256).contains(code) { (*code as u8 as char).to_string() } else { String::new() } }

/// What lowered Python generics assume of a type parameter: copyable by value, defaultable, comparable.
pub trait PyVal: Clone + Default + PartialEq + PartialOrd + 'static {}
impl<T: Clone + Default + PartialEq + PartialOrd + 'static> PyVal for T {}
/// ...and as a dict key additionally hashable.
pub trait PyKey: PyVal + Eq + Hash {}
impl<T: PyVal + Eq + Hash> PyKey for T {}

/// Python `Any`: a shared, immutable-by-handle dynamically typed value.
#[derive(Clone, Default)]
pub struct PyAny(Option<std::rc::Rc<dyn std::any::Any>>);
impl PyAny {
    pub fn new<T: 'static>(value: T) -> Self { to_any(value) }
    pub fn is_set(&self) -> bool { self.0.is_some() }
    pub fn downcast_ref<T: 'static>(&self) -> Option<&T> { self.0.as_ref().and_then(|value| value.downcast_ref::<T>()) }
}

/// `container[index]` as a read: a copy of the element. Python semantics (negative indices, one-unit str, dict keys).
pub trait PyIndex<I: ?Sized> { type Output; fn py_index(&self, index: &I) -> Self::Output; }
fn py_position(index: i64, len: usize) -> usize {
    let at = if index < 0 { index + len as i64 } else { index };
    if at < 0 || at >= len as i64 { crate::fail(1, "index out of range"); }
    at as usize
}
impl<T: Clone> PyIndex<i64> for Vec<T> {
    type Output = T;
    fn py_index(&self, index: &i64) -> T { self[py_position(*index, self.len())].clone() }
}
/// A str is a sequence of 8-bit units (source bytes); unit `i` is the Latin-1 character of byte `i`.
impl PyIndex<i64> for String {
    type Output = String;
    fn py_index(&self, index: &i64) -> String { (self.as_bytes()[py_position(*index, self.len())] as char).to_string() }
}
impl<K: Eq + Hash, V: Clone> PyIndex<K> for HashMap<K, V> {
    type Output = V;
    fn py_index(&self, key: &K) -> V { self.get(key).cloned().unwrap_or_else(|| crate::fail(5, "key not found")) }
}

impl<I: ?Sized, T: PyIndex<I> + ?Sized> PyIndex<I> for &mut T { type Output = T::Output; fn py_index(&self, index: &I) -> T::Output { (**self).py_index(index) } }
impl<I: ?Sized, T: PyIndex<I> + ?Sized> PyIndex<I> for &T { type Output = T::Output; fn py_index(&self, index: &I) -> T::Output { (**self).py_index(index) } }

/// A one-unit string from whatever a character expression yields (a unit string already, a char, or a byte code).
pub trait CharLike { fn char_string(&self) -> String; }
impl CharLike for String { fn char_string(&self) -> String { self.clone() } }
impl CharLike for char { fn char_string(&self) -> String { self.to_string() } }
impl CharLike for u8 { fn char_string(&self) -> String { (*self as char).to_string() } }
impl CharLike for i64 { fn char_string(&self) -> String { char_from_code(self) } }

/// Wrap a value as `Any`; an `Any` stays as it is.
pub fn to_any<T: 'static>(value: T) -> PyAny {
    let mut slot = Some(value);
    if let Some(any) = (&mut slot as &mut dyn std::any::Any).downcast_mut::<Option<PyAny>>() { return any.take().unwrap(); }
    PyAny(Some(std::rc::Rc::new(slot.take().unwrap())))
}

/// A cast out of a type-erased value (`Any`, or a closed variant's payload): the identity, or a checked downcast.
pub fn py_cast<S: 'static, T: Clone + 'static>(value: S) -> T {
    let mut slot = Some(value);
    if let Some(same) = (&mut slot as &mut dyn std::any::Any).downcast_mut::<Option<T>>() { return same.take().unwrap(); }
    if let Some(Some(any)) = (&slot as &dyn std::any::Any).downcast_ref::<Option<PyAny>>() {
        if let Some(found) = any.downcast_ref::<T>() { return found.clone(); }
    }
    crate::fail(4, "invalid cast")
}
