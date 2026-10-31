//! AST printing (`ASTPrinter`, `PrintValue`) and `to_string` / `concat` for generated types (port of ispastdlib
//! ast_printer.hpp and to_string.hpp).
use crate::{Node, Shared};
use std::fmt::Write;

pub struct ASTPrinter<'a> { out: &'a mut dyn Write, branches: Vec<bool> }
impl<'a> ASTPrinter<'a> {
    pub fn new(out: &'a mut dyn Write) -> Self { Self { out, branches: Vec::new() } }
    pub fn up(&mut self, has_more_siblings: bool) { self.branches.push(has_more_siblings); }
    pub fn down(&mut self) { self.branches.pop(); }
    fn prefix(&mut self, last: bool) {
        let depth = self.branches.len();
        if depth == 0 { return; }
        for branch in &self.branches[..depth - 1] { let _ = self.out.write_str(if *branch { "│   " } else { "    " }); }
        let _ = self.out.write_str(if last { "└── " } else { "├── " });
    }
    pub fn node(&mut self, name: impl AsRef<str>, last: bool) { self.prefix(last); let _ = writeln!(self.out, "{}", name.as_ref()); }
    /// A named child: a nested tree for structured values, `name: value` for scalars.
    pub fn node_with_value(&mut self, name: impl AsRef<str>, value: impl PrintValue, last: bool) {
        if value.structured() { self.subtree(name, value, last); }
        else {
            self.prefix(last);
            let _ = writeln!(self.out, "{}: {}", name.as_ref(), value.scalar());
        }
    }
    pub fn subtree(&mut self, name: impl AsRef<str>, value: impl PrintValue, last: bool) {
        self.node(name, last);
        self.up(!last);
        value.print_subtree(self);
        self.down();
    }
}

/// How a value shows up in a printed tree.
pub trait PrintValue {
    /// Printed as a nested tree rather than `name: value`.
    fn structured(&self) -> bool { false }
    fn scalar(&self) -> String { String::new() }
    /// The children of the value's node.
    fn print_subtree(&self, printer: &mut ASTPrinter) {
        printer.node_with_value("value", ScalarText(self.scalar()), true);
    }
    /// What `Node::print` shows for a filled node: the value's tree (nothing for an unset handle).
    fn print_data(&self, printer: &mut ASTPrinter) { self.print_subtree(printer) }
}
struct ScalarText(String);
impl PrintValue for ScalarText { fn scalar(&self) -> String { self.0.clone() } }

macro_rules! scalar_print {
    ($($t:ty),*) => { $(impl PrintValue for $t { fn scalar(&self) -> String { self.to_text() } })* };
}
scalar_print!(String, &str, i64, i32, u64, usize, f64, char);
// the C++ printer streams a bool as a number
impl PrintValue for bool { fn scalar(&self) -> String { (if *self { "1" } else { "0" }).to_string() } }
// `u8` is the IR's character
impl PrintValue for u8 { fn scalar(&self) -> String { (*self as char).to_string() } }
impl<T: PrintValue> PrintValue for Vec<T> {
    fn structured(&self) -> bool { true }
    fn print_subtree(&self, printer: &mut ASTPrinter) {
        for (index, item) in self.iter().enumerate() {
            let last = index + 1 == self.len();
            printer.node("item", last);
            printer.up(!last);
            item.print_subtree(printer);
            printer.down();
        }
    }
}
impl<T: PrintValue> PrintValue for Node<T> {
    // a node prints itself (`<empty>` when unset), so it is always a nested tree
    fn structured(&self) -> bool { true }
    fn scalar(&self) -> String { self.value.scalar() }
    fn print_subtree(&self, printer: &mut ASTPrinter) { self.print(printer) }
}
impl<T: PrintValue + 'static> PrintValue for Shared<T> {
    fn structured(&self) -> bool { !self.is_null() && self.borrow().structured() }
    fn scalar(&self) -> String { if self.is_null() { "nullptr".into() } else { self.borrow().scalar() } }
    fn print_subtree(&self, printer: &mut ASTPrinter) {
        if self.is_null() { printer.node("nullptr", true); } else { self.borrow().print_subtree(printer); }
    }
    fn print_data(&self, printer: &mut ASTPrinter) { if !self.is_null() { self.borrow().print_data(printer); } }
}
impl<T: PrintValue> Node<T> {
    /// Print the node's value as the children of the current tree position.
    pub fn print(&self, printer: &mut ASTPrinter) {
        if self.empty() { printer.node("<empty>", false); return; }
        self.value.print_data(printer)
    }
}

/// `to_string` of a value as the generated code concatenates it.
pub trait ToText { fn to_text(&self) -> String; }
macro_rules! display_text {
    ($($t:ty),*) => { $(impl ToText for $t { fn to_text(&self) -> String { self.to_string() } })* };
}
display_text!(String, &str, i64, i32, u64, usize, bool, f64, char);
impl ToText for u8 { fn to_text(&self) -> String { (*self as char).to_string() } }
impl<T: ToText> ToText for Vec<T> { fn to_text(&self) -> String { self.iter().map(ToText::to_text).collect() } }
impl<T: ToText> ToText for Node<T> { fn to_text(&self) -> String { self.value.to_text() } }
impl<T: ToText + 'static> ToText for Shared<T> {
    fn to_text(&self) -> String { if self.is_null() { String::new() } else { self.borrow().to_text() } }
}
impl<T: ToText> Node<T> { pub fn to_string(&self) -> String { self.value.to_text() } }

/// Join the text of every argument (`ISPA_STD::concat`).
pub fn concat(parts: &[&dyn ToText]) -> String { parts.iter().map(|part| part.to_text()).collect() }

// tuples (a group of a rule: `(COMMA item)`): printed as their items, concatenated as text
macro_rules! tuple_support {
    ($($name:ident $index:tt),+) => {
        impl<$($name: PrintValue),+> PrintValue for ($($name,)+) {
            fn structured(&self) -> bool { true }
            fn print_subtree(&self, printer: &mut ASTPrinter) {
                let count = [$($index),+].len();
                $( printer.node_with_value("item", &self.$index, $index + 1 == count); )+
            }
        }
        impl<$($name: ToText),+> ToText for ($($name,)+) {
            fn to_text(&self) -> String { let mut text = String::new(); $( text += &self.$index.to_text(); )+ text }
        }
    };
}
tuple_support!(A 0);
tuple_support!(A 0, B 1);
tuple_support!(A 0, B 1, C 2);
tuple_support!(A 0, B 1, C 2, D 3);
tuple_support!(A 0, B 1, C 2, D 3, E 4);
tuple_support!(A 0, B 1, C 2, D 3, E 4, F 5);
impl<T: PrintValue> PrintValue for &T {
    fn structured(&self) -> bool { (**self).structured() }
    fn scalar(&self) -> String { (**self).scalar() }
    fn print_subtree(&self, printer: &mut ASTPrinter) { (**self).print_subtree(printer) }
    fn print_data(&self, printer: &mut ASTPrinter) { (**self).print_data(printer) }
}
