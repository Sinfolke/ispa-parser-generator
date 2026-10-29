"""std::optional stored as a native array holding zero or one element.

No uninitialised storage is needed, so T need not be default-constructible and
no target primitive is involved.
"""
from typing import Generic, TypeVar

from ispa_std import prim
from ispa_std.errors import ErrorKind

T = TypeVar("T")
U = TypeVar("U")


class Optional(Generic[T]):
    _items: list

    def __init__(self):
        self._items = []

    @staticmethod
    def some(value: T) -> "Optional[T]":
        result = Optional()
        result._items.append(value)
        return result

    @staticmethod
    def none() -> "Optional[T]":
        return Optional()

    def has_value(self) -> bool:
        return len(self._items) == 1

    def value(self) -> T:
        if len(self._items) == 0:
            prim.fail(ErrorKind.BadOptionalAccess, "Optional.value: no value")
        return self._items[0]

    def value_or(self, fallback: T) -> T:
        if len(self._items) == 1:
            return self._items[0]
        return fallback

    def reset(self) -> None:
        self._items.clear()

    def emplace(self, value: T) -> None:
        self._items.clear()
        self._items.append(value)

    def transform(self, fn) -> "Optional[U]":
        if len(self._items) == 1:
            return Optional.some(fn(self._items[0]))
        return Optional.none()

    def and_then(self, fn) -> "Optional[U]":
        if len(self._items) == 1:
            return fn(self._items[0])
        return Optional.none()

    def or_else(self, fn) -> "Optional[T]":
        if len(self._items) == 1:
            return self
        return fn()

    def __eq__(self, other) -> bool:
        if len(self._items) != len(other._items):
            return False
        if len(self._items) == 0:
            return True
        return self._items[0] == other._items[0]
