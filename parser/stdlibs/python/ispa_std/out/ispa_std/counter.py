"""counter.hpp: repetition counts. A count measures successful matcher
invocations, never payloads or bytes."""
from ispa_std.failures import invalid_argument


class CountSpec:
    name: str
    has_count: bool
    count: int
    minimum: int

    def __init__(self, name: str, has_count: bool, count: int, minimum: int):
        self.name = name
        self.has_count = has_count
        self.count = count
        self.minimum = minimum

    @staticmethod
    def named(name: str, minimum: int) -> "CountSpec":
        return CountSpec(name, False, 0, minimum)

    @staticmethod
    def fixed(value: int, minimum: int) -> "CountSpec":
        if value < 0:
            invalid_argument("a repetition count cannot be negative")
        effective = minimum
        if value >= minimum:
            effective = value
        return CountSpec("", True, value, effective)
