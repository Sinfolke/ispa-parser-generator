"""Tree-sitter based, conservative lowering of foreign actions to Python AST.

There are no language grammars here. Each adapter selects an upstream parser
and a syntactic wrapper for a statement fragment. Unsupported nodes abort the
whole action, never leaving a partially executed translation.
"""
import ast
import keyword
import re
import textwrap
from dataclasses import dataclass


TARGETS = {
    'Java': ('java', 'class A { void f() {\n', '\n} }'),
    'Cpp': ('cpp', 'void f() {\n', '\n}'),
    'CSharp': ('csharp', 'class A { void F() {\n', '\n} }'),
    'JavaScript': ('javascript', 'function f() {\n', '\n}'),
    'TypeScript': ('typescript', 'function f() {\n', '\n}'),
    'Go': ('go', 'package p\nfunc f() {\n', '\n}'),
    'Rust': ('rust', 'fn f() {\n', '\n}'),
    'Swift': ('swift', 'func f() {\n', '\n}'),
    'Dart': ('dart', 'void f() {\n', '\n}'),
    'PHP': ('php', '<?php function f() {\n', '\n}'),
    'Python3': ('python', '', ''),
}
ALIASES = {'c++': 'Cpp', 'c#': 'CSharp', 'python': 'Python3', 'python3': 'Python3',
           **{k.lower(): k for k in TARGETS}}


class UnsupportedAction(ValueError):
    pass


def canonical_language(language):
    return ALIASES.get(language.lower(), language)


@dataclass
class ActionResult:
    source: str
    python: str = ''
    reason: str = ''


def attributes(code, bindings, php=False):
    """Rewrite ANTLR references outside strings/comments, keeping PHP variables.

Bindings are explicit; an unknown ANTLR attribute must never become an
unbound Python name. PHP variables that aren't ANTLR bindings are handled by
the PHP AST adapter.
"""
    pattern = re.compile(r'''("""[\s\S]*?"""|''' + "'''[\\s\\S]*?'''" + r'''|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|`(?:\\.|[^`\\])*`|//[^\n]*|/\*[\s\S]*?\*/)|(\$([A-Za-z_]\w*)(?:\.([A-Za-z_]\w*))?)''')
    replacements = {}
    def replace(match):
        if match.group(1):
            return match.group(0)
        name, attr = match.group(3), match.group(4)
        key = name + ('.' + attr if attr else '')
        if key not in bindings:
            if php and not attr:
                return match.group(0)
            raise UnsupportedAction(f'unresolved ANTLR reference ${key}')
        marker = 'ispa_ref_' + str(len(replacements))
        replacements[marker] = bindings[key]
        return marker
    return pattern.sub(replace, code), replacements


