"""ISPA Python target runtime (Python 3.10+)."""
from .model import (Node, RawToken, MatchResult, Variant, Position,
                    variant_get, variant_holds, variant_index, assign_compatible, concat,
                    step_attr, step_item, assign_attr, assign_item)
from .lexer import Lexer, Cursor, LexError
from .python_lexer import make_python_lexer
from .parser import LLParser, ParseError, CountSpec, parse_token_stream
from . import recovery as Recovery
from .recovery import EndOfInput
from .recovery_context import RuleContext, FailContext, custom_failure, custom_failure_decision
from .dfa import Captures, DFADebug, DFAResult, TdfaLayout, run_dfa, run_tdfa, dynamic_match, NULL_STATE, DYNAMIC_FAIL_STATE
from .overload import overload
from .conversion import divide, modulo, python_update, substr, template, TypeTag
from .printer import ASTPrinter, format_ast
from .external import ExternalRule, external_match
from .lr import (LRParser, LRGeneralizedParser, LRRegionFrame, LRRuleSpan, LRControlPhase, Diagnostic,
                 lr_token, lr_pack, lr_get, lr_node, lr_capture_assign, lr_convert,
                 ABSENT, CONFLICT_ACTION, INVALID_ACTION)


def lookahead_kind(cursor):
    token = cursor.peek()
    return EndOfInput if token is None else Recovery.token_id(token)

__all__ = [
    'divide', 'modulo', 'python_update', 'substr', 'template', 'TypeTag',
    'Node', 'RawToken', 'MatchResult', 'Variant', 'Position', 'Cursor', 'Lexer', 'LexError',
    'LLParser', 'ParseError', 'CountSpec', 'parse_token_stream', 'make_python_lexer', 'Captures',
    'DFADebug', 'DFAResult', 'TdfaLayout', 'run_dfa', 'run_tdfa', 'dynamic_match', 'NULL_STATE', 'DYNAMIC_FAIL_STATE',
    'variant_get', 'variant_holds', 'variant_index', 'assign_compatible', 'concat',
    'step_attr', 'step_item', 'assign_attr', 'assign_item', 'overload', 'ASTPrinter', 'format_ast',
    'ExternalRule', 'external_match',
    'LRParser', 'LRGeneralizedParser', 'LRRegionFrame', 'LRRuleSpan', 'LRControlPhase', 'Diagnostic',
    'lr_token', 'lr_pack', 'lr_get', 'lr_node', 'lr_capture_assign', 'lr_convert',
    'ABSENT', 'CONFLICT_ACTION', 'INVALID_ACTION',
    'Recovery', 'EndOfInput', 'lookahead_kind', 'RuleContext', 'FailContext', 'custom_failure', 'custom_failure_decision',
]
