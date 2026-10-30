"""Conservative Flex/Bison conversion with preserved source and diagnostics."""
import ast
import json
import re


def literal(value):
    return "'" + value.replace('\\', '\\\\').replace("'", "\\'").replace('\n', '\\n').replace('\r', '\\r').replace('\t', '\\t') + "'"


def scan(text, first=False):
    """Split grammar syntax while keeping strings, classes and actions intact."""
    tokens = []
    i = 0
    while i < len(text):
        if text[i].isspace():
            i += 1
            continue
        if text.startswith('/*', i) or text.startswith('//', i):
            end = text.find('*/' if text[i + 1] == '*' else '\n', i + 2)
            if end < 0:
                if text[i + 1] == '*':
                    raise ValueError('unterminated comment')
                break
            i = end + (2 if text[i + 1] == '*' else 1)
            continue
        start = i
        c = text[i]
        if c in "\"'[{":
            closing = {'[': ']', '{': '}'}.get(c, c)
            depth = 1
            i += 1
            while i < len(text):
                if text[i] == '\\':
                    i += 2
                    continue
                if c == '{' and text[i] in "\"'":
                    quote = text[i]
                    i += 1
                    while i < len(text) and text[i] != quote:
                        i += 2 if text[i] == '\\' else 1
                elif c == '{' and text.startswith('/*', i):
                    end = text.find('*/', i + 2)
                    if end < 0:
                        raise ValueError('unterminated action comment')
                    i = end + 2
                    continue
                elif c == '{' and text.startswith('//', i):
                    end = text.find('\n', i + 2)
                    i = len(text) if end < 0 else end + 1
                    continue
                elif c == '{' and text[i] == '{':
                    depth += 1
                elif text[i] == closing:
                    depth -= 1
                    if depth == 0:
                        i += 1
                        break
                i += 1
            else:
                raise ValueError('unterminated string, class or action')
        else:
            match = re.match(r'%[\w-]+|[A-Za-z_][\w]*|\d+|.', text[i:])
            i += len(match[0])
        tokens.append(text[start:i])
        if first:
            return tokens
    return tokens


def sections(source):
    parts = re.split(r'^\s*%%\s*$', source, flags=re.M)
    if len(parts) < 2:
        raise ValueError('expected a %% section separator')
    return parts[0], parts[1], '\n%%\n'.join(parts[2:])


