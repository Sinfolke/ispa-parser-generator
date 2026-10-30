"""Literal string operations; no regular expressions or locale dependencies."""
from langapi import size

def starts_with(text: str, prefix: str) -> bool:
    if size(prefix) > size(text):
        return False
    i = 0
    while i < size(prefix):
        if text[i] != prefix[i]:
            return False
        i += 1
    return True

def ends_with(text: str, suffix: str) -> bool:
    if size(suffix) > size(text):
        return False
    i = 0
    offset = size(text) - size(suffix)
    while i < size(suffix):
        if text[offset + i] != suffix[i]:
            return False
        i += 1
    return True

def find(text: str, needle: str) -> int:
    """First matching offset, or -1; the empty needle matches at zero."""
    i = 0
    while i + size(needle) <= size(text):
        j = 0
        matched = True
        while j < size(needle):
            if text[i + j] != needle[j]:
                matched = False
            j += 1
        if matched:
            return i
        i += 1
    return -1

def contains(text: str, needle: str) -> bool:
    return find(text, needle) >= 0

def repeat(text: str, amount: int) -> str:
    result = ''
    i = 0
    while i < amount:
        result += text
        i += 1
    return result

def join(parts: list[str], separator: str) -> str:
    result = ''
    i = 0
    while i < size(parts):
        if i > 0:
            result += separator
        result += parts[i]
        i += 1
    return result
