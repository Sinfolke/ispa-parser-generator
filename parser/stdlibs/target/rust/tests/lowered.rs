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

mod dfa {
    use ispa_std::python::ispa_std::{dfa_api, dfa_run, raw_token, refs, utility};
    use ispa_std::Shared;

    /// A DFA for `[a-z]+`: class 1 is a lowercase letter; the state after the last letter is the semantic action.
    fn tables() -> Shared<dfa_run::DfaTables> {
        let mut class_table = vec![0i64; 256];
        for byte in b'a'..=b'z' { class_table[byte as usize] = 1; }
        let null = *dfa_api::NULL_STATE.get();
        let table = vec![vec![null, 1], vec![2, 1]];
        let layout = dfa_api::TdfaLayout::shared_default();
        layout.borrow_mut().__init__(0, 0);
        let index = dfa_api::DebugIndex::shared_default();
        index.borrow_mut().__init__();
        let tables = dfa_run::DfaTables::shared_default();
        tables.borrow_mut().__init__(&table, &class_table, &Vec::new(), layout, &Vec::new(), index);
        tables
    }

    fn scan(text: &str) -> (i64, i64) {
        let source = dfa_api::SourceText::shared_default();
        source.borrow_mut().__init__(&text.to_string());
        let cursor = refs::Cursor::shared_default();
        cursor.borrow_mut().__init__(0, 1);
        let empty = raw_token::RawToken::empty_token();
        let accepted = std::cell::Cell::new(-1i64);
        let carrier = empty.clone();
        let token = dfa_run::run(source, cursor.clone(), 0, tables(),
            |index, _captures, _start, length, _line, _cursor| {
                accepted.set(length);
                let pair = utility::Pair::<i64, Shared<raw_token::RawToken>>::shared_default();
                pair.borrow_mut().__init__(*dfa_api::NULL_STATE.get(), carrier.clone());
                pair
            }, empty.clone());
        let _ = token;
        let end = cursor.borrow().pos;
        (accepted.get(), end)
    }

    #[test]
    fn lowered_dfa_scans_one_token() {
        assert_eq!(scan("abc def"), (3, 3));
        assert_eq!(scan("x"), (1, 1));
    }
}
