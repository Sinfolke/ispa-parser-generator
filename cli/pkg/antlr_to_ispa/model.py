"""Source-preserving intermediate representation shared by all passes."""
from dataclasses import dataclass, field, asdict
from typing import List


@dataclass
class Span:
    source: str
    start: int
    end: int
    line: int
    column: int


@dataclass
class Diagnostic:
    code: str
    message: str
    remedy: str
    span: Span

    def to_dict(self):
        return asdict(self)


@dataclass
class Token:
    kind: str
    text: str
    span: Span


@dataclass
class Expr:
    kind: str
    value: str = ""
    children: List['Expr'] = field(default_factory=list)
    span: Span = None


@dataclass
class Rule:
    name: str
    body: Expr
    span: Span
    fragment: bool = False
    mode: str = "DEFAULT_MODE"
    headers: List[Token] = field(default_factory=list)

    @property
    def lexer(self):
        return self.name[0].isupper()


@dataclass
class Grammar:
    name: str
    kind: str
    source: str
    rules: List[Rule] = field(default_factory=list)
    options: dict = field(default_factory=dict)
    imports: list = field(default_factory=list)
    declarations: List[Token] = field(default_factory=list)
    diagnostics: List[Diagnostic] = field(default_factory=list)


def walk(expr):
    yield expr
    for child in expr.children:
        yield from walk(child)
