# Native bindings

Python code in a grammar (`import py`) can call functions that the target language implements.
Declare them in the imported file with ordinary Python; the stub body marks a declaration:

```python
# natives.py
def hash_name(text: str) -> int:
    """Implemented in C++."""
    raise NotImplementedError

class Interner:
    def __init__(self) -> None:
        raise NotImplementedError
    def intern(self, text: str) -> int:
        raise NotImplementedError
```

```
import py natives;
rule: ID { value = natives.hash_name(ID) };
```

A stub is a function or method whose body is only `raise NotImplementedError` (optionally
`NotImplementedError(...)`), after an optional docstring. Parameter and return annotations are required,
as for any imported Python.

| Target | Result |
| --- | --- |
| C++ | declaration in the namespace of the module (`python::natives`); define it in your own translation unit |
| Python | a function that raises `NotImplementedError` until you assign an implementation |
| Rust | rejected with an error |

Calls are checked like calls to any imported Python function. `depends` is no longer part of the language.
