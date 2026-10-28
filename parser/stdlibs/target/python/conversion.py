"""Operations whose C++ IR semantics need explicit Python lowering."""
from functools import cache


def divide(left, right):
    if isinstance(left, int) and isinstance(right, int):
        quotient = abs(left) // abs(right)
        return -quotient if (left < 0) != (right < 0) else quotient
    return left / right


def modulo(left, right):
    return left - divide(left, right) * right


def python_update(target, *arguments):
    for argument in arguments:
        if isinstance(argument, tuple):
            target[argument[0]] = argument[1]
        else:
            target.update(argument)


def substr(value, start, length=None):
    return value[start:] if length is None else value[start:start + length]


def template(factory):
    """A generated class factory retains one type identity per instantiation."""
    return cache(factory)


class TypeTag:
    @classmethod
    @cache
    def specialize(cls, target):
        return type('TypeTag', (cls,), {'target': target})
