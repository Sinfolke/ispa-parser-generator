"""<algorithm> over native arrays, written in the iterator-pair style with
Int indices: every function takes (arr, first, last) meaning [first, last).
"Not found" is -1. Predicates and comparators are ordinary callables."""
from typing import Callable, TypeVar

from ispa_std import prim
from ispa_std.errors import ErrorKind

T = TypeVar("T")
U = TypeVar("U")


def _check(arr: list[T], first: int, last: int) -> None:
    if first < 0 or last < first or last > len(arr):
        prim.fail(ErrorKind.OutOfRange, "algorithm: range outside array")


def for_each(arr: list[T], first: int, last: int, fn: Callable[[T], None]) -> None:
    _check(arr, first, last)
    i = first
    while i < last:
        fn(arr[i])
        i = i + 1


def find(arr: list[T], first: int, last: int, value: T) -> int:
    _check(arr, first, last)
    i = first
    while i < last:
        if arr[i] == value:
            return i
        i = i + 1
    return -1


def find_if(arr: list[T], first: int, last: int, pred: Callable[[T], bool]) -> int:
    _check(arr, first, last)
    i = first
    while i < last:
        if pred(arr[i]):
            return i
        i = i + 1
    return -1


def count(arr: list[T], first: int, last: int, value: T) -> int:
    total = 0
    i = first
    _check(arr, first, last)
    while i < last:
        if arr[i] == value:
            total = total + 1
        i = i + 1
    return total


def count_if(arr: list[T], first: int, last: int, pred: Callable[[T], bool]) -> int:
    total = 0
    i = first
    _check(arr, first, last)
    while i < last:
        if pred(arr[i]):
            total = total + 1
        i = i + 1
    return total


def all_of(arr: list[T], first: int, last: int, pred: Callable[[T], bool]) -> bool:
    return count_if(arr, first, last, pred) == last - first


def any_of(arr: list[T], first: int, last: int, pred: Callable[[T], bool]) -> bool:
    return find_if(arr, first, last, pred) != -1


def none_of(arr: list[T], first: int, last: int, pred: Callable[[T], bool]) -> bool:
    return find_if(arr, first, last, pred) == -1


def equal(a: list[T], a_first: int, a_last: int, b: list[T], b_first: int) -> bool:
    # Like std::equal(first1, last1, first2): b must have at least that many elements.
    _check(a, a_first, a_last)
    _check(b, b_first, b_first + (a_last - a_first))
    i = 0
    while i < a_last - a_first:
        if a[a_first + i] != b[b_first + i]:
            return False
        i = i + 1
    return True


def accumulate(arr: list[T], first: int, last: int, init: U, fn: Callable[[U, T], U]) -> U:
    _check(arr, first, last)
    total = init
    i = first
    while i < last:
        total = fn(total, arr[i])
        i = i + 1
    return total


def transform(arr: list[T], first: int, last: int, fn: Callable[[T], U]) -> list[U]:
    _check(arr, first, last)
    result = []
    i = first
    while i < last:
        result.append(fn(arr[i]))
        i = i + 1
    return result


def fill(arr: list[T], first: int, last: int, value: T) -> None:
    _check(arr, first, last)
    i = first
    while i < last:
        arr[i] = value
        i = i + 1


def reverse(arr: list[T], first: int, last: int) -> None:
    _check(arr, first, last)
    i = first
    j = last - 1
    while i < j:
        tmp = arr[i]
        arr[i] = arr[j]
        arr[j] = tmp
        i = i + 1
        j = j - 1


def min_element(arr: list[T], first: int, last: int) -> int:
    _check(arr, first, last)
    if first == last:
        return -1
    best = first
    i = first + 1
    while i < last:
        if arr[i] < arr[best]:
            best = i
        i = i + 1
    return best


def max_element(arr: list[T], first: int, last: int) -> int:
    _check(arr, first, last)
    if first == last:
        return -1
    best = first
    i = first + 1
    while i < last:
        if arr[best] < arr[i]:
            best = i
        i = i + 1
    return best


def is_sorted(arr: list[T], first: int, last: int) -> bool:
    _check(arr, first, last)
    i = first + 1
    while i < last:
        if arr[i] < arr[i - 1]:
            return False
        i = i + 1
    return True


def lower_bound(arr: list[T], first: int, last: int, value: T) -> int:
    # First index in [first, last) whose element is not less than `value`.
    _check(arr, first, last)
    lo = first
    hi = last
    while lo < hi:
        mid = lo + (hi - lo) // 2
        if arr[mid] < value:
            lo = mid + 1
        else:
            hi = mid
    return lo


def upper_bound(arr: list[T], first: int, last: int, value: T) -> int:
    # First index in [first, last) whose element is greater than `value`.
    _check(arr, first, last)
    lo = first
    hi = last
    while lo < hi:
        mid = lo + (hi - lo) // 2
        if value < arr[mid]:
            hi = mid
        else:
            lo = mid + 1
    return lo


def binary_search(arr: list[T], first: int, last: int, value: T) -> bool:
    pos = lower_bound(arr, first, last, value)
    return pos < last and not (value < arr[pos])


def stable_sort(arr: list[T], first: int, last: int, less: Callable[[T, T], bool]) -> None:
    # Bottom-up merge sort. A right-hand element is taken only when strictly
    # less than the left one, which is what keeps equal elements in order.
    _check(arr, first, last)
    n = last - first
    if n < 2:
        return
    tmp = []
    i = 0
    while i < n:
        tmp.append(arr[first + i])
        i = i + 1
    width = 1
    while width < n:
        lo = 0
        while lo < n:
            mid = lo + width
            if mid > n:
                mid = n
            hi = lo + 2 * width
            if hi > n:
                hi = n
            a = lo
            b = mid
            k = lo
            while a < mid and b < hi:
                if less(arr[first + b], arr[first + a]):
                    tmp[k] = arr[first + b]
                    b = b + 1
                else:
                    tmp[k] = arr[first + a]
                    a = a + 1
                k = k + 1
            while a < mid:
                tmp[k] = arr[first + a]
                a = a + 1
                k = k + 1
            while b < hi:
                tmp[k] = arr[first + b]
                b = b + 1
                k = k + 1
            lo = lo + 2 * width
        i = 0
        while i < n:
            arr[first + i] = tmp[i]
            i = i + 1
        width = width * 2


def sort(arr: list[T], first: int, last: int) -> None:
    stable_sort(arr, first, last, lambda a, b: a < b)


def erase_if(arr: list[T], pred: Callable[[T], bool]) -> int:
    # The erase-remove idiom in one call over the whole array. Returns the number removed.
    n = len(arr)
    kept = []
    i = 0
    while i < n:
        if not pred(arr[i]):
            kept.append(arr[i])
        i = i + 1
    arr.clear()
    arr.extend(kept)
    return n - len(kept)
