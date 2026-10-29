"""Convert explicitly supplied companion source files without executing them.

Tree-sitter supplies complete compilation-unit trees. Known functions and
constants are linked across supplied files; unknown declarations are retained
in the report, not replaced by silently successful stubs.
"""
import ast
from collections import Counter, defaultdict
from dataclasses import dataclass, field, asdict
import keyword
from pathlib import Path
import re
import symtable
import textwrap

from .actions import TARGETS, Lowerer, UnsupportedAction, canonical_language
from .model import Diagnostic, Span


EXTENSIONS = {
    '.java': 'Java', '.cpp': 'Cpp', '.cc': 'Cpp', '.cxx': 'Cpp', '.h': 'Cpp',
    '.hpp': 'Cpp', '.hxx': 'Cpp', '.cs': 'CSharp', '.js': 'JavaScript',
    '.mjs': 'JavaScript', '.ts': 'TypeScript', '.go': 'Go', '.rs': 'Rust',
    '.swift': 'Swift', '.dart': 'Dart', '.php': 'PHP', '.py': 'Python3',
}
FUNCTIONS = {'function_declaration', 'function_definition', 'function_item',
             'method_declaration', 'function_signature'}
IMPORTS = {'import_declaration', 'import_statement', 'import_or_export',
           'preproc_include', 'using_directive', 'use_declaration',
           'namespace_use_declaration', 'import_from_statement'}
GLOBALS = {'declaration', 'lexical_declaration', 'variable_declaration',
           'const_declaration', 'var_declaration', 'static_item', 'const_item',
           'property_declaration', 'top_level_variable_declaration'}


def identifier(value):
    result = re.sub(r'\W', '_', value)
    if not result or result[0].isdigit() or keyword.iskeyword(result):
        result = 'source_' + result
    return result


@dataclass
class Declaration:
    unit: object
    node: object
    name: str
    qualified: str
    output_name: str
    body: object = None
    parameters: list = field(default_factory=list)
    returns: object = None
    kind: str = 'function'
    error: str = ''
    python: str = ''

    @property
    def target(self):
        return self.unit.module + '.' + self.output_name


class Unit:
    def __init__(self, path, module, language):
        self.path, self.module, self.language = Path(path), module, language
        self.source = self.path.read_text(encoding='utf-8-sig')
        self.data = self.source.encode('utf-8')
        self.diagnostics = []
        self.declarations = []
        self.omitted = []
        self.python = ''

    def span(self, node=None):
        if node is None:
            return Span(str(self.path), 0, len(self.source), 1, 1)
        # Tree-sitter uses byte offsets; reports use Unicode character offsets.
        start = len(self.data[:node.start_byte].decode('utf-8'))
        end = len(self.data[:node.end_byte].decode('utf-8'))
        prefix = self.source[:start]
        return Span(str(self.path), start, end, prefix.count('\n') + 1,
                    len(prefix.rsplit('\n', 1)[-1]) + 1)

    def text(self, node):
        return self.data[node.start_byte:node.end_byte].decode('utf-8')

    def warn(self, code, message, remedy, node=None):
        self.diagnostics.append(Diagnostic(code, message, remedy, self.span(node)))

    def omit(self, node, reason):
        self.warn('companion-code', reason, 'Port this declaration manually; the original source is preserved in the report.', node)
        self.omitted.append({'span': asdict(self.span(node)), 'source': self.text(node), 'reason': reason})

    def ast_node(self, root, node):
        lines = self.data.splitlines(keepends=True)
        start = sum(len(line) for line in lines[:node.lineno - 1]) + node.col_offset
        end = sum(len(line) for line in lines[:node.end_lineno - 1]) + node.end_col_offset
        return root.named_descendant_for_byte_range(start, end)


def field(node, *names):
    for name in names:
        found = node.child_by_field_name(name)
        if found is not None:
            return found
    return None


