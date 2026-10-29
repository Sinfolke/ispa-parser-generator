"""Recursive-descent bootstrap of grammar/antlr.isc with opaque action tokens."""
from .model import Expr, Grammar, Rule
from .scanner import Scanner, GrammarSyntaxError


class Parser:
    def __init__(self, text, source='<input>'):
        self.tokens = Scanner(text, source).scan()
        self.i = 0
        self.source = source

    @property
    def token(self):
        return self.tokens[self.i]

    def take(self, expected=None):
        token = self.token
        if expected is not None and token.text != expected and token.kind != expected:
            self.error(f'expected {expected!r}, found {token.text!r}')
        self.i += 1
        return token

    def accept(self, text):
        if self.token.text == text:
            return self.take()
        return None

    def error(self, message):
        s = self.token.span
        raise GrammarSyntaxError(f'{s.source}:{s.line}:{s.column}: {message}')

    def parse(self):
        kind = self.take().text if self.token.text in ('lexer', 'parser') else 'combined'
        self.take('grammar')
        name = self.take('ID').text
        self.take(';')
        grammar = Grammar(name, kind, self.source)
        mode = 'DEFAULT_MODE'
        while self.token.kind != 'EOF':
            if self.accept('options'):
                block = self.take('ACTION')
                opts = Parser('grammar Options;' + block.text[1:-1], self.source)
                opts.i = 3
                while opts.token.kind != 'EOF':
                    key = opts.take('ID').text
                    opts.take('=')
                    value = []
                    while opts.token.text != ';' and opts.token.kind != 'EOF':
                        value.append(opts.take().text)
                    opts.take(';')
                    grammar.options[key] = ''.join(value).strip("'")
            elif self.token.text in ('tokens', 'channels'):
                grammar.declarations.extend((self.take(), self.take('ACTION')))
            elif self.accept('import'):
                while True:
                    imported = self.take('ID').text
                    if self.accept('='):
                        imported = self.take('ID').text
                    grammar.imports.append(imported)
                    if not self.accept(','):
                        break
                self.take(';')
            elif self.accept('@'):
                grammar.declarations.append(self.take('ID'))
                if self.accept('::'):
                    grammar.declarations.append(self.take('ID'))
                grammar.declarations.append(self.take('ACTION'))
            elif self.accept('mode'):
                mode = self.take('ID').text
                self.take(';')
            else:
                fragment = bool(self.accept('fragment'))
                headers = []
                if self.token.text in ('public', 'private', 'protected'):
                    headers.append(self.take())
                name = self.take('ID')
                while self.token.text != ':' and self.token.kind != 'EOF':
                    headers.append(self.take())
                self.take(':')
                body = self.alternatives()
                self.take(';')
                while self.token.text in ('catch', 'finally'):
                    headers.append(self.take())
                    if self.token.kind == 'BRACKET':
                        headers.append(self.take())
                    headers.append(self.take('ACTION'))
                grammar.rules.append(Rule(name.text, body, name.span, fragment, mode, headers))
        return grammar

    def alternatives(self):
        start = self.token.span
        branches = [self.sequence()]
        while self.accept('|'):
            branches.append(self.sequence())
        return branches[0] if len(branches) == 1 else Expr('choice', children=branches, span=start)

    def sequence(self):
        start = self.token.span
        children = []
        while self.token.text not in ('|', ')', ';', '#', '->') and self.token.kind != 'EOF':
            children.append(self.element())
        if self.accept('->'):
            while True:
                command = self.take('ID')
                argument = ''
                if self.accept('('):
                    argument = self.take().text
                    self.take(')')
                children.append(Expr('command', command.text + (f'({argument})' if argument else ''), span=command.span))
                if not self.accept(','):
                    break
        if self.accept('#'):
            label = self.take('ID')
            children.append(Expr('alt_label', label.text, span=label.span))
        return Expr('sequence', children=children, span=start)

    def element(self, suffix=True):
        token = self.take()
        if token.text == '(':
            options = []
            if self.token.text in ('options', '@') or self.token.text == ':':
                while self.token.text != ':' and self.token.kind != 'EOF':
                    options.append(self.take())
                self.take(':')
            node = Expr('group', children=[self.alternatives()], span=token.span)
            if options:
                node.children.insert(0, Expr('unsupported', ''.join(t.text for t in options), span=options[0].span))
            self.take(')')
        elif token.text == '~':
            node = Expr('not', children=[self.element(False)], span=token.span)
        elif token.kind == 'ACTION':
            node = Expr('predicate' if self.accept('?') else 'action', token.text[1:-1], span=token.span)
        elif token.kind in ('ID', 'STRING', 'BRACKET') or token.text == '.':
            node = Expr({'ID': 'ref', 'STRING': 'literal', 'BRACKET': 'charset'}.get(token.kind, 'any'), token.text, span=token.span)
            if token.kind == 'ID' and self.token.text in ('=', '+='):
                op = self.take().text
                return Expr('list_label' if op == '+=' else 'label', token.text, [self.element()], token.span)
            if token.kind == 'ID' and self.token.kind == 'BRACKET':
                node = Expr('call', self.take().text, [node], token.span)
            if self.accept('..'):
                end = self.take('STRING')
                node = Expr('range', children=[node, Expr('literal', end.text, span=end.span)], span=token.span)
        else:
            self.error(f'unexpected element {token.text!r}')
        if self.accept('<'):
            options = []
            while self.token.text != '>' and self.token.kind != 'EOF':
                options.append(self.take().text)
            self.take('>')
            node = Expr('element_options', ''.join(options), [node], token.span)
        if suffix and self.token.text in ('?', '+', '*'):
            op = self.take().text
            if self.accept('?'):
                op += '?'
            node = Expr('repeat', op, [node], token.span)
        return node


def parse(text, source='<input>'):
    return Parser(text, source).parse()