def convert(source, kind):
    declarations, body, trailer = sections(source)
    warnings, actions, lines = [], [], [f'// Converted from {kind}. Review the migration report.', '']
    def warn(code, message):
        warnings.append({'code': code, 'message': message})
    if trailer.strip():
        warn('user-code', 'Trailing user code requires manual migration.')
    if kind == 'bison':
        aliases = {}
        for declaration in re.findall(r'^\s*%token\s+([^\n]+)', declarations, re.M):
            previous = None
            for token in scan(declaration):
                if token.startswith(('"', "'")) and previous:
                    aliases[token] = previous
                elif re.fullmatch(r'[A-Za-z_]\w*', token):
                    previous = token
        if declarations.strip():
            warn('declarations', 'Review token declarations, start symbol, types, precedence and parser options; supply lexer rules separately.')
        tokens = scan(body)
        if not tokens:
            raise ValueError('no Bison grammar rules found')
        i = 0
        while i < len(tokens):
            name = tokens[i]
            if not re.fullmatch(r'[A-Za-z_]\w*', name) or i + 1 >= len(tokens) or tokens[i + 1] != ':':
                raise ValueError(f'expected a rule name followed by colon near {name!r}')
            if name.upper() == name:
                warn('rule-name', f'Parser rule {name} must be renamed to lowercase in ISPA.')
            i += 2
            alternatives, current = [], []
            while i < len(tokens) and tokens[i] != ';':
                token = tokens[i]
                i += 1
                if token == '|':
                    alternatives.append(current); current = []
                elif token.startswith('{'):
                    actions.append({'rule': name, 'source': token})
                    warn('action', f'Port the preserved semantic action in {name}, including any midrule timing.')
                elif token == '%empty':
                    pass
                elif token in ('%prec', '%dprec', '%merge'):
                    if i >= len(tokens):
                        raise ValueError(f'missing argument for {token}')
                    warn('precedence', f'{token} {tokens[i]} in {name} requires manual migration.')
                    i += 1
                elif token.startswith('%') or token in ('[', ']', '<', '>'):
                    raise ValueError(f'unsupported Bison syntax {token!r}')
                else:
                    current.append(aliases.get(token, literal(ast.literal_eval(token)) if token.startswith(('\"', "'")) else token))
            if i == len(tokens):
                raise ValueError(f'missing semicolon after {name}')
            i += 1
            alternatives.append(current)
            if any(a and a[0] == name for a in alternatives):
                warn('left-recursion', f'{name} is left recursive; select a compatible parser algorithm or rewrite the rule.')
            present = [' '.join(a) for a in alternatives if a]
            expression = ' | '.join('(' + a + ')' for a in present)
            if len(present) < len(alternatives) and expression:
                expression = '(' + expression + ')?'
            lines.extend([f'{name}: {expression};' if expression else f'{name} {{}}', ''])
    else:
        definitions = {}
        clean = re.sub(r'%\{.*?%\}', '', declarations, flags=re.S)
        for line in clean.splitlines():
            match = re.fullmatch(r'([A-Za-z_]\w*)\s+(.+)', line.strip())
            if match:
                definitions[match[1]] = match[2]
            elif line.strip():
                warn('option', f'Review Flex declaration: {line.strip()}')
        if clean != declarations:
            warn('user-code', 'Port the preserved Flex prologue code.')
        def regex(pattern, stack=()):
            result = []
            i = 0
            while i < len(pattern):
                c = pattern[i]
                if c == '{':
                    end = pattern.find('}', i)
                    if end < 0:
                        raise ValueError('unterminated Flex definition reference')
                    name = pattern[i + 1:end]
                    if name not in definitions or name in stack:
                        raise ValueError(f'unsupported repetition or recursive/unknown definition: {{{name}}}')
                    result.append('(' + regex(definitions[name], stack + (name,)) + ')')
                    i = end + 1
                elif c in '["':
                    token = scan(pattern[i:], first=True)[0]
                    if token.startswith('[^') or '[:' in token:
                        raise ValueError('negated/POSIX classes require manual migration')
                    result.append(literal(ast.literal_eval(token)) if token.startswith('"') else token); i += len(token)
                elif c == '\\':
                    if i + 1 >= len(pattern):
                        raise ValueError('trailing regex escape')
                    escape = pattern[i:i + 2]
                    value = {'\\n': '\n', '\\t': '\t', '\\r': '\r'}.get(escape, pattern[i + 1])
                    result.append(literal(value)); i += 2
                elif c == '.':
                    result.append("[^\\n]"); i += 1
                elif c in '()*+?|':
                    result.append(c); i += 1
                elif c in '^$/<>':
                    raise ValueError(f'Flex anchors, trailing context and start conditions require manual migration: {pattern}')
                else:
                    result.append(literal(c)); i += 1
            return ' '.join(result)
        # Patterns end at unquoted whitespace; actions may span multiple lines.
        i, count, names = 0, 0, set()
        while i < len(body):
            while i < len(body) and body[i].isspace(): i += 1
            if i == len(body): break
            start, quoted, bracket = i, False, False
            while i < len(body):
                c = body[i]
                if c == '\\': i += 2; continue
                if c == '"' and not bracket: quoted = not quoted
                if c == '[' and not quoted: bracket = True
                if c == ']' and bracket: bracket = False
                if c.isspace() and not quoted and not bracket: break
                i += 1
            pattern = body[start:i]
            while i < len(body) and body[i] in ' \t': i += 1
            if i < len(body) and body[i] == '{':
                action = scan(body[i:], first=True)[0]; i += len(action)
            else:
                end = body.find('\n', i)
                end = len(body) if end < 0 else end
                action = body[i:end]; i = end
            count += 1
            returned = re.fullmatch(r'\s*\{?\s*return\s+([A-Za-z_]\w*|\'(?:\\.|[^\'])+\')\s*;\s*\}?\s*', action)
            name = returned[1] if returned and not returned[1].startswith("'") else f'FLEX_TOKEN_{count}'
            if name.upper() != name:
                warn('token-name', f'Renamed Flex token {name} to {name.upper()}.')
                name = name.upper()
            if name in names:
                warn('duplicate-token', f'Merge rules returning {name} manually.')
                name = f'FLEX_TOKEN_{count}'
            names.add(name)
            try:
                expression = regex(pattern)
            except ValueError as error:
                warn('pattern', str(error)); continue
            skip = bool(re.fullmatch(r'\s*\{?\s*(?:/\*.*?\*/\s*)*;?\s*\}?\s*', action, re.S))
            if skip: lines.append('#[skip]')
            if not returned and not skip:
                warn('action', f'Port the preserved action for {name}; default ECHO and shared actions need review.')
            if returned and returned[1].startswith("'"):
                warn('literal-token', f'Bind {name} to Bison literal {returned[1]} manually.')
            actions.append({'rule': name, 'pattern': pattern, 'source': action})
            lines.extend([f'{name}: @ ({expression}) {{@}};', ''])
        if not count:
            raise ValueError('no Flex lexer rules found')
        warn('lexer-semantics', 'Validate Flex longest-match and rule priority against the ISPA lexer.')
    return '\n'.join(lines), {'format': kind, 'requires_changes': bool(warnings), 'diagnostics': warnings, 'actions': actions, 'source': source}