def primitive_type(unit, node):
    if node is None:
        return None
    value = unit.text(node).strip().removeprefix(':').strip()
    value = re.sub(r'\b(const|final|readonly)\s+', '', value)
    types = {
        'int': 'int', 'short': 'int', 'long': 'int', 'long long': 'int',
        'unsigned': 'int', 'unsigned int': 'int', 'unsigned long': 'int',
        'byte': 'int', 'sbyte': 'int', 'uint': 'int', 'ulong': 'int',
        'int8': 'int', 'int16': 'int', 'int32': 'int', 'int64': 'int',
        'i8': 'int', 'i16': 'int', 'i32': 'int', 'i64': 'int', 'isize': 'int',
        'u8': 'int', 'u16': 'int', 'u32': 'int', 'u64': 'int', 'usize': 'int',
        'Int': 'int', 'Int32': 'int', 'Int64': 'int', 'UInt': 'int',
        'float': 'float', 'double': 'float', 'float32': 'float', 'float64': 'float',
        'f32': 'float', 'f64': 'float', 'Float': 'float', 'Double': 'float',
        'number': 'float', 'num': 'float', 'bool': 'bool', 'boolean': 'bool', 'Bool': 'bool',
        'String': 'str', 'string': 'str', 'str': 'str', 'std::string': 'str',
        'void': 'None', 'Void': 'None', '()': 'None',
    }
    if value not in types:
        raise UnsupportedAction(f'type {value!r} needs a Python/ISPA type mapping')
    return types[value]


def signature(unit, node, scope, body=None):
    declarator = field(node, 'declarator')
    function = declarator if declarator is not None and declarator.type == 'function_declarator' else node
    name_node = field(function, 'name', 'declarator')
    if name_node is None or name_node.type not in ('identifier', 'name', 'simple_identifier', 'qualified_identifier'):
        raise UnsupportedAction('function name/declarator requires manual conversion')
    name = unit.text(name_node)
    if not name.isidentifier() or keyword.iskeyword(name):
        raise UnsupportedAction(f'function name {name!r} requires manual conversion')
    params = field(function, 'parameters')
    if params is None:
        params = next((c for c in node.named_children if c.type == 'formal_parameter_list'), None)
    parameter_nodes = params.named_children if params is not None else [c for c in node.named_children if c.type == 'parameter']
    parameters = []
    for param in parameter_nodes:
        if param.type in ('comment', 'line_comment'):
            continue
        if param.type in ('optional_parameter', 'variadic_parameter', 'rest_pattern', 'optional_parameter_declaration') or field(param, 'value', 'default_value') is not None:
            raise UnsupportedAction('default/variadic parameters require manual conversion')
        param_name = field(param, 'name', 'pattern', 'declarator')
        if param_name is None and param.type == 'identifier':
            param_name = param
        if param_name is None:
            raise UnsupportedAction('destructured or unnamed parameter requires manual conversion')
        value = unit.text(param_name).lstrip('$')
        if not value.isidentifier() or keyword.iskeyword(value):
            raise UnsupportedAction(f'parameter {value!r} requires renaming')
        type_node = field(param, 'type')
        if type_node is None and unit.language in ('Swift', 'Dart'):
            type_node = next((c for c in param.named_children if c.type in ('user_type', 'type_identifier', 'predefined_type')), None)
        parameters.append((value, primitive_type(unit, type_node)))
    returns = field(node, 'return_type', 'returns', 'result', 'type')
    if unit.language == 'Swift':
        returns = next((c for c in node.named_children if c.type == 'user_type'), None)
    elif unit.language == 'Dart':
        returns = next((c for c in node.named_children if c.type in ('type_identifier', 'void_type')), None)
    return_type = primitive_type(unit, returns)
    if returns is None and unit.language in ('Go', 'Rust', 'Swift'):
        return_type = 'None'
    qualified = '.'.join((*scope, name))
    return Declaration(unit, node, name, qualified, identifier('_'.join((*scope, name))),
                       body or field(node, 'body'), parameters, return_type)


