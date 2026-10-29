//! The lowered Python runtime (python::ispa_std) executes: strings, sorted maps, algorithms.
use ispa_std::python::ispa_std::{algorithm, format, sortedmap};

#[test]
fn formatting_helpers_run() {
    assert_eq!(format::int_to_string(-42), "-42");
    assert_eq!(format::repeat(&String::from("ab"), 3), "ababab");
}

#[test]
fn reverse_and_find_operate_on_the_callers_vector() {
    let mut values = vec![1i64, 2, 3, 4];
    algorithm::reverse(&mut values, 0, 4);
    assert_eq!(values, vec![4, 3, 2, 1]);
    assert_eq!(algorithm::find(&values, 0, 4, 2), 2);
}

#[test]
fn sorted_map_keeps_keys_ordered() {
    let map = sortedmap::StrIntMap::shared_default();
    map.borrow_mut().__init__();
    for (key, value) in [("pear", 3), ("apple", 1), ("fig", 2)] {
        assert!(map.borrow_mut().try_emplace(&String::from(key), value));
    }
    assert!(!map.borrow_mut().try_emplace(&String::from("fig"), 9));
    assert_eq!(map.borrow_mut().get(&String::from("fig")), 2);
    assert!(map.borrow_mut().contains(&String::from("apple")));
    assert!(!map.borrow_mut().contains(&String::from("plum")));
}
