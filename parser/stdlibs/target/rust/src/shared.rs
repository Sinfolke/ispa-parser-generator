//! Single-threaded object identity for lowered reference-semantic classes.
//!
//! A `Shared<T>` is a handle to a `T` that lives inside some heap allocation (usually a bigger object whose base
//! class part is the `T`). It keeps that allocation alive, shares its borrow flag, and points at the `T`; upcasting a
//! `Shared<Derived>` to a `Shared<Base>` is pointer arithmetic (`Upcast`), so identity is preserved across the
//! hierarchy and subclass handles are accepted wherever a base handle is.
use std::cell::{Cell, UnsafeCell};
use std::marker::PhantomData;
use std::ops::{Deref, DerefMut};
use std::rc::{Rc, Weak};

/// `Self` embeds a `B` (its base class, or itself): the address of that part from the address of `Self`.
pub trait Upcast<B> {
    /// # Safety
    /// `this` must point to a live `Self` (it may be uninitialized memory; only the address is computed).
    unsafe fn up_ptr(this: *mut Self) -> *mut B;
}
impl<T> Upcast<T> for T {
    unsafe fn up_ptr(this: *mut T) -> *mut T { this }
}

/// The allocation behind handles: borrow state + the object.
struct Slot<E> { flag: Cell<isize>, value: UnsafeCell<E> }
trait KeepAlive {}
impl<E> KeepAlive for Slot<E> {}

pub struct Shared<T: ?Sized> { keep: Option<Rc<dyn KeepAlive>>, flag: *const Cell<isize>, ptr: *mut T }
pub struct WeakShared<T: ?Sized> { keep: Option<Weak<dyn KeepAlive>>, flag: *const Cell<isize>, ptr: *mut T }




impl<T> Default for Shared<T> {
    /// A handle to nothing: valid to hold and compare, panics when dereferenced.
    fn default() -> Self { Self { keep: None, flag: std::ptr::null(), ptr: std::ptr::null_mut() } }
}
impl<T> Default for WeakShared<T> {
    fn default() -> Self { Self { keep: None, flag: std::ptr::null(), ptr: std::ptr::null_mut() } }
}
impl<T: ?Sized> Clone for Shared<T> {
    fn clone(&self) -> Self { Self { keep: self.keep.clone(), flag: self.flag, ptr: self.ptr } }
}
impl<T: ?Sized> Clone for WeakShared<T> {
    fn clone(&self) -> Self { Self { keep: self.keep.clone(), flag: self.flag, ptr: self.ptr } }
}

/// A live borrow. It owns a keep-alive reference to the allocation instead of borrowing the handle, so it does not
/// hold the place the handle was read from (a field of `self`) borrowed while it is alive.
pub struct Ref<T: ?Sized> { ptr: *const T, _keep: Option<Rc<dyn KeepAlive>> }
pub struct RefMut<T: ?Sized> { ptr: *mut T, _keep: Option<Rc<dyn KeepAlive>> }
impl<T: ?Sized> Deref for Ref<T> { type Target = T; fn deref(&self) -> &T { unsafe { &*self.ptr } } }
impl<T: ?Sized> Deref for RefMut<T> { type Target = T; fn deref(&self) -> &T { unsafe { &*self.ptr } } }
impl<T: ?Sized> DerefMut for RefMut<T> { fn deref_mut(&mut self) -> &mut T { unsafe { &mut *self.ptr } } }

impl<T: 'static> Shared<T> {
    pub fn new(value: T) -> Self { Self::new_with(|_| value) }

    /// Allocate identity before running initialization, so `self` can escape through constructor callbacks.
    /// The handle is readable once `initialize` has returned.
    pub fn new_with(initialize: impl FnOnce(&WeakShared<T>) -> T) -> Self {
        let slot = Rc::new_cyclic(|weak: &Weak<Slot<T>>| {
            let raw = weak.as_ptr() as *mut Slot<T>;
            // only addresses are computed here; the slot is still being built
            let (flag, ptr) = unsafe { (std::ptr::addr_of!((*raw).flag), UnsafeCell::raw_get(std::ptr::addr_of!((*raw).value))) };
            let handle = WeakShared { keep: Some(weak.clone() as Weak<dyn KeepAlive>), flag, ptr };
            Slot { flag: Cell::new(0), value: UnsafeCell::new(initialize(&handle)) }
        });
        let flag = std::ptr::addr_of!(slot.flag);
        let ptr = slot.value.get();
        Self { keep: Some(slot as Rc<dyn KeepAlive>), flag, ptr }
    }
}