def collect(unit, node, scope=()):
    children = node.named_children
    index = 0
    while index < len(children):
        child = children[index]
        index += 1
        kind = child.type
        if kind in ('comment', 'line_comment', 'block_comment', 'php_tag', 'package_clause', 'package_declaration'):
            continue
        if kind in IMPORTS:
            unit.omitted.append({'span': asdict(unit.span(child)), 'source': unit.text(child), 'reason': 'external dependency'})
            unit.warn('companion-import', f'Imported dependency has no automatic mapping: {unit.text(child).strip()}',
                      'Provide its source with --code or port/map the library API; the foreign import is not executed or emitted as a guessed Python import.', child)
        elif kind in ('export_statement',):
            collect(unit, child, scope)
        elif kind == 'namespace_definition':
            name, body = field(child, 'name'), field(child, 'body')
            if name is None or body is None:
                unit.omit(child, 'anonymous namespace requires manual scope conversion')
            else:
                collect(unit, body, (*scope, unit.text(name)))
        elif kind in ('class_declaration', 'class_specifier'):
            name, body = field(child, 'name'), field(child, 'body')
            if name is None or body is None:
                unit.omit(child, 'unsupported class shape')
                continue
            # Static methods have no instance state. Flatten only those; never
            # guess how an ANTLR Parser superclass or object lifetime works.
            class_scope = (*scope, unit.text(name))
            for member in body.named_children:
                if member.type in ('comment', 'line_comment', 'access_specifier'):
                    continue
                if member.type in FUNCTIONS and re.search(r'\bstatic\b', unit.text(member).split('{', 1)[0]):
                    try:
                        unit.declarations.append(signature(unit, member, class_scope))
                    except UnsupportedAction as error:
                        unit.omit(member, str(error))
                else:
                    unit.omit(member, 'instance state, inheritance or non-static class member requires manual conversion')
        elif kind in FUNCTIONS:
            body = None
            if kind == 'function_signature' and index < len(children) and children[index].type == 'function_body':
                body = children[index]
                index += 1
            try:
                declaration = signature(unit, child, scope, body)
                if declaration.body is None:
                    raise UnsupportedAction('function without a body requires its implementation')
                unit.declarations.append(declaration)
            except UnsupportedAction as error:
                unit.omit(child, str(error))
        elif kind in GLOBALS:
            # Only a single initialized scalar declaration is accepted here.
            lowerer = Lowerer(unit.data, unit.language, {})
            try:
                python = lowerer.statement(child)
                tree = ast.parse(python)
                if len(tree.body) != 1 or not isinstance(tree.body[0], ast.Assign) or len(tree.body[0].targets) != 1 or not isinstance(tree.body[0].targets[0], ast.Name) or not isinstance(tree.body[0].value, ast.Constant):
                    raise UnsupportedAction('global initializer must be a scalar constant')
                name = tree.body[0].targets[0].id
                declaration = Declaration(unit, child, name, '.'.join((*scope, name)), identifier('_'.join((*scope, name))), kind='constant')
                declaration.python = f'{declaration.output_name} = {ast.unparse(tree.body[0].value)}'
                unit.declarations.append(declaration)
            except (UnsupportedAction, SyntaxError) as error:
                unit.omit(child, str(error))
        else:
            unit.omit(child, f'unsupported {unit.language} top-level construct {kind}')


def symbols_for(declarations, unit=None, current=None):
    candidates = defaultdict(set)
    for declaration in declarations:
        target = declaration.output_name if declaration.unit is unit else declaration.target
        aliases = {declaration.qualified, declaration.qualified.replace('.', '::'),
                   declaration.unit.path.stem + '.' + declaration.qualified,
                   declaration.unit.path.stem + '::' + declaration.qualified}
        # Unqualified static method calls resolve within their own class only.
        if '.' not in declaration.qualified or (current and current.qualified.rpartition('.')[0] == declaration.qualified.rpartition('.')[0]):
            aliases.add(declaration.name)
        for alias in aliases:
            candidates[alias].add(target)
    return {name: next(iter(targets)) for name, targets in candidates.items() if len(targets) == 1}


