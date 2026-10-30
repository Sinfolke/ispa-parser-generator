use ispa_std::pycoll::{PyIndex, PyIndexBorrow};
use std::{cell::Cell, collections::HashMap, rc::Rc};

struct Counted(Rc<Cell<usize>>);
impl Clone for Counted {
    fn clone(&self) -> Self {
        self.0.set(self.0.get() + 1);
        Self(self.0.clone())
    }
}

#[test]
fn nested_read_clones_only_the_selected_value() {
    let clones = Rc::new(Cell::new(0));
    let rows = vec![vec![Counted(clones.clone()), Counted(clones.clone())]];
    let row = rows.py_index_borrow(&-1);
    assert_eq!(clones.get(), 0);
    let _owned = row.py_index(&-1);
    assert_eq!(clones.get(), 1);
    assert_eq!(ispa_std::attempt(|| { rows.py_index_borrow(&-2); }), 1);
    assert_eq!(ispa_std::attempt(|| { rows.py_index_borrow(&1); }), 1);
}

#[test]
fn dictionary_rows_and_string_units_keep_python_semantics() {
    let mut rows = HashMap::new();
    rows.insert(String::from("row"), vec![String::from("value")]);
    let mut owned = rows.py_index_borrow(&String::from("row")).py_index(&-1);
    owned.push('!');
    assert_eq!(rows["row"][0], "value");
    assert_eq!(ispa_std::attempt(|| { rows.py_index_borrow(&String::from("missing")); }), 5);
    let text = String::from("é");
    assert_eq!(text.py_index_borrow(&0).py_index(&0), text.py_index(&0).py_index(&0));
}
