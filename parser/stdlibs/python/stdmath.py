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
    a = abs_value(a)
    b = abs_value(b)
    while b != 0:
        remainder = a % b
        a = b
        b = remainder
    return a

def in_range(value: int, lower: int, upper: int) -> bool:
    return value >= lower and value < upper

def lcm(a: int, b: int) -> int:
    """Nonnegative least common multiple; zero if either argument is zero."""
    if a == 0 or b == 0:
        return 0
    return abs_value(a // gcd(a, b) * b)

def abs_value(value: int) -> int:
    if value < 0:
        return -value
    return value

def power(base: int, exponent: int) -> int:
    """Integer power; negative exponents yield 0 (integer truncation)."""
    if exponent < 0:
        return 0
    result = 1
    while exponent > 0:
        if exponent % 2 == 1:
            result *= base
        base *= base
        exponent //= 2
    return result

def is_even(value: int) -> bool:
    return value % 2 == 0
