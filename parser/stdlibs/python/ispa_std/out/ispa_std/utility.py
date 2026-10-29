"""std::pair and the small value helpers from <utility> / <algorithm>."""
from typing import Generic, TypeVar

A = TypeVar("A")
B = TypeVar("B")
T = TypeVar("T")


class Pair(Generic[A, B]):
    first: A
    second: B

    def __init__(self, first: A, second: B):
        self.first = first
        self.second = second

    def __eq__(self, other: "Pair[A, B]") -> bool:
        return self.first == other.first and self.second == other.second

    def __lt__(self, other: "Pair[A, B]") -> bool:
        if self.first < other.first:
            return True
        if other.first < self.first:
            return False
        return self.second < other.second


def make_pair(first: A, second: B) -> Pair[A, B]:
    return Pair(first, second)


def minimum(a: T, b: T) -> T:
    # Like std::min: returns the first argument when they are equivalent.
    if b < a:
        return b
    return a


def maximum(a: T, b: T) -> T:
    # Like std::max: returns the first argument when they are equivalent.
    if a < b:
        return b
    return a


def clamp(value: T, low: T, high: T) -> T:
    if value < low:
        return low
    if high < value:
        return high
    return value
