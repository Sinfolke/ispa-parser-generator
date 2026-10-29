"""std::vector operations on the native Array (a Python list here, the C++
target's std::vector there). The array itself is never wrapped; these are the
checked and C++-shaped operations the native methods do not provide."""
from typing import TypeVar

from ispa_std import prim
from ispa_std.errors import ErrorKind

T = TypeVar("T")


def filled(count: int, value: T) -> list:
    result = []
    resize(result, count, value)
    return result


def at(arr: list, index: int) -> T:
    if index < 0 or index >= len(arr):
        prim.fail(ErrorKind.OutOfRange, "vector.at: index out of range")
    return arr[index]


def front(arr: list) -> T:
    if len(arr) == 0:
        prim.fail(ErrorKind.OutOfRange, "vector.front: empty")
    return arr[0]


def back(arr: list) -> T:
    if len(arr) == 0:
        prim.fail(ErrorKind.OutOfRange, "vector.back: empty")
    return arr[len(arr) - 1]


def pop_back(arr: list) -> None:
    if len(arr) == 0:
        prim.fail(ErrorKind.OutOfRange, "vector.pop_back: empty")
    arr.pop()


def insert_at(arr: list, index: int, value: T) -> None:
    if index < 0 or index > len(arr):
        prim.fail(ErrorKind.OutOfRange, "vector.insert_at: index out of range")
    arr.insert(index, value)


def erase_at(arr: list, index: int) -> None:
    if index < 0 or index >= len(arr):
        prim.fail(ErrorKind.OutOfRange, "vector.erase_at: index out of range")
    arr.pop(index)


def erase_range(arr: list, first: int, last: int) -> None:
    # Removes [first, last). Rebuilds once, so it is O(n) rather than O(n * count).
    n = len(arr)
    if first < 0 or last < first or last > n:
        prim.fail(ErrorKind.OutOfRange, "vector.erase_range: bad range")
    kept = arr.copy()
    arr.clear()
    i = 0
    while i < n:
        if i < first or i >= last:
            arr.append(kept[i])
        i = i + 1


def resize(arr: list, count: int, fill: T) -> None:
    if count < 0:
        prim.fail(ErrorKind.LengthError, "vector.resize: negative size")
    while len(arr) > count:
        arr.pop()
    while len(arr) < count:
        arr.append(fill)


def assign(arr: list, count: int, value: T) -> None:
    arr.clear()
    resize(arr, count, value)


def swap(a: list, b: list) -> None:
    tmp = a.copy()
    a.clear()
    a.extend(b)
    b.clear()
    b.extend(tmp)


def lexicographical_less(a: list, b: list) -> bool:
    i = 0
    while i < len(a) and i < len(b):
        if a[i] < b[i]:
            return True
        if b[i] < a[i]:
            return False
        i = i + 1
    return len(a) < len(b)
