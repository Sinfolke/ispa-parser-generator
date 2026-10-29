"""Grammar dependency loading and static checks (without executing actions)."""
from pathlib import Path
from .model import Diagnostic, walk
from .parser import parse


def load(path, include=(), active=None, loaded=None):
    path = Path(path).resolve()
    active = set() if active is None else active
    loaded = {} if loaded is None else loaded
    if path in loaded:
        return loaded[path]
    grammar = parse(path.read_text(encoding='utf-8-sig'), str(path))
    active.add(path)
    dependencies = list(grammar.imports)
    if 'tokenVocab' in grammar.options:
        dependencies.append(grammar.options['tokenVocab'])
    own_names = {r.name for r in grammar.rules}
    for name in dependencies:
        candidates = [directory / (name + '.g4') for directory in (path.parent, *(Path(p) for p in include))]
        dependency = next((p.resolve() for p in candidates if p.is_file()), None)
        span = grammar.rules[0].span if grammar.rules else grammar.declarations[0].span if grammar.declarations else None
        if span is None:
            from .model import Span
            span = Span(str(path), 0, 0, 1, 1)
        if dependency is None or dependency in active:
            grammar.diagnostics.append(Diagnostic('dependency', f'Cannot resolve dependency {name}' if dependency is None else f'Cyclic dependency {name}', 'Supply the .g4 dependency with -I, or resolve the import cycle.', span))
            continue
        imported = load(dependency, include, active, loaded)
        grammar.diagnostics.extend(imported.diagnostics)
        for rule in imported.rules:
            if rule.name not in own_names:
                grammar.rules.append(rule)
                own_names.add(rule.name)
        grammar.declarations.extend(imported.declarations)
    active.remove(path)
    loaded[path] = grammar
    return grammar


def analyze(grammar):
    diagnostics = list(grammar.diagnostics)
    symbols = {}
    for rule in grammar.rules:
        if rule.name in symbols:
            diagnostics.append(Diagnostic('duplicate-rule', f'Duplicate rule {rule.name}', 'Rename or remove the duplicate definition.', rule.span))
        symbols[rule.name] = rule
    nullable = set()
    def empty(node):
        if node.kind in ('action', 'predicate', 'command', 'alt_label'):
            return True
        if node.kind == 'ref':
            return node.value in nullable or node.value == 'EOF'
        if node.kind == 'sequence':
            return all(empty(c) for c in node.children)
        if node.kind == 'choice':
            return any(empty(c) for c in node.children)
        if node.kind == 'repeat':
            return node.value[0] in '?*' or empty(node.children[0])
        return bool(node.children) and empty(node.children[-1]) if node.kind in ('group', 'label', 'list_label', 'element_options', 'call') else False
    while True:
        updated = nullable | {r.name for r in grammar.rules if empty(r.body)}
        if updated == nullable:
            break
        nullable = updated
    for rule in grammar.rules:
        if rule.lexer and rule.name in nullable:
            diagnostics.append(Diagnostic('nullable-lexer', f'Lexer rule {rule.name} can match empty input', 'Make every lexer token consume at least one character.', rule.span))
        for node in walk(rule.body):
            if node.kind == 'ref' and node.value != 'EOF' and node.value not in symbols:
                diagnostics.append(Diagnostic('undefined-rule', f'Undefined reference {node.value}', 'Supply the defining grammar or add an ISPA rule.', node.span))
            if node.kind == 'repeat' and node.value[0] in '*+' and empty(node.children[0]):
                diagnostics.append(Diagnostic('nullable-repeat', f'Repetition in {rule.name} may not consume input', 'Remove the nullable branch from the repeated expression.', node.span))
    # Recursion is retained verbatim; no elimination or precedence rewriting.
    return diagnostics
