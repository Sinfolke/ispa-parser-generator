"""std::span over a native array: a non-owning window [begin, end) that sees
writes made through it. Like the C++ original it is invalidated if the owner
is resized. The algorithms take (array, first, last), so a span plugs in as
algorithm.sort(s.data(), s.begin(), s.end())."""
from typing import Generic, TypeVar

from ispa_std import prim
from ispa_std.errors import ErrorKind

T = TypeVar("T")


class Span(Generic[T]):
    _arr: list
    _begin: int
    _end: int

    def __init__(self, arr: list, begin: int, end: int):
        if begin < 0 or end < begin or end > len(arr):
            prim.fail(ErrorKind.OutOfRange, "Span: window outside array")
        self._arr = arr
        self._begin = begin
        self._end = end

    def data(self) -> list:
        return self._arr

    def begin(self) -> int:
        return self._begin

    def end(self) -> int:
        return self._end

    def size(self) -> int:
        return self._end - self._begin

    def empty(self) -> bool:
        return self._end == self._begin

    def at(self, index: int) -> T:
        if index < 0 or index >= self._end - self._begin:
            prim.fail(ErrorKind.OutOfRange, "Span.at: index out of range")
        return self._arr[self._begin + index]

    def set(self, index: int, value: T) -> None:
        if index < 0 or index >= self._end - self._begin:
            prim.fail(ErrorKind.OutOfRange, "Span.set: index out of range")
        self._arr[self._begin + index] = value

    def first(self, count: int) -> "Span[T]":
        if count < 0 or count > self._end - self._begin:
            prim.fail(ErrorKind.OutOfRange, "Span.first: count out of range")
        return Span(self._arr, self._begin, self._begin + count)

    def last(self, count: int) -> "Span[T]":
        if count < 0 or count > self._end - self._begin:
            prim.fail(ErrorKind.OutOfRange, "Span.last: count out of range")
        return Span(self._arr, self._end - count, self._end)

    def subspan(self, offset: int, count: int) -> "Span[T]":
        if offset < 0 or count < 0 or offset + count > self._end - self._begin:
            prim.fail(ErrorKind.OutOfRange, "Span.subspan: window out of range")
        return Span(self._arr, self._begin + offset, self._begin + offset + count)
