"""Emit ISPA and a lossless migration report from the grammar IR."""
import ast
from dataclasses import asdict
from .actions import translate, canonical_language
from .analysis import analyze
from .model import Diagnostic, walk


class Emitter:
    def __init__(self, grammar, language=None, companions=None):
        self.grammar = grammar
        self.language = canonical_language(language or grammar.options.get('language', 'Java'))
        self.diagnostics = analyze(grammar)
        self.companions = companions
        self.symbols = companions.symbols if companions else {}
        if companions:
            self.diagnostics.extend(companions.diagnostics)
        self.actions = []
        self.current = None
        tokens = {r.name for r in grammar.rules if r.lexer}
        tokens.update(n.value for r in grammar.rules for n in walk(r.body)
                      if n.kind == 'ref' and n.value[0].isupper() and n.value != 'EOF')
        # Reserve existing uppercase names before allocating renamed tokens.
        used = {name for name in tokens if name == name.upper()}
        self.token_names = {name: name for name in used}
        for name in sorted(tokens - used):
            candidate = name.upper()
            index = 2
            while candidate in used:
                candidate = f'{name.upper()}_{index}'
                index += 1
            self.token_names[name] = candidate
            used.add(candidate)

    def symbol(self, name):
        return self.token_names.get(name, name)

    def warn(self, code, message, remedy, span):
        self.diagnostics.append(Diagnostic(code, message, remedy, span))

    def expression(self, node):
        kind = node.kind
        if kind in ('sequence', 'choice'):
            pieces = [self.expression(c) for c in node.children]
            if kind == 'choice':
                present = [p for p in pieces if p]
                choice = ' | '.join('(' + p + ')' for p in present)
                return '(' + choice + ')?' if choice and len(present) != len(pieces) else choice
            return ' '.join(p for p in pieces if p)
        if kind == 'group':
            return '(' + ' '.join(self.expression(c) for c in node.children) + ')'
        if kind == 'ref':
            if node.value == 'EOF':
                self.warn('eof', 'Explicit EOF assertion requires review', 'Require full input consumption at the parser entry point.', node.span)
                return ''
            return self.symbol(node.value)
        if kind in ('literal', 'charset'):
            if '\\p{' in node.value or '\\P{' in node.value or '\\u{' in node.value:
                self.warn('unicode-set', 'Unicode property/braced escape is target-dependent', 'Expand Unicode properties and braced escapes into ISPA-supported ranges.', node.span)
            return node.value
        if kind == 'range':
            try:
                endpoints = [ast.literal_eval(c.value) for c in node.children]
                if any(len(c) != 1 for c in endpoints):
                    raise ValueError()
                return '[' + '-'.join(c.replace('\\', '\\\\').replace(']', '\\]').replace('-', '\\-').replace('^', '\\^') for c in endpoints) + ']'
            except (ValueError, SyntaxError):
                self.warn('range', 'Range cannot be represented safely', 'Replace this range with an explicit ISPA character group.', node.span)
                return '(' + ' | '.join(c.value for c in node.children) + ')'
        if kind == 'any':
            return '[*]'
        if kind == 'not':
            return '!(' + self.expression(node.children[0]) + ')'
        if kind == 'repeat':
            if len(node.value) > 1:
                self.warn('nongreedy', 'ANTLR non-greedy quantifier retained as greedy approximation', 'Rewrite the expression with an explicit terminator/negative match.', node.span)
            return '(' + self.expression(node.children[0]) + ')' + node.value[0]
        if kind in ('label', 'list_label'):
            self.warn('capture', f'Capture {node.value} requires AST shaping', 'Add an ISPA capture and AST field where this label is consumed.', node.span)
            return self.expression(node.children[0])
        if kind in ('element_options', 'call'):
            self.warn(kind, f'ANTLR {kind} {node.value} needs migration', 'Port rule arguments/element options to ISPA templates or advanced-rule parameters.', node.span)
            return self.expression(node.children[0])
        if kind == 'alt_label':
            self.warn('alternative-label', f'Alternative label #{node.value} has no automatic visitor equivalent', 'Define the desired ISPA AST shape and update visitor consumers.', node.span)
            return ''
        if kind in ('action', 'predicate'):
            result = translate(node.value, self.language, predicate=kind == 'predicate', symbols=self.symbols)
            self.actions.append({'rule': self.current.name, 'kind': kind, 'span': asdict(node.span), **asdict(result), 'integrated': False})
            self.warn('action' if kind == 'action' else 'predicate', result.reason or 'Translated Python needs binding at this grammar position', 'Integrate the preserved action in an ISPA advanced rule; preserve predicate evaluation before matching.' if not result.reason else 'Translate the preserved source and map its runtime/attribute dependencies.', node.span)
            return ''
        if kind == 'command':
            if node.value != 'skip':
                self.warn('lexer-command', f'Lexer command {node.value} requires migration', 'Implement token type/channel/mode state in an advanced lexer rule.', node.span)
            return ''
        self.warn('unsupported', f'Unsupported grammar construct {node.value}', 'Port this construct manually from the original grammar.', node.span)
        return ''

    def advanced(self, rule):
        """Integrate straight-line actions with explicit local captures.

        Branches, predicates, lexer actions and parameter declarations need
        runtime bindings and remain in the report, not silently rearranged.
        """
        if rule.lexer or rule.headers or rule.body.kind != 'sequence':
            return None
        if not any(n.kind == 'action' for n in rule.body.children):
            return None
        allowed = {'ref', 'literal', 'charset', 'range', 'any', 'label', 'action'}
        if any(n.kind not in allowed for n in walk(rule.body) if n is not rule.body):
            return None
        bindings, statements, records = {}, [], []
        referenced = {}
        for node in rule.body.children:
            if node.kind == 'ref':
                referenced[node.value] = referenced.get(node.value, 0) + 1
        for node in rule.body.children:
            if node.kind == 'action':
                result = translate(node.value, self.language, bindings, symbols=self.symbols)
                if result.reason:
                    return None
                statements.extend(result.python.splitlines())
                records.append({'rule': rule.name, 'kind': 'action', 'span': asdict(node.span), **asdict(result), 'integrated': True})
            elif node.kind == 'label':
                child = node.children[0]
                if child.kind != 'ref' or not child.value[0].isupper():
                    return None
                name = 'antlr_' + node.value
                statements.append(f'match @ {self.symbol(child.value)} -> {name};')
                bindings[node.value + '.text'] = name + '.value.value'
                bindings[node.value + '.int'] = f'int({name}.value.value)'
            elif node.kind == 'ref' and node.value == 'EOF':
                return None
            elif node.kind == 'ref' and node.value[0].isupper() and referenced[node.value] == 1:
                name = 'antlr_' + node.value
                statements.append(f'match @ {self.symbol(node.value)} -> {name};')
                bindings[node.value + '.text'] = name + '.value.value'
                bindings[node.value + '.int'] = f'int({name}.value.value)'
            else:
                statements.append('match ' + self.expression(node) + ';')
        self.actions.extend(records)
        for record in records:
            tree = ast.parse(record['python'])
            if any(isinstance(n, ast.Call) and isinstance(n.func, ast.Name) and n.func.id in ('int', 'float', 'str', 'bool', 'list', 'dict', 'len', 'tuple') for n in ast.walk(tree)):
                self.warn('builtin-lowering', 'Translated action uses Python builtins that need target validation', 'Check builtin lowering in your ISPA generator: the current Python target can emit renamed builtin calls such as int_; supply a helper or fix that target mapping.', rule.span)
        if self.language != 'Python3':
            self.warn('action-semantics', 'Foreign action lowered to Python semantics', 'Review numeric overflow, coercions, string formatting and exception behavior in translated actions.', rule.span)
        return rule.name + ' {\n' + '\n'.join('    ' + line for line in statements) + '\n}'

    def emit(self):
        lines = ['// Converted from ANTLR 4. See the migration report before use.', '']
        if self.companions:
            lines.extend(self.companions.imports + [''])
        for rule in self.grammar.rules:
            self.current = rule
            if rule.mode != 'DEFAULT_MODE':
                self.warn('lexer-mode', f'{rule.name} belongs to lexer mode {rule.mode}', 'Restore mode-dependent token activation before using this lexer.', rule.span)
            for header in rule.headers:
                self.warn('rule-header', f'Rule metadata/action {header.text} requires migration', 'Port parameters, returns, locals and lifecycle/exception actions into an advanced rule.', header.span)
                if header.kind == 'ACTION':
                    result = translate(header.text[1:-1], self.language, symbols=self.symbols)
                    self.actions.append({'rule': rule.name, 'kind': 'header', 'span': asdict(header.span), **asdict(result), 'integrated': False})
            advanced = self.advanced(rule)
            if advanced is not None:
                lines.extend((advanced, ''))
                continue
            body = self.expression(rule.body)
            if rule.fragment:
                lines.append('#[inline]')
            commands = [n.value for n in walk(rule.body) if n.kind == 'command']
            if commands == ['skip'] and rule.body.kind == 'sequence':
                lines.append('#[skip]')
            elif 'skip' in commands:
                self.warn('conditional-skip', 'Skip applies to only some alternatives', 'Split skipped alternatives into a separate lexer rule.', rule.span)
            # Lexer text values are needed for ANTLR $token.text bindings.
            if rule.lexer:
                lines.append(f'{self.symbol(rule.name)}: @ ({body}) {{@}};')
            else:
                lines.append(f'{rule.name}: {body};' if body else f'{rule.name} {{}}')
            lines.append('')
        for declaration in self.grammar.declarations:
            self.warn('declaration', 'ANTLR declaration requires migration', 'Port named actions, token/channel declarations, and member state into ISPA equivalents.', declaration.span)
            if declaration.kind == 'ACTION':
                result = translate(declaration.text[1:-1], self.language, symbols=self.symbols)
                self.actions.append({'rule': None, 'kind': 'declaration', 'span': asdict(declaration.span), **asdict(result), 'integrated': False})
        for key, value in self.grammar.options.items():
            if key not in ('language', 'tokenVocab'):
                span = self.grammar.rules[0].span if self.grammar.rules else None
                self.warn('option', f'ANTLR option {key}={value} requires review', 'Implement this option explicitly in the converted grammar or generator configuration.', span)
        if self.diagnostics:
            lines.extend(['// Changes required before running this grammar:'] + [f'// [{d.code}] {d.remedy}' for d in self.diagnostics])
        report = {'grammar': self.grammar.name, 'language': self.language,
                  'token_names': self.token_names,
                  'requires_changes': bool(self.diagnostics),
                  'diagnostics': [d.to_dict() for d in self.diagnostics], 'actions': self.actions}
        if self.companions:
            report['companions'] = self.companions.report()
        return '\n'.join(lines) + '\n', report
