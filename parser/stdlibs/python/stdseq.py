"""Non-mutating integer sequence helpers for ISPA grammar actions."""
from langapi import size, push

def find(values: list[int], value: int) -> int:
    """Return the first index, or -1 when absent."""
    i = 0
    while i < size(values):
        if values[i] == value:
            return i
        i += 1
    return -1

def contains(values: list[int], value: int) -> bool:
    return find(values, value) >= 0

def unique(values: list[int]) -> list[int]:
    """Keep first occurrences in input order."""
    result: list[int] = []
    i = 0
    while i < size(values):
        if not contains(result, values[i]):
            push(result, values[i])
        i += 1
    return result

def count(values: list[int], value: int) -> int:
    result = 0
    i = 0
    while i < size(values):
        if values[i] == value:
            result += 1
        i += 1
    return result

def slice(values: list[int], start: int, stop: int) -> list[int]:
    """Copy values[start:stop], clamping both bounds to the list."""
    result: list[int] = []
    i = start
    if i < 0:
        i = 0
    while i < stop and i < size(values):
        push(result, values[i])
        i += 1
    return result

def take(values: list[int], amount: int) -> list[int]:
    """Copy at most amount elements; negative amounts return an empty list."""
    return slice(values, 0, amount)

def drop(values: list[int], amount: int) -> list[int]:
    """Copy after amount elements; negative amounts behave as zero."""
    return slice(values, amount, size(values))

def concat(first: list[int], second: list[int]) -> list[int]:
    result = take(first, size(first))
    i = 0
    while i < size(second):
        push(result, second[i])
        i += 1
    return result

def product(values: list[int]) -> int:
    result = 1
    i = 0
    while i < size(values):
        result *= values[i]
        i += 1
    return result
