"""ANTLR metasyntax scanner; foreign bodies remain opaque and source-spanned.

This deliberately does not tokenize or execute a target language. Tree-sitter
handles action contents after the ANTLR attribute references are resolved.
"""
import bisect
import re
from .model import Span, Token


class GrammarSyntaxError(ValueError):
    pass


class Scanner:
    def __init__(self, text, source):
        self.text, self.source, self.i = text, source, 0
        self.lines = [-1] + [m.start() for m in re.finditer('\n', text)]
        self.python = bool(re.search(r'\blanguage\s*=\s*Python3\b', text))

    def raw_string(self):
        # Rust raw strings and C++ raw strings can contain arbitrary quotes and
        # braces; recognize their delimiters before balancing action braces.
        match = re.match(r'(?:br|r)(#+)?"', self.text[self.i:])
        if match:
            ending = '"' + (match.group(1) or '')
        else:
            match = re.match(r'(?:u8|u|U|L)?R"([^ ()\\\t\r\n]{0,16})\(', self.text[self.i:])
            if not match:
                return False
            ending = ')' + match.group(1) + '"'
        end = self.text.find(ending, self.i + len(match.group(0)))
        if end < 0:
            self.error('unterminated raw string', self.i)
        self.i = end + len(ending)
        return True

    def span(self, start, end):
        line = bisect.bisect_left(self.lines, start)
        return Span(self.source, start, end, line, start - self.lines[line - 1])

    def error(self, message, start):
        s = self.span(start, self.i)
        raise GrammarSyntaxError(f'{s.source}:{s.line}:{s.column}: {message}')

    def quoted(self, quote):
        start = self.i
        triple = self.text.startswith(quote * 3, self.i)
        delimiter = quote * (3 if triple else 1)
        self.i += len(delimiter)
        while self.i < len(self.text):
            if self.text[self.i] == '\\':
                self.i += 2
            elif self.text.startswith(delimiter, self.i):
                self.i += len(delimiter)
                return
            else:
                self.i += 1
        self.error('unterminated string', start)

    def comment(self):
        if self.text.startswith('//', self.i):
            end = self.text.find('\n', self.i)
            self.i = len(self.text) if end < 0 else end
            return True
        if self.text.startswith('/*', self.i):
            start = self.i
            end = self.text.find('*/', self.i + 2)
            if end < 0:
                self.error('unterminated comment', start)
            self.i = end + 2
            return True
        return False

    def balanced(self, opening, closing, charset=False):
        start, depth = self.i, 1
        self.i += 1
        while self.i < len(self.text):
            c = self.text[self.i]
            if not charset and self.raw_string():
                continue
            if opening == '{' and self.python and c == '#':
                end = self.text.find('\n', self.i)
                self.i = len(self.text) if end < 0 else end
            elif c == '\\':
                self.i += 2
            elif c in '\"\'`' and not charset:
                # A Rust lifetime is not a character literal.
                lifetime = re.match(r"'[A-Za-z_]\w*", self.text[self.i:]) if c == "'" else None
                if lifetime and self.i + len(lifetime.group()) < len(self.text) and self.text[self.i + len(lifetime.group())] != "'":
                    self.i += len(lifetime.group())
                    continue
                self.quoted(c)
            elif opening == '{' and self.comment():
                pass
            elif c == opening and not charset:
                depth += 1
                self.i += 1
            elif c == closing:
                depth -= 1
                self.i += 1
                if not depth:
                    return
            else:
                self.i += 1
        self.error(f'unterminated {opening}{closing} block', start)

    def scan(self):
        out = []
        while self.i < len(self.text):
            start = self.i
            c = self.text[self.i]
            if c.isspace() or self.comment():
                if self.i == start:
                    self.i += 1
                continue
            if c == "'":
                self.quoted(c)
                kind = 'STRING'
            elif c in '{[':
                # A character set may contain an unescaped '['. Arguments may
                # contain nested arrays; distinguish them by the lexer rule.
                recent = []
                for previous in reversed(out):
                    if previous.text == ';':
                        break
                    recent.append(previous)
                recent.reverse()
                colon = next((j for j, t in enumerate(recent) if t.text == ':'), None)
                charset = c == '[' and colon is not None and any(t.kind == 'ID' and t.text[0].isupper() for t in recent[:colon])
                self.balanced(c, '}' if c == '{' else ']', charset)
                kind = 'ACTION' if c == '{' else 'BRACKET'
            elif c.isalpha() or c == '_':
                self.i += 1
                while self.i < len(self.text) and (self.text[self.i].isalnum() or self.text[self.i] == '_'):
                    self.i += 1
                kind = 'ID'
            elif c.isdigit():
                self.i += 1
                while self.i < len(self.text) and self.text[self.i].isdigit():
                    self.i += 1
                kind = 'INT'
            else:
                pair = self.text[self.i:self.i + 2]
                self.i += 2 if pair in ('->', '+=', '::', '..') else 1
                kind = self.text[start:self.i]
            out.append(Token(kind, self.text[start:self.i], self.span(start, self.i)))
        out.append(Token('EOF', '', self.span(len(self.text), len(self.text))))
        return out
