"""std::unordered_map operations on the native Map (dict here)."""
from typing import TypeVar

from ispa_std import prim
from ispa_std.errors import ErrorKind
from ispa_std.optional import Optional

K = TypeVar("K")
V = TypeVar("V")


def contains(m: dict[K, V], key: K) -> bool:
    return key in m


def at(m: dict[K, V], key: K) -> V:
    if key not in m:
        prim.fail(ErrorKind.KeyNotFound, "map.at: key not found")
    return m[key]


def get(m: dict[K, V], key: K) -> Optional[V]:
    result = Optional[V]()
    if key in m:
        result.emplace(m[key])
    return result


def insert(m: dict[K, V], key: K, value: V) -> bool:
    # Like unordered_map::insert: never overwrites. Returns whether it inserted.
    if key in m:
        return False
    m[key] = value
    return True


def insert_or_assign(m: dict[K, V], key: K, value: V) -> bool:
    inserted = key not in m
    m[key] = value
    return inserted


def erase(m: dict[K, V], key: K) -> bool:
    if key not in m:
        return False
    m.pop(key)
    return True
