//! Error boundary used by the lowered common runtime.
use std::cell::RefCell;
use std::panic::{catch_unwind, panic_any, resume_unwind, AssertUnwindSafe};

#[derive(Debug)]
pub struct StdError { pub kind: i64, pub message: String }

thread_local! { static LAST_MESSAGE: RefCell<String> = const { RefCell::new(String::new()) }; }

pub fn fail(kind: i64, message: impl Into<String>) -> ! {
    panic_any(StdError { kind, message: message.into() })
}

pub fn attempt(function: impl FnOnce()) -> i64 {
    match catch_unwind(AssertUnwindSafe(function)) {
        Ok(()) => 0,
        Err(payload) => match payload.downcast::<StdError>() {
            Ok(error) => {
                let StdError { kind, message } = *error;
                LAST_MESSAGE.with(|last| *last.borrow_mut() = message);
                kind
            }
            Err(payload) => resume_unwind(payload),
        },
    }
}

pub fn failure_message() -> String { LAST_MESSAGE.with(|message| message.borrow().clone()) }

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn nested_attempts_preserve_failure_kind_and_message() {
        assert_eq!(attempt(|| {
            assert_eq!(attempt(|| fail(10, "no match")), 10);
            assert_eq!(failure_message(), "no match");
            fail(11, "external failure");
        }), 11);
        assert_eq!(failure_message(), "external failure");
        assert_eq!(attempt(|| {}), 0);
        assert_eq!(failure_message(), "external failure");
    }

    #[test]
    fn host_panics_propagate_unchanged() {
        let panic = catch_unwind(|| attempt(|| panic_any(37_u32))).unwrap_err();
        assert_eq!(*panic.downcast::<u32>().unwrap(), 37);
    }
}
