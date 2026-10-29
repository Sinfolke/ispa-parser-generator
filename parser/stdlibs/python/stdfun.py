"""Common helpers implemented in the Python subset accepted by ISPA."""

from langapi import size, push, extend


def zip(keys: list, values: list) -> dict:
    """Build a mapping from corresponding elements of two lists.

    This ISPA helper intentionally has the two-list mapping shape used by
    grammar capture utilities, rather than Python's iterator-of-tuples shape.
    If the lists have different lengths, extra elements are ignored.
    """
    result = {}
    i = 0
    while i < size(keys) and i < size(values):
        result[keys[i]] = values[i]
        i += 1
    return result


def range(stop: int) -> list[int]:
    """Return the integers from zero up to, but not including, stop."""
    result: list[int] = []
    i = 0
    while i < stop:
        push(result, i)
        i += 1
    return result


def sum(values: list[int]) -> int:
    """Return the sum of integer values."""
    result = 0
    i = 0
    while i < size(values):
        result += values[i]
        i += 1
    return result


def min(values: list[int]) -> int:
    """Return the smallest integer in a non-empty list."""
    result = values[0]
    i = 0
    while i < size(values):
        if values[i] < result:
            result = values[i]
        i += 1
    return result


def max(values: list[int]) -> int:
    """Return the largest integer in a non-empty list."""
    result = values[0]
    i = 0
    while i < size(values):
        if values[i] > result:
            result = values[i]
        i += 1
    return result


def abs(value: int) -> int:
    """Return the absolute value of an integer."""
    if value < 0:
        return -value
    return value


def all(values: list[bool]) -> bool:
    """Return whether every boolean in the list is true."""
    i = 0
    while i < size(values):
        if not values[i]:
            return False
        i += 1
    return True


def any(values: list[bool]) -> bool:
    """Return whether at least one boolean in the list is true."""
    i = 0
    while i < size(values):
        if values[i]:
            return True
        i += 1
    return False


def reversed(values: list[int]) -> list[int]:
    """Return a new list containing the integers in reverse order."""
    result: list[int] = []
    i = size(values) - 1
    while i >= 0:
        push(result, values[i])
        i -= 1
    return result


def sorted(values: list[int]) -> list[int]:
    """Return a stable ascending copy of an integer list."""
    result: list[int] = []
    extend(result, values)
    i = 1
    value = 0
    j = 0
    while i < size(result):
        value = result[i]
        j = i
        while j > 0 and result[j - 1] > value:
            result[j] = result[j - 1]
            j -= 1
        result[j] = value
        i += 1
    return result