def lower_function(declaration, symbols):
    unit = declaration.unit
    lowerer = Lowerer(unit.data, unit.language, {}, symbols, allow_return=True)
    # Parameter/local names shadow globals and function names.
    local_names = {name for name, _ in declaration.parameters}
    pending = [declaration.body]
    while pending:
        node = pending.pop()
        pending.extend(node.named_children)
        if node.type in ('variable_declarator', 'init_declarator', 'let_declaration', 'property_declaration', 'initialized_variable_definition'):
            name = field(node, 'name', 'declarator', 'pattern')
            if name is not None:
                local_names.add(unit.text(name))
        elif node.type == 'short_var_declaration':
            left = field(node, 'left')
            if left is not None:
                local_names.update(unit.text(n) for n in left.named_children if n.type == 'identifier')
    for name in local_names:
        lowerer.symbols = {k: v for k, v in lowerer.symbols.items() if k != name and not k.startswith(name + '.')}
    body = declaration.body
    if unit.language == 'Rust' and body.named_children and body.named_children[-1].type not in ('expression_statement', 'let_declaration', 'return_expression', 'empty_statement'):
        items = [lowerer.statement(c) for c in body.named_children[:-1]]
        items.append('return ' + lowerer.expr(body.named_children[-1]))
        python = '\n'.join(items)
    else:
        python = lowerer.statement(body)
    params = ', '.join(name + (': ' + annotation if annotation else '') for name, annotation in declaration.parameters)
    returns = ' -> ' + declaration.returns if declaration.returns else ''
    result = f'def {declaration.output_name}({params}){returns}:\n' + textwrap.indent(python or 'pass', '    ')
    # Compile validates return/break/continue scope as well as syntax. Never run.
    compile(result, str(unit.path), 'exec')
    scope = symtable.symtable(result, str(unit.path), 'exec').get_children()[0]
    allowed = {value.split('.')[0] for value in symbols.values()} | {'print', 'int', 'float', declaration.output_name}
    unknown = {symbol.get_name() for symbol in scope.get_symbols() if symbol.is_global() and symbol.is_referenced() and symbol.get_name() not in allowed}
    if unknown:
        raise UnsupportedAction('unresolved global/library names: ' + ', '.join(sorted(unknown)))
    return result


def python_unit(unit, root, local_modules):
    """Python is already the destination language; preserve it with diagnostics."""
    if root.has_error:
        unit.omit(root, 'Tree-sitter reported invalid Python source')
        return
    try:
        tree = ast.parse(unit.source)
        compile(tree, str(unit.path), 'exec')
    except SyntaxError as error:
        unit.omit(root, str(error))
        return
    for node in ast.walk(tree):
        if isinstance(node, (ast.Import, ast.ImportFrom)):
            unresolved = []
            if isinstance(node, ast.Import):
                for alias in node.names:
                    if alias.name in local_modules:
                        old = alias.name
                        alias.name = local_modules[old]
                        alias.asname = alias.asname or old
                    else:
                        unresolved.append(alias.name)
            elif node.level == 0 and node.module in local_modules:
                node.module = local_modules[node.module]
            else:
                unresolved.append(ast.unparse(node))
            if unresolved:
                unit.warn('companion-import', 'Python import needs a generator/library mapping: ' + ', '.join(unresolved),
                          'Supply the imported Python module and verify that ISPA can lower its dependencies.', unit.ast_node(root, node))
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            if node.returns is None or any(p.annotation is None for p in (*node.args.posonlyargs, *node.args.args, *node.args.kwonlyargs) if p.arg not in ('self', 'cls')):
                unit.warn('companion-types', f'{node.name} lacks complete type annotations', 'Add parameter and return annotations for ISPA cross-target lowering.', unit.ast_node(root, node))
        if isinstance(node, (ast.AsyncFunctionDef, ast.Await, ast.Yield, ast.YieldFrom)):
            unit.warn('companion-code', 'Async/generator code requires generator support', 'Port this construct or confirm that the selected ISPA target supports it.', unit.ast_node(root, node))
    for node in tree.body:
        if isinstance(node, (ast.FunctionDef, ast.ClassDef)):
            unit.declarations.append(Declaration(unit, unit.ast_node(root, node), node.name, node.name, node.name, kind='python', python=ast.unparse(node)))
    unit.python = ast.unparse(tree) + '\n'
    top_scope = symtable.symtable(unit.python, str(unit.path), 'exec')
    # Python source is retained, but unresolved names must not be presented as
    # successfully linked merely because Python's syntax parser accepts them.
    import builtins
    known = {symbol.get_name() for symbol in top_scope.get_symbols() if symbol.is_assigned() or symbol.is_imported() or symbol.is_namespace()}
    known.update(dir(builtins))
    def check_scope(scope):
        unknown = {symbol.get_name() for symbol in scope.get_symbols() if symbol.is_global() and symbol.is_referenced() and symbol.get_name() not in known}
        if unknown:
            unit.warn('companion-library', 'Unresolved Python global/library names: ' + ', '.join(sorted(unknown)),
                      'Provide the missing definitions or map the external API before generating the parser.')
        for child in scope.get_children():
            check_scope(child)
    check_scope(top_scope)