class Lowerer:
    def __init__(self, source, language, replacements, symbols=None, allow_return=False):
        self.source, self.language, self.replacements = source, language, replacements
        self.symbols = symbols or {}
        self.allow_return = allow_return

    def text(self, node):
        return self.source[node.start_byte:node.end_byte].decode('utf-8')

    def field(self, node, *names):
        for name in names:
            result = node.child_by_field_name(name)
            if result is not None:
                return result
        return None

    def children(self, node):
        return [c for c in node.named_children if c.type not in ('comment', 'line_comment', 'block_comment')]

    def call(self, name, arguments):
        if name in self.symbols:
            return self.symbols[name] + '(' + ', '.join(self.expr(a) for a in arguments) + ')'
        mappings = {
            'Java': {'System.out.println': 'print', 'Integer.parseInt': 'int', 'Double.parseDouble': 'float'},
            'Cpp': {'std::stoi': 'int', 'std::stol': 'int', 'std::stod': 'float'},
            'CSharp': {'Console.WriteLine': 'print', 'System.Console.WriteLine': 'print', 'int.Parse': 'int', 'Int32.Parse': 'int', 'double.Parse': 'float'},
            'JavaScript': {'console.log': 'print'},
            'TypeScript': {'console.log': 'print'},
            'Go': {'fmt.Println': 'print'},
            'Swift': {'print': 'print'},
            'Dart': {'print': 'print'},
            'PHP': {}, 'Rust': {},
        }
        target = mappings.get(self.language, {}).get(name)
        if target is None:
            raise UnsupportedAction(f'call {name} needs a library/runtime mapping')
        if target != 'print' and len(arguments) != 1:
            raise UnsupportedAction(f'overload of {name} needs an explicit mapping')
        return target + '(' + ', '.join(self.expr(a) for a in arguments) + ')'

    def expr(self, node):
        if node is None:
            raise UnsupportedAction('missing expression')
        kind, value, children = node.type, self.text(node), self.children(node)
        if value in self.symbols and kind in ('identifier', 'simple_identifier', 'name', 'variable_name', 'field_access', 'member_expression', 'selector_expression', 'qualified_identifier'):
            return self.symbols[value]
        if kind in ('identifier', 'simple_identifier', 'name', 'variable_name'):
            name = value.lstrip('$')
            if name in self.replacements:
                return self.replacements[name]
            if keyword.iskeyword(name) or not name.isidentifier():
                raise UnsupportedAction(f'identifier {name!r} needs renaming')
            return name
        if value in ('true', 'false', 'null', 'nil', 'nullptr', 'NULL'):
            return {'true': 'True', 'false': 'False'}.get(value, 'None')
        if kind in ('integer_literal', 'decimal_integer_literal', 'int_literal', 'integer', 'number', 'number_literal', 'real_literal', 'float_literal', 'decimal_floating_point_literal'):
            if not re.fullmatch(r'(?:0[xX][0-9a-fA-F]+|[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?)', value):
                raise UnsupportedAction(f'numeric literal {value!r} requires explicit conversion')
            return repr(ast.literal_eval(value))
        if kind in ('string', 'string_literal', 'string_value', 'interpreted_string_literal', 'encapsed_string'):
            if not re.fullmatch(r'''["'][^\\\r\n$]*["']''', value):
                raise UnsupportedAction('escaped/interpolated string needs semantic review')
            return repr(value[1:-1])
        if kind in ('parenthesized_expression', 'condition_clause', 'assignable_expression', 'directly_assignable_expression', 'pattern', 'expression_list', 'argument', 'value_argument') and len(children) == 1:
            return self.expr(children[0])
        if kind in ('call_expression', 'invocation_expression', 'method_invocation', 'function_call_expression'):
            function = self.field(node, 'function', 'name')
            arguments = self.field(node, 'arguments')
            if kind == 'method_invocation':
                receiver = self.field(node, 'object')
                name = (self.text(receiver) + '.' if receiver else '') + self.text(function)
            else:
                function = function or (children[0] if children else None)
                name = self.text(function) if function else ''
            if arguments is None:
                def find_arguments(current):
                    if current.type in ('argument_list', 'arguments', 'value_arguments'):
                        return current
                    for child in current.named_children:
                        found = find_arguments(child)
                        if found is not None:
                            return found
                    return None
                arguments = find_arguments(node)
            if arguments is None:
                raise UnsupportedAction('call has an unsupported argument shape')
            return self.call(name, self.children(arguments))
        if kind == 'macro_invocation' and self.language == 'Rust':
            macro = self.field(node, 'macro')
            if macro is not None and self.text(macro) == 'println':
                tokens = children[-1]
                args = self.children(tokens)
                if len(args) == 1 and args[0].type == 'string_literal' and '{' not in self.text(args[0]):
                    return 'print(' + self.expr(args[0]) + ')'
            raise UnsupportedAction('Rust macro requires expansion or an explicit mapping')
        if kind in ('binary_expression', 'binary_operator', 'additive_expression', 'multiplicative_expression', 'comparison_expression', 'equality_expression', 'conjunction_expression', 'disjunction_expression', 'relational_expression'):
            left = self.field(node, 'left', 'lhs') or children[0]
            right = self.field(node, 'right', 'rhs') or children[-1]
            operator = self.source[left.end_byte:right.start_byte].decode().strip()
            operators = {'&&': 'and', '||': 'or', '===': '==', '!==': '!='}
            operator = operators.get(operator, operator)
            if operator not in ('+', '-', '*', '==', '!=', '<', '>', '<=', '>=', 'and', 'or', '&', '|', '^', '<<', '>>'):
                raise UnsupportedAction(f'operator {operator!r} needs target type semantics')
            if self.language in ('JavaScript', 'TypeScript', 'PHP') and operator in ('==', '!='):
                raise UnsupportedAction('equality requires checking source-language coercions')
            return f'({self.expr(left)} {operator} {self.expr(right)})'
        if kind in ('unary_expression', 'unary_operator', 'prefix_unary_expression'):
            child = children[-1]
            op = self.source[node.start_byte:child.start_byte].decode().strip()
            if op not in ('!', '-', '+', '~'):
                raise UnsupportedAction(f'unsupported unary operator {op}')
            return f'({"not " if op == "!" else op}{self.expr(child)})'
        # Calls and member access need explicit runtime/library mappings.
        raise UnsupportedAction(f'unsupported {self.language} expression {kind}: {value[:80]}')

    def statement(self, node):
        kind, children = node.type, self.children(node)
        if kind in ('block', 'compound_statement', 'statement_block', 'function_body', 'statements'):
            return '\n'.join(filter(None, (self.statement(c) for c in children)))
        if kind in ('expression_statement', 'local_declaration_statement', 'local_variable_declaration', 'lexical_declaration', 'variable_declaration', 'declaration', 'field_declaration'):
            relevant = [c for c in children if c.type not in ('integral_type', 'primitive_type', 'predefined_type', 'type_identifier', 'inferred_type', 'modifiers')]
            if self.language == 'Dart' and len(relevant) == 2 and relevant[1].type == 'selector':
                selector = relevant[1]
                while selector.type != 'arguments' and len(selector.named_children) == 1:
                    selector = selector.named_children[0]
                if selector.type == 'arguments':
                    return self.call(self.text(relevant[0]), self.children(selector))
            return '\n'.join(self.statement(c) for c in relevant)
        if kind in ('assignment_expression', 'assignment_statement', 'short_var_declaration', 'assignment', 'compound_assignment_expr', 'augmented_assignment_expression'):
            left = self.field(node, 'left', 'target')
            right = self.field(node, 'right', 'result')
            operator = self.source[left.end_byte:right.start_byte].decode().strip() if left and right else ''
            if operator not in ('=', ':=', '+=', '-=', '*='):
                raise UnsupportedAction(f'assignment {operator!r} requires manual translation')
            target = self.expr(left)
            if not target.isidentifier():
                raise UnsupportedAction('only local variable assignment is supported')
            return f'{target} {"=" if operator == ":=" else operator} {self.expr(right)}'
        if kind in ('variable_declarator', 'init_declarator', 'let_declaration', 'property_declaration', 'initialized_variable_definition'):
            name = self.field(node, 'name', 'declarator', 'pattern')
            value = self.field(node, 'value')
            if value is None and kind == 'variable_declarator' and len(children) == 2:
                value = children[-1]
            if value is None:
                raise UnsupportedAction('uninitialized variable needs a type-specific default')
            return f'{self.expr(name)} = {self.expr(value)}'
        if kind == 'if_statement':
            condition = self.field(node, 'condition')
            yes = self.field(node, 'consequence', 'body')
            no = self.field(node, 'alternative')
            if self.language == 'Swift':
                bodies = [c for c in children if c.type == 'statements']
                yes = bodies[0] if bodies else None
                no = bodies[1] if len(bodies) > 1 else None
            if self.language == 'Dart' and condition is None:
                condition = children[0]
            if condition is None or yes is None:
                raise UnsupportedAction('unsupported conditional shape')
            result = f'if {self.expr(condition)}:\n' + textwrap.indent(self.statement(yes) or 'pass', '    ')
            if no is not None:
                result += '\nelse:\n' + textwrap.indent(self.statement(no) or 'pass', '    ')
            return result
        if kind == 'else_clause':
            return self.statement(self.field(node, 'body') or children[-1])
        if kind in ('while_statement', 'while_expression', 'for_statement'):
            condition = self.field(node, 'condition')
            body = self.field(node, 'body')
            if self.language == 'Swift':
                body = next((c for c in children if c.type == 'statements'), None)
            if self.language == 'Go' and kind == 'for_statement':
                if len(children) != 2 or children[0].type in ('for_clause', 'range_clause'):
                    raise UnsupportedAction('Go range/three-part loop requires iterator lowering')
                condition = children[0]
            if condition is None or body is None:
                raise UnsupportedAction('unsupported loop shape')
            return f'while {self.expr(condition)}:\n' + textwrap.indent(self.statement(body) or 'pass', '    ')
        if kind in ('break_statement', 'continue_statement') and not children:
            return 'break' if kind == 'break_statement' else 'continue'
        if kind in ('call_expression', 'invocation_expression', 'method_invocation', 'function_call_expression', 'macro_invocation'):
            return self.expr(node)
        if kind == 'echo_statement' and self.language == 'PHP':
            return '\n'.join('print(' + self.expr(c) + ', end="")' for c in children)
        if kind in ('return_statement', 'return_expression') or (kind == 'control_transfer_statement' and value_starts_return(self.text(node))):
            if not self.allow_return:
                raise UnsupportedAction('return needs the enclosing ISPA rule return contract')
            if len(children) > 1:
                raise UnsupportedAction('multiple return values require tuple lowering')
            return 'return' + (' ' + self.expr(children[0]) if children else '')
        if kind in ('empty_statement', 'comment', 'line_comment', 'block_comment'):
            return ''
        raise UnsupportedAction(f'unsupported {self.language} statement {kind}: {self.text(node)[:80]}')


