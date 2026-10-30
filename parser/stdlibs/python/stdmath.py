"""Integer arithmetic helpers for ISPA grammar actions."""

def clamp(value: int, lower: int, upper: int) -> int:
    """Clamp to inclusive bounds; caller supplies lower <= upper."""
    if value < lower:
        return lower
    if value > upper:
        return upper
    return value

def sign(value: int) -> int:
    if value < 0:
        return -1
    if value > 0:
        return 1
    return 0

def gcd(a: int, b: int) -> int:
    """Nonnegative greatest common divisor, including gcd(0, 0) == 0."""
    if a < 0:
        a = -a
    if b < 0:
        b = -b
    while b != 0:
        remainder = a % b
        a = b
        b = remainder
    return a

def in_range(value: int, lower: int, upper: int) -> bool:
    return value >= lower and value < upper
