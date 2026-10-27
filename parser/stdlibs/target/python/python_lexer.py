"""Python layout and literal support for a generated Python grammar lexer."""

from __future__ import annotations

import re
from inspect import signature


def make_python_lexer(generated, newline_state):
    """Run the generated DFA and its advanced token actions with EOF layout."""

    class BootstrapLexer(generated.Lexer):
        def __init__(self, source: str = ''):
            super().__init__(source)
            self._use_on_token = True
            self.ispa_pending_tokens = []
            self.ispa_shared_indents = [0]

        def on_token(self, token):
            if not self.ispa_skip_token:
                self.tokens.append(token)
            self.ispa_skip_token = False
            self.tokens.extend(self.ispa_pending_tokens)
            self.ispa_pending_tokens.clear()

        def _next_char_at(self, offset):
            return self.source[offset:offset + 1]

        def _layout_semantic(self, state, start, length, pos):
            args = (state, None, start, self.source, length, self._line(start))
            if len(signature(self.semantic_action_exec).parameters) >= 7:
                return self.semantic_action_exec(*args, pos)
            return self.semantic_action_exec(*args)

        def makeToken(self, pos):
            start = pos.index
            rest = self.source[start:]
            # The generated DFA does not accept its NEWLINE rule after a
            # repeated blank line. Run that rule's generated action directly.
            blank_lines = re.match(r'\n(?:[ \t]*\n)+[ \t]*', rest)
            if blank_lines:
                width = len(blank_lines.group())
                pos.advance(width)
                return self._layout_semantic(newline_state, start, width, pos)[1]
            if rest.startswith('#'):
                end = rest.find('\n')
                if end < 0:
                    end = len(rest)
                prefix = self.source[self.source.rfind('\n', 0, start) + 1:start]
                if not prefix.strip() and end < len(rest):
                    newline = start + end
                    after = newline + 1
                    while after < len(self.source) and self.source[after] in ' \t':
                        after += 1
                    self._layout_semantic(newline_state, newline, after - newline, pos)
                    self.tokens.extend(self.ispa_pending_tokens)
                    self.ispa_pending_tokens.clear()
                    self.ispa_skip_token = False
                    end = after - start
                pos.advance(end)
                return None
            if rest.startswith(("'", '"')):
                quote = rest[0]
                width = 3 if rest.startswith(quote * 3) else 1
                index = width
                while index < len(rest):
                    if rest[index] == '\\' and index + 1 < len(rest):
                        index += 2
                        continue
                    if rest.startswith(quote * width, index):
                        value = rest[width:index]
                        end = index + width
                        pos.advance(end)
                        ids = generated.Tokens.modules.expr_valued_tokens
                        types = generated.Types.modules.expr_valued_tokens
                        return generated.Token.create(start, self.source, end,
                                                      self._line(start), ids.STRING.id,
                                                      types.STRING(value), self._column(start))
                    if width == 1 and rest[index] in '\r\n':
                        break
                    index += 1
                raise SyntaxError(f'unterminated Python string at offset {start}')
            return super().makeToken(pos)

        def push(self):
            offset = len(self.source)
            line = self._line(offset)
            column = self._column(offset)
            token_type = generated.Types.modules.tokens
            token_id = generated.Tokens.modules.tokens

            def synthetic(name):
                return generated.Token.create(offset, self.source, 0, line,
                                              getattr(token_id, name).id,
                                              getattr(token_type, name)(), column)

            last_logical = next((item for item in reversed(self.tokens)
                                 if not item.holds(token_type.DEDENT)), None)
            if last_logical is not None and not last_logical.holds(token_type.NEWLINE):
                self.tokens.append(synthetic('NEWLINE'))
            while len(self.ispa_shared_indents) > 1:
                self.ispa_shared_indents.pop()
                self.tokens.append(synthetic('DEDENT'))

    return BootstrapLexer
