"""Literal string operations; no regular expressions or locale dependencies."""
from langapi import size, push

def starts_with(text: str, prefix: str) -> bool:
    return starts_at(text, prefix, 0)

def ends_with(text: str, suffix: str) -> bool:
    return starts_at(text, suffix, size(text) - size(suffix))

def starts_at(text: str, needle: str, offset: int) -> bool:
    """Whether needle occurs in text beginning at offset."""
    if offset < 0 or offset + size(needle) > size(text):
        return False
    j = 0
    while j < size(needle):
        if text[offset + j] != needle[j]:
            return False
        j += 1
    return True

def find(text: str, needle: str) -> int:
    """First matching offset, or -1; the empty needle matches at zero."""
    i = 0
    while i + size(needle) <= size(text):
        if starts_at(text, needle, i):
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

def count(text: str, needle: str) -> int:
    """Non-overlapping occurrences; the empty needle counts zero."""
    if size(needle) == 0:
        return 0
    result = 0
    i = 0
    while i + size(needle) <= size(text):
        if starts_at(text, needle, i):
            result += 1
            i += size(needle)
        else:
            i += 1
    return result

def replace(text: str, old: str, new: str) -> str:
    """Replace every non-overlapping occurrence; empty old returns text."""
    if size(old) == 0:
        return text
    result = ''
    i = 0
    while i < size(text):
        if starts_at(text, old, i):
            result += new
            i += size(old)
        else:
            result += text[i]
            i += 1
    return result

def split(text: str, separator: str) -> list[str]:
    """Split on a literal separator; empty separator returns [text]."""
    result: list[str] = []
    if size(separator) == 0:
        push(result, text)
        return result
    part = ''
    i = 0
    while i < size(text):
        if starts_at(text, separator, i):
            push(result, part)
            part = ''
            i += size(separator)
        else:
            part += text[i]
            i += 1
    push(result, part)
    return result

def is_space(char: str) -> bool:
    return find(' \t\r\n\f\v', char) >= 0 and size(char) == 1

def strip(text: str) -> str:
    """Remove leading and trailing ASCII whitespace."""
    start = 0
    end = size(text)
    while start < end and is_space(text[start]):
        start += 1
    while end > start and is_space(text[end - 1]):
        end -= 1
    result = ''
    while start < end:
        result += text[start]
        start += 1
    return result

def _map_case(text: str, source: str, target: str) -> str:
    result = ''
    i = 0
    while i < size(text):
        index = find(source, text[i])
        if index >= 0 and size(text[i]) == 1:
            result += target[index]
        else:
            result += text[i]
        i += 1
    return result

def lower(text: str) -> str:
    """ASCII lowercase; other characters are unchanged."""
    return _map_case(text, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')

def upper(text: str) -> str:
    """ASCII uppercase; other characters are unchanged."""
    return _map_case(text, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')