@dataclass
class CompanionResult:
    units: list
    symbols: dict

    @property
    def diagnostics(self):
        return [d for unit in self.units for d in unit.diagnostics]

    @property
    def files(self):
        return {unit.module + '.py': unit.python for unit in self.units}

    @property
    def imports(self):
        return ['import py ' + unit.module + ';' for unit in self.units]

    def report(self):
        return [{'source': str(unit.path), 'language': unit.language, 'module': unit.module,
                 'python': unit.python, 'omitted': unit.omitted,
                 'declarations': [{'name': d.qualified, 'target': d.target, 'converted': not bool(d.error),
                                   'reason': d.error, 'span': asdict(unit.span(d.node))}
                                  for d in unit.declarations]}
                for unit in self.units]


def convert_companions(paths, fallback_language='Java'):
    units, used = [], set()
    for path in dict.fromkeys(Path(p).resolve() for p in paths):
        base = 'antlr_' + identifier(path.stem).lower()
        module, index = base, 2
        while module in used:
            module = f'{base}_{index}'
            index += 1
        used.add(module)
        language = EXTENSIONS.get(path.suffix.lower(), canonical_language(fallback_language))
        unit = Unit(path, module, language)
        units.append(unit)
    stem_counts = Counter(unit.path.stem for unit in units)
    local_modules = {unit.path.stem: unit.module for unit in units if stem_counts[unit.path.stem] == 1}
    for unit in units:
        language = unit.language
        try:
            from tree_sitter_language_pack import get_parser
            root = get_parser(TARGETS[language][0]).parse(unit.data).root_node
        except (ImportError, LookupError, KeyError) as error:
            unit.warn('companion-parser', str(error), 'Install the language parsers or select a supported source language.')
            unit.python = '# Source could not be parsed; see the migration report.\n'
            continue
        if language == 'Python3':
            python_unit(unit, root, local_modules)
        elif root.has_error:
            unit.omit(root, 'Tree-sitter reported invalid or unsupported compilation-unit syntax')
        else:
            collect(unit, root)
    declarations = [d for unit in units for d in unit.declarations]
    # Duplicate/overloaded names cannot be resolved without type analysis.
    counts = Counter((d.unit.module, d.output_name) for d in declarations)
    for declaration in declarations:
        if counts[declaration.unit.module, declaration.output_name] > 1:
            declaration.error = 'overloaded or colliding declaration requires explicit renaming'
    active = [d for d in declarations if not d.error]
    while True:
        failed = []
        for declaration in active:
            if declaration.kind != 'function':
                continue
            try:
                declaration.python = lower_function(declaration, symbols_for(active, declaration.unit, declaration))
            except (UnsupportedAction, SyntaxError, ValueError) as error:
                declaration.error = str(error)
                failed.append(declaration)
        if not failed:
            break
        active = [d for d in active if not d.error]
    for declaration in declarations:
        unit = declaration.unit
        if declaration.error:
            unit.omit(declaration.node, f'{declaration.qualified}: {declaration.error}')
        elif declaration.kind == 'function':
            if declaration.returns is None or any(t is None for _, t in declaration.parameters):
                unit.warn('companion-types', f'{declaration.qualified} lacks source type annotations',
                          'Add explicit Python parameter/return annotations before ISPA cross-target generation.', declaration.node)
    for unit in units:
        if unit.language == 'Python3':
            continue
        converted = [d for d in active if d.unit is unit]
        code = '\n\n'.join(d.python for d in converted)
        imports = []
        if code:
            names = {n.id for n in ast.walk(ast.parse(code)) if isinstance(n, ast.Name)}
            imports = ['import ' + other.module for other in units if other is not unit and other.module in names]
            unit.warn('companion-semantics', 'Foreign source lowered to Python semantics',
                      'Review source numeric widths, coercions, exceptions, and library behavior before generating another target.')
        comments = ['# Converted companion source. See the migration report.']
        for omitted in unit.omitted:
            comments.append('# TODO: ' + omitted['reason'].replace('\n', ' '))
        unit.python = '\n'.join(comments + imports) + '\n\n' + code + '\n'
        compile(unit.python, str(unit.path), 'exec')
    return CompanionResult(units, symbols_for(active))
