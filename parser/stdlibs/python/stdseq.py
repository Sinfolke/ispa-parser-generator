"""Non-mutating integer sequence helpers for ISPA grammar actions."""
from langapi import size, push

def unique(values: list[int]) -> list[int]:
    """Keep first occurrences in input order."""
    result: list[int] = []
    i = 0
    while i < size(values):
        found = False
        j = 0
        while j < size(result):
            if result[j] == values[i]:
                found = True
            j += 1
        if not found:
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

def find(values: list[int], value: int) -> int:
    """Return the first index, or -1 when absent."""
    i = 0
    while i < size(values):
        if values[i] == value:
            return i
        i += 1
    return -1

def take(values: list[int], amount: int) -> list[int]:
    """Copy at most amount elements; negative amounts return an empty list."""
    result: list[int] = []
    i = 0
    while i < amount and i < size(values):
        push(result, values[i])
        i += 1
    return result

def drop(values: list[int], amount: int) -> list[int]:
    """Copy after amount elements; negative amounts behave as zero."""
    result: list[int] = []
    i = amount
    if i < 0:
        i = 0
    while i < size(values):
        push(result, values[i])
        i += 1
    return result

def product(values: list[int]) -> int:
    result = 1
    i = 0
    while i < size(values):
        result *= values[i]
        i += 1
    return result
