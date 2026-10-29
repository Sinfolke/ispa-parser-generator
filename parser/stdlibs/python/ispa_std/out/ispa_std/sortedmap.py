"""Tiny ordered containers built on native arrays and `<`.

The subset has no dict/set literal, and C++ std::map / std::set iterate in key
order, which the parsers rely on for deterministic results. A sorted array with
binary search gives both."""
from ispa_std.algorithm import lower_bound


class StrIntMap:
    """std::map<std::string, std::size_t>."""
    keys: list
    values: list

    def __init__(self):
        self.keys = []
        self.values = []

    def find(self, key: str) -> int:
        pos = lower_bound(self.keys, 0, len(self.keys), key)
        if pos < len(self.keys) and self.keys[pos] == key:
            return pos
        return -1

    def contains(self, key: str) -> bool:
        return self.find(key) != -1

    def get(self, key: str) -> int:
        return self.values[self.find(key)]

    def set(self, key: str, value: int) -> None:
        pos = lower_bound(self.keys, 0, len(self.keys), key)
        if pos < len(self.keys) and self.keys[pos] == key:
            self.values[pos] = value
        else:
            self.keys.insert(pos, key)
            self.values.insert(pos, value)

    def try_emplace(self, key: str, value: int) -> bool:
        """Insert only when absent; True when inserted."""
        if self.find(key) != -1:
            return False
        self.set(key, value)
        return True

    def clear(self) -> None:
        self.keys.clear()
        self.values.clear()

    def copy(self) -> "StrIntMap":
        out = StrIntMap()
        out.keys.extend(self.keys)
        out.values.extend(self.values)
        return out

    def assign(self, other: "StrIntMap") -> None:
        self.keys.clear()
        self.values.clear()
        self.keys.extend(other.keys)
        self.values.extend(other.values)


class IntSet:
    """std::set<std::size_t>: ascending, unique."""
    items: list

    def __init__(self):
        self.items = []

    def add(self, value: int) -> bool:
        """True when the value was not present (std::set::insert().second)."""
        pos = lower_bound(self.items, 0, len(self.items), value)
        if pos < len(self.items) and self.items[pos] == value:
            return False
        self.items.insert(pos, value)
        return True

    def contains(self, value: int) -> bool:
        pos = lower_bound(self.items, 0, len(self.items), value)
        return pos < len(self.items) and self.items[pos] == value

    def size(self) -> int:
        return len(self.items)

    def empty(self) -> bool:
        return len(self.items) == 0

    def smallest(self) -> int:
        return self.items[0]

    def copy(self) -> "IntSet":
        out = IntSet()
        out.items.extend(self.items)
        return out
