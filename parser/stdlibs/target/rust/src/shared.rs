//! Single-threaded object identity for lowered reference-semantic classes.
use std::cell::{Ref, RefCell, RefMut};
use std::rc::{Rc, Weak};

pub struct Shared<T>(Rc<RefCell<Option<T>>>);
pub struct WeakShared<T>(Weak<RefCell<Option<T>>>);

impl<T> Clone for Shared<T> {
    fn clone(&self) -> Self { Self(Rc::clone(&self.0)) }
}
impl<T> Default for WeakShared<T> {
    fn default() -> Self { Self(Weak::new()) }
}
impl<T: Default> Default for Shared<T> {
    fn default() -> Self { Self::new(T::default()) }
}
impl<T> Clone for WeakShared<T> {
    fn clone(&self) -> Self { Self(Weak::clone(&self.0)) }
}
impl<T> Shared<T> {
    pub fn new(value: T) -> Self { Self(Rc::new(RefCell::new(Some(value)))) }

    /// Allocate identity before running initialization, so `self` can escape
    /// through constructor callbacks. Fields become readable after it returns.
    pub fn new_with(initialize: impl FnOnce(&Self) -> T) -> Self {
        let object = Self(Rc::new(RefCell::new(None)));
        let value = initialize(&object);
        *object.0.borrow_mut() = Some(value);
        object
    }

    pub fn borrow(&self) -> Ref<'_, T> {
        Ref::map(self.0.borrow(), |value| value.as_ref().expect("object initialization is incomplete"))
    }
    pub fn borrow_mut(&self) -> RefMut<'_, T> {
        RefMut::map(self.0.borrow_mut(), |value| value.as_mut().expect("object initialization is incomplete"))
    }
    pub fn downgrade(&self) -> WeakShared<T> { WeakShared(Rc::downgrade(&self.0)) }
    pub fn ptr_eq(&self, other: &Self) -> bool { Rc::ptr_eq(&self.0, &other.0) }
}
impl<T> WeakShared<T> {
    pub fn upgrade(&self) -> Option<Shared<T>> { self.0.upgrade().map(Shared) }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Owner { receiver: WeakShared<Owner>, value: i64 }

    #[test]
    fn constructor_retains_identity_and_aliases_share_mutations() {
        let object = Shared::new_with(|receiver| Owner { receiver: receiver.downgrade(), value: 1 });
        let alias = object.clone();
        alias.borrow_mut().value = 7;
        let receiver = object.borrow().receiver.upgrade().unwrap();
        assert!(object.ptr_eq(&receiver));
        assert_eq!(receiver.borrow().value, 7);
        let weak = receiver.downgrade();
        drop(receiver);
        drop(alias);
        drop(object);
        assert!(weak.upgrade().is_none());
    }

    #[test]
    fn cloning_a_handle_does_not_require_cloning_the_value() {
        struct NotClone;
        let value = Shared::new(NotClone);
        assert!(value.ptr_eq(&value.clone()));
    }
}