impl<T: ?Sized> Shared<T> {
    fn live(&self) -> bool { !self.ptr.is_null() && self.keep.is_some() }
    pub fn is_null(&self) -> bool { !self.live() }

    // Python objects alias freely (`x.a = x.a + 1` borrows twice in one statement), so there is no dynamic borrow
    // check: the handle keeps the allocation alive, the borrow only exposes the place.
    pub fn borrow(&self) -> Ref<T> {
        assert!(self.live(), "use of an uninitialized object handle");
        Ref { ptr: self.ptr, _keep: self.keep.clone() }
    }
    pub fn borrow_mut(&self) -> RefMut<T> {
        assert!(self.live(), "use of an uninitialized object handle");
        RefMut { ptr: self.ptr, _keep: self.keep.clone() }
    }
    pub fn downgrade(&self) -> WeakShared<T> {
        WeakShared { keep: self.keep.as_ref().map(Rc::downgrade), flag: self.flag, ptr: self.ptr }
    }
    pub fn ptr_eq(&self, other: &Self) -> bool { self.ptr as *const () == other.ptr as *const () }
}
impl<T: 'static> Shared<T> {
    /// The same object seen as one of its base classes (or itself).
    pub fn upcast<B: 'static>(&self) -> Shared<B> where T: Upcast<B> {
        let ptr = if self.ptr.is_null() { std::ptr::null_mut() } else { unsafe { T::up_ptr(self.ptr) } };
        Shared { keep: self.keep.clone(), flag: self.flag, ptr }
    }
}
impl<T: ?Sized> WeakShared<T> {
    pub fn upgrade(&self) -> Option<Shared<T>> {
        let keep = self.keep.as_ref()?.upgrade()?;
        Some(Shared { keep: Some(keep), flag: self.flag, ptr: self.ptr })
    }
}
impl<T: 'static> WeakShared<T> {
    pub fn upcast<B: 'static>(&self) -> WeakShared<B> where T: Upcast<B> {
        let ptr = if self.ptr.is_null() { std::ptr::null_mut() } else { unsafe { T::up_ptr(self.ptr) } };
        WeakShared { keep: self.keep.clone(), flag: self.flag, ptr }
    }
}

// Python object comparison without `__eq__`/`__lt__` is identity.
impl<T: ?Sized> PartialEq for Shared<T> { fn eq(&self, other: &Self) -> bool { self.ptr_eq(other) } }
impl<T: ?Sized> Eq for Shared<T> {}
impl<T: ?Sized> PartialOrd for Shared<T> {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        if self.ptr_eq(other) { Some(std::cmp::Ordering::Equal) } else { None }
    }
}
impl<T: ?Sized> std::hash::Hash for Shared<T> {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) { (self.ptr as *const ()).hash(state) }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Owner { receiver: WeakShared<Owner>, value: i64 }

    #[test]
    fn constructor_retains_identity_and_aliases_share_mutations() {
        let object = Shared::new_with(|receiver| Owner { receiver: receiver.clone(), value: 1 });
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

    #[repr(C)]
    struct Base { x: i64 }
    #[repr(C)]
    struct Derived { base_0: Base, y: i64 }
    impl Upcast<Base> for Derived {
        unsafe fn up_ptr(this: *mut Self) -> *mut Base { std::ptr::addr_of_mut!((*this).base_0) }
    }

    #[test]
    fn upcast_keeps_identity_and_sees_base_fields() {
        let derived = Shared::new(Derived { base_0: Base { x: 3 }, y: 4 });
        let base: Shared<Base> = derived.upcast();
        base.borrow_mut().x = 9;
        assert_eq!(derived.borrow().base_0.x, 9);
        assert!(base.ptr_eq(&derived.upcast()));
    }

    #[test]
    fn null_handles_are_comparable_and_panic_on_use() {
        let null: Shared<Base> = Default::default();
        assert!(null.is_null() && null.ptr_eq(&Default::default()));
        assert!(std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| { null.borrow(); })).is_err());
    }
}