def value_starts_return(text):
    return text == 'return' or text.startswith(('return ', 'return\n', 'return\t'))


def translate(code, language, bindings=None, predicate=False, symbols=None):
    language = canonical_language(language)
    try:
        if language not in TARGETS:
            raise UnsupportedAction(f'unknown target {language}')
        transformed, replacements = attributes(textwrap.dedent(code).strip(), bindings or {}, language == 'PHP')
        try:
            from tree_sitter_language_pack import get_parser
        except ImportError:
            raise UnsupportedAction('install the antlr-to-ispa optional dependencies to parse actions')
        if language == 'Python3':
            parsed = get_parser('python').parse(transformed.encode())
            if parsed.root_node.has_error:
                raise UnsupportedAction('Tree-sitter reported invalid Python action syntax')
            tree = ast.parse(transformed, mode='eval' if predicate else 'exec')
            class References(ast.NodeTransformer):
                def visit_Call(self, node):
                    name = ast.unparse(node.func)
                    node = self.generic_visit(node)
                    if name in (symbols or {}):
                        node.func = ast.parse(symbols[name], mode='eval').body
                    return node

                def visit_Name(self, node):
                    if node.id in replacements:
                        replacement = ast.parse(replacements[node.id], mode='eval').body
                        if isinstance(replacement, ast.Name):
                            replacement.ctx = node.ctx
                        return ast.copy_location(replacement, node)
                    return node
            tree = ast.fix_missing_locations(References().visit(tree))
            return ActionResult(code, ast.unparse(tree))
        parser_name, prefix, suffix = TARGETS[language]
        # Use an assignment to get an expression node for a predicate.
        fragment = 'ispa_predicate = ' + transformed + ';' if predicate else transformed
        source = (prefix + fragment + suffix).encode()
        root = get_parser(parser_name).parse(source).root_node
        if root.has_error:
            raise UnsupportedAction('Tree-sitter reported invalid or unsupported action syntax')
        begin, end = len(prefix.encode()), len((prefix + fragment).encode())
        def contained(node):
            for child in node.named_children:
                if child.start_byte >= begin and child.end_byte <= end:
                    yield child
                elif child.start_byte < end and child.end_byte > begin:
                    yield from contained(child)
        lowerer = Lowerer(source, language, replacements, symbols)
        python = '\n'.join(lowerer.statement(node) for node in contained(root))
        tree = ast.parse(python)
        if predicate:
            if len(tree.body) != 1 or not isinstance(tree.body[0], ast.Assign):
                raise UnsupportedAction('predicate did not lower to a single expression')
            python = ast.unparse(tree.body[0].value)
        return ActionResult(code, python)
    except (UnsupportedAction, SyntaxError, ValueError, LookupError) as error:
        return ActionResult(code, reason=str(error))
