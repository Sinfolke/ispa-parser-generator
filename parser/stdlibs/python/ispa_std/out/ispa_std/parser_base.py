"""parser.hpp: Parser_base, the bridge between generated parsers and the lexer.

Overloads are split: parse_lexer(lex) / parse_text(text) (C++ parse(...)),
set_input_lexer / set_input_text. The generated parser overrides
parseFromTokens and lazyParse."""
from ispa_std.failures import api_error, logic_error
from ispa_std.lexer import Lexer_base
from ispa_std.node import MatchResult, Node


class Parser_base:
    lexer: Lexer_base
    has_lexer: bool
    text: str
    has_text: bool
    tree: object

    def __init__(self):
        self.lexer = Lexer_base()
        self.has_lexer = False
        self.text = ""
        self.has_text = False
        self.tree = Node.blank()

    @staticmethod
    def with_lexer(parser: "Parser_base", lex: Lexer_base) -> "Parser_base":
        parser.lexer = lex
        parser.has_lexer = True
        return parser

    def parse_lexer(self, lex: Lexer_base):
        self.lexer = lex
        self.has_lexer = True
        return self.parse_tokens()

    def parse_text(self, text: str):
        self.text = text
        self.has_text = True
        return self.parseLazy()

    def set_input_lexer(self, lex: Lexer_base) -> None:
        if not lex.hasTokens():
            self.lexer = lex
            self.has_lexer = True

    def set_input_text(self, text: str) -> None:
        self.text = text
        self.has_text = True

    def clear_input(self) -> None:
        self.has_lexer = False
        self.has_text = False
        self.text = ""

    # Parse NON-LAZILY input (C++ protected virtual parse()).
    def parse_tokens(self):
        if not self.has_lexer:
            api_error("Can't parse because no input lexer is provided", [])
        if not self.lexer.hasInput():
            self.lexer.makeTokens()
        self.parseFromTokens()
        return self.tree

    # Parse LAZILY input.
    def parseLazy(self):
        if not self.has_text:
            api_error("Can't parse because no input text is provided", [])
        result = self.lazyParse()
        if result.status:
            self.tree = result.node.data()
        return self.tree

    # DO NOT OVERRIDE the two above; override these (generated code does).
    def parseFromTokens(self) -> None:
        logic_error("Parser_base.parseFromTokens is abstract")

    def lazyParse(self) -> MatchResult:
        logic_error("Parser_base.lazyParse is abstract")
        return MatchResult.failed(0)
