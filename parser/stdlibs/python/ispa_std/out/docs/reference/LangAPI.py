"""Generated Python-facing representation of the LangAPI IR.

Constructors are compiler intrinsics: the ISPA Python lowerer recognizes

__ispa_internal__.construct calls and creates native LangAPI nodes.

Standalone Python uses a tagged-node fallback for inspection/testing.

"""
import __ispa_internal__

class ExpressionElement:
    GroupOpen = __ispa_internal__.enum("LangAPI.ExpressionElement", "GroupOpen")
    GroupClose = __ispa_internal__.enum("LangAPI.ExpressionElement", "GroupClose")
    SquareBraceOpen = __ispa_internal__.enum("LangAPI.ExpressionElement", "SquareBraceOpen")
    SquareBraceClose = __ispa_internal__.enum("LangAPI.ExpressionElement", "SquareBraceClose")
    And = __ispa_internal__.enum("LangAPI.ExpressionElement", "And")
    Or = __ispa_internal__.enum("LangAPI.ExpressionElement", "Or")
    Not = __ispa_internal__.enum("LangAPI.ExpressionElement", "Not")
    Equal = __ispa_internal__.enum("LangAPI.ExpressionElement", "Equal")
    NotEqual = __ispa_internal__.enum("LangAPI.ExpressionElement", "NotEqual")
    Higher = __ispa_internal__.enum("LangAPI.ExpressionElement", "Higher")
    Lower = __ispa_internal__.enum("LangAPI.ExpressionElement", "Lower")
    HigherOrEqual = __ispa_internal__.enum("LangAPI.ExpressionElement", "HigherOrEqual")
    LowerOrEqual = __ispa_internal__.enum("LangAPI.ExpressionElement", "LowerOrEqual")
    Add = __ispa_internal__.enum("LangAPI.ExpressionElement", "Add")
    Minus = __ispa_internal__.enum("LangAPI.ExpressionElement", "Minus")
    Multiply = __ispa_internal__.enum("LangAPI.ExpressionElement", "Multiply")
    Divide = __ispa_internal__.enum("LangAPI.ExpressionElement", "Divide")
    Modulo = __ispa_internal__.enum("LangAPI.ExpressionElement", "Modulo")
    PlusPlus = __ispa_internal__.enum("LangAPI.ExpressionElement", "PlusPlus")
    MinusMinus = __ispa_internal__.enum("LangAPI.ExpressionElement", "MinusMinus")

class OperatorType:
    Assign = __ispa_internal__.enum("LangAPI.OperatorType", "Assign")
    Add = __ispa_internal__.enum("LangAPI.OperatorType", "Add")
    Minus = __ispa_internal__.enum("LangAPI.OperatorType", "Minus")
    Multiply = __ispa_internal__.enum("LangAPI.OperatorType", "Multiply")
    Divide = __ispa_internal__.enum("LangAPI.OperatorType", "Divide")
    Modulo = __ispa_internal__.enum("LangAPI.OperatorType", "Modulo")

class ValueType:
    Undef = __ispa_internal__.enum("LangAPI.ValueType", "Undef")
    Void = __ispa_internal__.enum("LangAPI.ValueType", "Void")
    Char = __ispa_internal__.enum("LangAPI.ValueType", "Char")
    Int = __ispa_internal__.enum("LangAPI.ValueType", "Int")
    LargeUInt = __ispa_internal__.enum("LangAPI.ValueType", "LargeUInt")
    Bool = __ispa_internal__.enum("LangAPI.ValueType", "Bool")
    Float = __ispa_internal__.enum("LangAPI.ValueType", "Float")
    String = __ispa_internal__.enum("LangAPI.ValueType", "String")
    NonOwnedString = __ispa_internal__.enum("LangAPI.ValueType", "NonOwnedString")
    Array = __ispa_internal__.enum("LangAPI.ValueType", "Array")
    FixedSizeArray = __ispa_internal__.enum("LangAPI.ValueType", "FixedSizeArray")
    Map = __ispa_internal__.enum("LangAPI.ValueType", "Map")
    Symbol = __ispa_internal__.enum("LangAPI.ValueType", "Symbol")
    StorageSymbol = __ispa_internal__.enum("LangAPI.ValueType", "StorageSymbol")
    Token = __ispa_internal__.enum("LangAPI.ValueType", "Token")
    RawToken = __ispa_internal__.enum("LangAPI.ValueType", "RawToken")
    Rule = __ispa_internal__.enum("LangAPI.ValueType", "Rule")
    TokenResult = __ispa_internal__.enum("LangAPI.ValueType", "TokenResult")
    RuleResult = __ispa_internal__.enum("LangAPI.ValueType", "RuleResult")
    Span = __ispa_internal__.enum("LangAPI.ValueType", "Span")
    Variant = __ispa_internal__.enum("LangAPI.ValueType", "Variant")
    LargeVariant = __ispa_internal__.enum("LangAPI.ValueType", "LargeVariant")
    Box = __ispa_internal__.enum("LangAPI.ValueType", "Box")
    Any = __ispa_internal__.enum("LangAPI.ValueType", "Any")
    ImmutableSemantic = __ispa_internal__.enum("LangAPI.ValueType", "ImmutableSemantic")
    BorrowSemantic = __ispa_internal__.enum("LangAPI.ValueType", "BorrowSemantic")
    MoveSemantic = __ispa_internal__.enum("LangAPI.ValueType", "MoveSemantic")
    Tuple = __ispa_internal__.enum("LangAPI.ValueType", "Tuple")
    TypeTag = __ispa_internal__.enum("LangAPI.ValueType", "TypeTag")

class RValueType:
    Undef = __ispa_internal__.enum("LangAPI.RValueType", "Undef")
    Char = __ispa_internal__.enum("LangAPI.RValueType", "Char")
    Int = __ispa_internal__.enum("LangAPI.RValueType", "Int")
    Bool = __ispa_internal__.enum("LangAPI.RValueType", "Bool")
    Float = __ispa_internal__.enum("LangAPI.RValueType", "Float")
    String = __ispa_internal__.enum("LangAPI.RValueType", "String")
    Array = __ispa_internal__.enum("LangAPI.RValueType", "Array")
    FixedSizeArray = __ispa_internal__.enum("LangAPI.RValueType", "FixedSizeArray")
    Map = __ispa_internal__.enum("LangAPI.RValueType", "Map")
    Pos = __ispa_internal__.enum("LangAPI.RValueType", "Pos")
    Symbol = __ispa_internal__.enum("LangAPI.RValueType", "Symbol")
    IspaLibSymbol = __ispa_internal__.enum("LangAPI.RValueType", "IspaLibSymbol")
    StorageSymbol = __ispa_internal__.enum("LangAPI.RValueType", "StorageSymbol")
    Inheritance = __ispa_internal__.enum("LangAPI.RValueType", "Inheritance")
    IspaLibDfaTransition = __ispa_internal__.enum("LangAPI.RValueType", "IspaLibDfaTransition")
    IspaLibDfaSpanCharState = __ispa_internal__.enum("LangAPI.RValueType", "IspaLibDfaSpanCharState")
    IspaLibDfaSpanMultiTableState = __ispa_internal__.enum("LangAPI.RValueType", "IspaLibDfaSpanMultiTableState")
    IspaLibDfaEmptyState = __ispa_internal__.enum("LangAPI.RValueType", "IspaLibDfaEmptyState")
    IspaLibDfaSpan = __ispa_internal__.enum("LangAPI.RValueType", "IspaLibDfaSpan")
    Reference = __ispa_internal__.enum("LangAPI.RValueType", "Reference")
    Span = __ispa_internal__.enum("LangAPI.RValueType", "Span")
    MakeTuple = __ispa_internal__.enum("LangAPI.RValueType", "MakeTuple")
    GetVariant = __ispa_internal__.enum("LangAPI.RValueType", "GetVariant")
    CheckVariant = __ispa_internal__.enum("LangAPI.RValueType", "CheckVariant")
    ToString = __ispa_internal__.enum("LangAPI.RValueType", "ToString")
    CharToStringConstructor = __ispa_internal__.enum("LangAPI.RValueType", "CharToStringConstructor")
    Cast = __ispa_internal__.enum("LangAPI.RValueType", "Cast")
    GetFromBox = __ispa_internal__.enum("LangAPI.RValueType", "GetFromBox")

class ExpressionValueType:
    Empty = __ispa_internal__.enum("LangAPI.ExpressionValueType", "Empty")
    EmptyInitializer = __ispa_internal__.enum("LangAPI.ExpressionValueType", "EmptyInitializer")
    RValue = __ispa_internal__.enum("LangAPI.ExpressionValueType", "RValue")
    ExpressionElement = __ispa_internal__.enum("LangAPI.ExpressionValueType", "ExpressionElement")
    FunctionCall = __ispa_internal__.enum("LangAPI.ExpressionValueType", "FunctionCall")
    IspaLibFunctionCall = __ispa_internal__.enum("LangAPI.ExpressionValueType", "IspaLibFunctionCall")
    StringCompare = __ispa_internal__.enum("LangAPI.ExpressionValueType", "StringCompare")
    Return = __ispa_internal__.enum("LangAPI.ExpressionValueType", "Return")
    Break = __ispa_internal__.enum("LangAPI.ExpressionValueType", "Break")
    Continue = __ispa_internal__.enum("LangAPI.ExpressionValueType", "Continue")
    VariableAssignment = __ispa_internal__.enum("LangAPI.ExpressionValueType", "VariableAssignment")
    CounterIncreament = __ispa_internal__.enum("LangAPI.ExpressionValueType", "CounterIncreament")
    CounterAssignment = __ispa_internal__.enum("LangAPI.ExpressionValueType", "CounterAssignment")
    ResetPosCounter = __ispa_internal__.enum("LangAPI.ExpressionValueType", "ResetPosCounter")
    PushPosCounter = __ispa_internal__.enum("LangAPI.ExpressionValueType", "PushPosCounter")
    PopPosCounter = __ispa_internal__.enum("LangAPI.ExpressionValueType", "PopPosCounter")
    SkipSpaces = __ispa_internal__.enum("LangAPI.ExpressionValueType", "SkipSpaces")
    DfaLookup = __ispa_internal__.enum("LangAPI.ExpressionValueType", "DfaLookup")
    ReportError = __ispa_internal__.enum("LangAPI.ExpressionValueType", "ReportError")
    Lambda = __ispa_internal__.enum("LangAPI.ExpressionValueType", "Lambda")
    DFADebug = __ispa_internal__.enum("LangAPI.ExpressionValueType", "DFADebug")

class ArrayMethods:
    Push = __ispa_internal__.enum("LangAPI.ArrayMethods", "Push")
    Pop = __ispa_internal__.enum("LangAPI.ArrayMethods", "Pop")
    Size = __ispa_internal__.enum("LangAPI.ArrayMethods", "Size")
    Extend = __ispa_internal__.enum("LangAPI.ArrayMethods", "Extend")
    Clear = __ispa_internal__.enum("LangAPI.ArrayMethods", "Clear")
    PythonAppend = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonAppend")
    PythonExtend = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonExtend")
    PythonInsert = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonInsert")
    PythonRemove = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonRemove")
    PythonPop = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonPop")
    PythonClear = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonClear")
    PythonCopy = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonCopy")
    PythonCount = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonCount")
    PythonIndex = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonIndex")
    PythonReverse = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonReverse")
    PythonSort = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonSort")
    PythonGet = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonGet")
    PythonSetDefault = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonSetDefault")
    PythonUpdate = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonUpdate")
    PythonKeys = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonKeys")
    PythonValues = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonValues")
    PythonItems = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonItems")
    PythonFromKeys = __ispa_internal__.enum("LangAPI.ArrayMethods", "PythonFromKeys")

class Visibility:
    Private = __ispa_internal__.enum("LangAPI.Visibility", "Private")
    Public = __ispa_internal__.enum("LangAPI.Visibility", "Public")
    Protected = __ispa_internal__.enum("LangAPI.Visibility", "Protected")

class ObjectMethods:
    pass

class Language:
    Cpp = __ispa_internal__.enum("LangAPI.Language", "Cpp")
    Rust = __ispa_internal__.enum("LangAPI.Language", "Rust")
    Python = __ispa_internal__.enum("LangAPI.Language", "Python")

class StdlibExports:
    Node = __ispa_internal__.enum("LangAPI.StdlibExports", "Node")
    MatchResult = __ispa_internal__.enum("LangAPI.StdlibExports", "MatchResult")
    Lexer = __ispa_internal__.enum("LangAPI.StdlibExports", "Lexer")
    Parser = __ispa_internal__.enum("LangAPI.StdlibExports", "Parser")
    LRParser = __ispa_internal__.enum("LangAPI.StdlibExports", "LRParser")
    LRGeneralizedParser = __ispa_internal__.enum("LangAPI.StdlibExports", "LRGeneralizedParser")
    LRParse = __ispa_internal__.enum("LangAPI.StdlibExports", "LRParse")
    LRParseGeneralized = __ispa_internal__.enum("LangAPI.StdlibExports", "LRParseGeneralized")
    LRGet = __ispa_internal__.enum("LangAPI.StdlibExports", "LRGet")
    LRNode = __ispa_internal__.enum("LangAPI.StdlibExports", "LRNode")
    LRPack = __ispa_internal__.enum("LangAPI.StdlibExports", "LRPack")
    LRCaptureAssign = __ispa_internal__.enum("LangAPI.StdlibExports", "LRCaptureAssign")
    LRToken = __ispa_internal__.enum("LangAPI.StdlibExports", "LRToken")
    LexerMakeTokenParameter = __ispa_internal__.enum("LangAPI.StdlibExports", "LexerMakeTokenParameter")
    DfaState = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaState")
    DfaTable = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaTable")
    DfaClassTable = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaClassTable")
    DfaAcceptTable = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaAcceptTable")
    DfaLRTable = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaLRTable")
    DfaNullState = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaNullState")
    DfaDynamicFailState = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaDynamicFailState")
    ParserFunctionParameter = __ispa_internal__.enum("LangAPI.StdlibExports", "ParserFunctionParameter")
    Error = __ispa_internal__.enum("LangAPI.StdlibExports", "Error")
    TokenNodeConstruct = __ispa_internal__.enum("LangAPI.StdlibExports", "TokenNodeConstruct")
    RawTokenNodeConstruct = __ispa_internal__.enum("LangAPI.StdlibExports", "RawTokenNodeConstruct")
    ParserNodeConstructor = __ispa_internal__.enum("LangAPI.StdlibExports", "ParserNodeConstructor")
    DFADebug = __ispa_internal__.enum("LangAPI.StdlibExports", "DFADebug")
    TdfaLayout = __ispa_internal__.enum("LangAPI.StdlibExports", "TdfaLayout")
    DfaCaptures = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaCaptures")
    DfaDynamicMatch = __ispa_internal__.enum("LangAPI.StdlibExports", "DfaDynamicMatch")
    ASTPrinter = __ispa_internal__.enum("LangAPI.StdlibExports", "ASTPrinter")
    Concat = __ispa_internal__.enum("LangAPI.StdlibExports", "Concat")
    MatchResultValue = __ispa_internal__.enum("LangAPI.StdlibExports", "MatchResultValue")
    VariantIndex = __ispa_internal__.enum("LangAPI.StdlibExports", "VariantIndex")
    AssignCompatible = __ispa_internal__.enum("LangAPI.StdlibExports", "AssignCompatible")
    LexerTokenIterator = __ispa_internal__.enum("LangAPI.StdlibExports", "LexerTokenIterator")
    LexerLazyIterator = __ispa_internal__.enum("LangAPI.StdlibExports", "LexerLazyIterator")
    ParserRequired = __ispa_internal__.enum("LangAPI.StdlibExports", "ParserRequired")
    ParserOptional = __ispa_internal__.enum("LangAPI.StdlibExports", "ParserOptional")
    ParserRange = __ispa_internal__.enum("LangAPI.StdlibExports", "ParserRange")
    ParserRangeOptional = __ispa_internal__.enum("LangAPI.StdlibExports", "ParserRangeOptional")
    RecoveryDecision = __ispa_internal__.enum("LangAPI.StdlibExports", "RecoveryDecision")
    RecoveryState = __ispa_internal__.enum("LangAPI.StdlibExports", "RecoveryState")
    RecoverEncoded = __ispa_internal__.enum("LangAPI.StdlibExports", "RecoverEncoded")
    ParserCounted = __ispa_internal__.enum("LangAPI.StdlibExports", "ParserCounted")
    RecoveryTokenSet = __ispa_internal__.enum("LangAPI.StdlibExports", "RecoveryTokenSet")
    RecoveryEndOfInput = __ispa_internal__.enum("LangAPI.StdlibExports", "RecoveryEndOfInput")
    RecoveryPolicy = __ispa_internal__.enum("LangAPI.StdlibExports", "RecoveryPolicy")
    CloneSelfBox = __ispa_internal__.enum("LangAPI.StdlibExports", "CloneSelfBox")
    LookaheadKind = __ispa_internal__.enum("LangAPI.StdlibExports", "LookaheadKind")
    TextOutput = __ispa_internal__.enum("LangAPI.StdlibExports", "TextOutput")
    RecoveryHandler = __ispa_internal__.enum("LangAPI.StdlibExports", "RecoveryHandler")
    TokenCursor = __ispa_internal__.enum("LangAPI.StdlibExports", "TokenCursor")
    LazyTokenCursor = __ispa_internal__.enum("LangAPI.StdlibExports", "LazyTokenCursor")
    ResetTokenStream = __ispa_internal__.enum("LangAPI.StdlibExports", "ResetTokenStream")

class ImplementationKind:
    Inherent = __ispa_internal__.enum("LangAPI.ImplementationKind", "Inherent")
    Interface = __ispa_internal__.enum("LangAPI.ImplementationKind", "Interface")

class Declarations:
    """LangAPI::Declarations IR node."""
    __ispa_kind__ = "LangAPI.Declarations"
    def __new__(cls, values=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "values": values,
        })

class Statements:
    """LangAPI::Statements IR node."""
    __ispa_kind__ = "LangAPI.Statements"
    def __new__(cls, values=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "values": values,
        })

class Expression:
    """LangAPI::Expression IR node."""
    __ispa_kind__ = "LangAPI.Expression"
    def __new__(cls, values=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "values": values,
        })

class Char:
    """LangAPI::Char IR node."""
    __ispa_kind__ = "LangAPI.Char"
    def __new__(cls, value=None, escaped=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
            "escaped": escaped,
        })

class Int:
    """LangAPI::Int IR node."""
    __ispa_kind__ = "LangAPI.Int"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class Bool:
    """LangAPI::Bool IR node."""
    __ispa_kind__ = "LangAPI.Bool"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class Float:
    """LangAPI::Float IR node."""
    __ispa_kind__ = "LangAPI.Float"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class String:
    """LangAPI::String IR node."""
    __ispa_kind__ = "LangAPI.String"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class Array:
    """LangAPI::Array IR node."""
    __ispa_kind__ = "LangAPI.Array"
    def __new__(cls, values=None, template_parameters=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "values": values,
            "template_parameters": template_parameters,
        })

class FixedSizeArray:
    """LangAPI::FixedSizeArray IR node."""
    __ispa_kind__ = "LangAPI.FixedSizeArray"
    def __new__(cls, values=None, template_parameters=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "values": values,
            "template_parameters": template_parameters,
        })

class Map:
    """LangAPI::Map IR node."""
    __ispa_kind__ = "LangAPI.Map"
    def __new__(cls, keys=None, values=None, template_parameters=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "keys": keys,
            "values": values,
            "template_parameters": template_parameters,
        })

class Pos:
    """LangAPI::Pos IR node."""
    __ispa_kind__ = "LangAPI.Pos"
    def __new__(cls, dereference=None, offset=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "dereference": dereference,
            "offset": offset,
        })

class ArrayMethodCall:
    """LangAPI::ArrayMethodCall IR node."""
    __ispa_kind__ = "LangAPI.ArrayMethodCall"
    def __new__(cls, method=None, args=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "method": method,
            "args": args,
        })

class FunctionCall:
    """LangAPI::FunctionCall IR node."""
    __ispa_kind__ = "LangAPI.FunctionCall"
    def __new__(cls, name=None, template_parameters=None, args=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "template_parameters": template_parameters,
            "args": args,
        })

class IspaLibSymbol:
    """LangAPI::IspaLibSymbol IR node."""
    __ispa_kind__ = "LangAPI.IspaLibSymbol"
    def __new__(cls, exports=None, template_parameters=None, Const=None, Reference=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "exports": exports,
            "template_parameters": template_parameters,
            "Const": Const,
            "Reference": Reference,
        })

class Symbol:
    """LangAPI::Symbol IR node."""
    __ispa_kind__ = "LangAPI.Symbol"
    def __new__(cls, path=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "path": path,
        })

class StorageOffset:
    """LangAPI::StorageOffset IR node."""
    __ispa_kind__ = "LangAPI.StorageOffset"
    def __new__(cls, offset=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "offset": offset,
        })

class Property:
    """LangAPI::Property IR node."""
    __ispa_kind__ = "LangAPI.Property"
    def __new__(cls, name=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
        })

class StorageSymbol:
    """LangAPI::StorageSymbol IR node."""
    __ispa_kind__ = "LangAPI.StorageSymbol"
    def __new__(cls, what=None, path=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "what": what,
            "path": path,
        })

class IspaLibFunctionCall:
    """LangAPI::IspaLibFunctionCall IR node."""
    __ispa_kind__ = "LangAPI.IspaLibFunctionCall"
    def __new__(cls, symbol=None, args=None, template_parameters=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "symbol": symbol,
            "args": args,
            "template_parameters": template_parameters,
        })

class Inheritance:
    """LangAPI::Inheritance IR node."""
    __ispa_kind__ = "LangAPI.Inheritance"
    def __new__(cls, name=None, args=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "args": args,
        })

class Token:
    """LangAPI::Token IR node."""
    __ispa_kind__ = "LangAPI.Token"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class Rule:
    """LangAPI::Rule IR node."""
    __ispa_kind__ = "LangAPI.Rule"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class TokenResult:
    """LangAPI::TokenResult IR node."""
    __ispa_kind__ = "LangAPI.TokenResult"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class RuleResult:
    """LangAPI::RuleResult IR node."""
    __ispa_kind__ = "LangAPI.RuleResult"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class IspaLibDfaTransition:
    """LangAPI::IspaLibDfaTransition IR node."""
    __ispa_kind__ = "LangAPI.IspaLibDfaTransition"
    def __new__(cls, symbol=None, next=None, new_cst_node=None, new_member=None, close_cst_node=None, new_group=None, group_close=None, accept=None, transition_type=None, is_refferring_char_table=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "symbol": symbol,
            "next": next,
            "new_cst_node": new_cst_node,
            "new_member": new_member,
            "close_cst_node": close_cst_node,
            "new_group": new_group,
            "group_close": group_close,
            "accept": accept,
            "transition_type": transition_type,
            "is_refferring_char_table": is_refferring_char_table,
        })

class IspaLibDfaState:
    """LangAPI::IspaLibDfaState IR node."""
    __ispa_kind__ = "LangAPI.IspaLibDfaState"
    def __new__(cls, transitions=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "transitions": transitions,
        })

class IspaLibDfaSpanCharState:
    """LangAPI::IspaLibDfaSpanCharState IR node."""
    __ispa_kind__ = "LangAPI.IspaLibDfaSpanCharState"
    def __new__(cls, else_goto=None, else_goto_accept=None, state_id=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "else_goto": else_goto,
            "else_goto_accept": else_goto_accept,
            "state_id": state_id,
        })

class IspaLibDfaSpanMultiTableState:
    """LangAPI::IspaLibDfaSpanMultiTableState IR node."""
    __ispa_kind__ = "LangAPI.IspaLibDfaSpanMultiTableState"
    def __new__(cls, else_goto=None, else_goto_accept=None, state_id=None, mutli_table_transitions=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "else_goto": else_goto,
            "else_goto_accept": else_goto_accept,
            "state_id": state_id,
            "mutli_table_transitions": mutli_table_transitions,
        })

class IspaLibDfaEmptyState:
    """LangAPI::IspaLibDfaEmptyState IR node."""
    __ispa_kind__ = "LangAPI.IspaLibDfaEmptyState"
    def __new__(cls, token_name=None, construction_lambda=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "token_name": token_name,
            "construction_lambda": construction_lambda,
        })

class IspaLibDfaSpan:
    """LangAPI::IspaLibDfaSpan IR node."""
    __ispa_kind__ = "LangAPI.IspaLibDfaSpan"
    def __new__(cls, type=None, assing_name=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "type": type,
            "assing_name": assing_name,
        })

class Reference:
    """LangAPI::Reference IR node."""
    __ispa_kind__ = "LangAPI.Reference"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class Span:
    """LangAPI::Span IR node."""
    __ispa_kind__ = "LangAPI.Span"
    def __new__(cls, sym=None, type=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "sym": sym,
            "type": type,
        })

class EmptyInitializer:
    """LangAPI::EmptyInitializer IR node."""
    __ispa_kind__ = "LangAPI.EmptyInitializer"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class MakeTuple:
    """LangAPI::MakeTuple IR node."""
    __ispa_kind__ = "LangAPI.MakeTuple"
    def __new__(cls, args=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "args": args,
        })

class GetVariant:
    """LangAPI::GetVariant IR node."""
    __ispa_kind__ = "LangAPI.GetVariant"
    def __new__(cls, type=None, sym=None, is_raw_token=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "type": type,
            "sym": sym,
            "is_raw_token": is_raw_token,
        })

class CheckVariant:
    """LangAPI::CheckVariant IR node."""
    __ispa_kind__ = "LangAPI.CheckVariant"
    def __new__(cls, type=None, sym=None, is_raw_token=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "type": type,
            "sym": sym,
            "is_raw_token": is_raw_token,
        })

class CharToStringConstructor:
    """LangAPI::CharToStringConstructor IR node."""
    __ispa_kind__ = "LangAPI.CharToStringConstructor"
    def __new__(cls, what=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "what": what,
        })

class ToString:
    """LangAPI::ToString IR node."""
    __ispa_kind__ = "LangAPI.ToString"
    def __new__(cls, what=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "what": what,
        })

class Cast:
    """LangAPI::Cast IR node."""
    __ispa_kind__ = "LangAPI.Cast"
    def __new__(cls, type=None, what=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "type": type,
            "what": what,
        })

class GetFromBox:
    """LangAPI::GetFromBox IR node."""
    __ispa_kind__ = "LangAPI.GetFromBox"
    def __new__(cls, what=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "what": what,
        })

class RValue:
    """LangAPI::RValue IR node."""
    __ispa_kind__ = "LangAPI.RValue"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class Type:
    """LangAPI::Type IR node."""
    __ispa_kind__ = "LangAPI.Type"
    def __new__(cls, type=None, template_parameters=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "type": type,
            "template_parameters": template_parameters,
        })

class ForwardDeclaredClass:
    """LangAPI::ForwardDeclaredClass IR node."""
    __ispa_kind__ = "LangAPI.ForwardDeclaredClass"
    def __new__(cls, name=None, isStruct=None, template_parameters=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "isStruct": isStruct,
            "template_parameters": template_parameters,
        })

class Function:
    """LangAPI::Function IR node."""
    __ispa_kind__ = "LangAPI.Function"
    def __new__(cls, type=None, name=None, parameters=None, statements=None, template_parameters=None, is_static=None, is_const=None, is_wrap_function=None, override=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "type": type,
            "name": name,
            "parameters": parameters,
            "statements": statements,
            "template_parameters": template_parameters,
            "is_static": is_static,
            "is_const": is_const,
            "is_wrap_function": is_wrap_function,
            "override": override,
        })

class GenericParameter:
    """LangAPI::GenericParameter IR node."""
    __ispa_kind__ = "LangAPI.GenericParameter"
    def __new__(cls, name=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
        })

class TypeConstraint:
    """LangAPI::TypeConstraint IR node."""
    __ispa_kind__ = "LangAPI.TypeConstraint"
    def __new__(cls, type=None, interfaces=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "type": type,
            "interfaces": interfaces,
        })

class AssociatedType:
    """LangAPI::AssociatedType IR node."""
    __ispa_kind__ = "LangAPI.AssociatedType"
    def __new__(cls, name=None, default_type=None, bounds=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "default_type": default_type,
            "bounds": bounds,
        })

class AssociatedConstant:
    """LangAPI::AssociatedConstant IR node."""
    __ispa_kind__ = "LangAPI.AssociatedConstant"
    def __new__(cls, name=None, type=None, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "type": type,
            "value": value,
        })

class Interface:
    """LangAPI::Interface IR node."""
    __ispa_kind__ = "LangAPI.Interface"
    def __new__(cls, name=None, template_parameters=None, data=None, super_interfaces=None, associated_types=None, associated_constants=None, constraints=None, dependencies=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "template_parameters": template_parameters,
            "data": data,
            "super_interfaces": super_interfaces,
            "associated_types": associated_types,
            "associated_constants": associated_constants,
            "constraints": constraints,
            "dependencies": dependencies,
        })

class Implementation:
    """LangAPI::Implementation IR node."""
    __ispa_kind__ = "LangAPI.Implementation"
    def __new__(cls, kind=None, target=None, interface=None, template_parameters=None, constraints=None, data=None, associated_types=None, associated_constants=None, dependencies=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "kind": kind,
            "target": target,
            "interface": interface,
            "template_parameters": template_parameters,
            "constraints": constraints,
            "data": data,
            "associated_types": associated_types,
            "associated_constants": associated_constants,
            "dependencies": dependencies,
        })

class Class:
    """LangAPI::Class IR node."""
    __ispa_kind__ = "LangAPI.Class"
    def __new__(cls, name=None, template_parameters=None, data=None, inherit_members=None, default_visibility=None, constructor=None, interfaces=None, to_str_fun=None, output_fun=None, dependecies=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "template_parameters": template_parameters,
            "data": data,
            "inherit_members": inherit_members,
            "default_visibility": default_visibility,
            "constructor": constructor,
            "interfaces": interfaces,
            "to_str_fun": to_str_fun,
            "output_fun": output_fun,
            "dependecies": dependecies,
        })

class Namespace:
    """LangAPI::Namespace IR node."""
    __ispa_kind__ = "LangAPI.Namespace"
    def __new__(cls, name=None, declarations=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "declarations": declarations,
        })

class TypeAlias:
    """LangAPI::TypeAlias IR node."""
    __ispa_kind__ = "LangAPI.TypeAlias"
    def __new__(cls, name=None, type=None, template_parameters=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "type": type,
            "template_parameters": template_parameters,
        })

class Enum:
    """LangAPI::Enum IR node."""
    __ispa_kind__ = "LangAPI.Enum"
    def __new__(cls, name=None, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "value": value,
        })

class Variable:
    """LangAPI::Variable IR node."""
    __ispa_kind__ = "LangAPI.Variable"
    def __new__(cls, name=None, type=None, value=None, set=None, nested_array_type=None, is_static=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "type": type,
            "value": value,
            "set": set,
            "nested_array_type": nested_array_type,
            "is_static": is_static,
        })

class Declaration:
    """LangAPI::Declaration IR node."""
    __ispa_kind__ = "LangAPI.Declaration"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class Break:
    """LangAPI::Break IR node."""
    __ispa_kind__ = "LangAPI.Break"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class Continue:
    """LangAPI::Continue IR node."""
    __ispa_kind__ = "LangAPI.Continue"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class Return:
    """LangAPI::Return IR node."""
    __ispa_kind__ = "LangAPI.Return"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class StringCompare:
    """LangAPI::StringCompare IR node."""
    __ispa_kind__ = "LangAPI.StringCompare"
    def __new__(cls, str=None, is_string=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "str": str,
            "is_string": is_string,
        })

class VariableAssignment:
    """LangAPI::VariableAssignment IR node."""
    __ispa_kind__ = "LangAPI.VariableAssignment"
    def __new__(cls, name=None, type=None, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
            "type": type,
            "value": value,
        })

class CounterIncreament:
    """LangAPI::CounterIncreament IR node."""
    __ispa_kind__ = "LangAPI.CounterIncreament"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class AssignCounter:
    """LangAPI::AssignCounter IR node."""
    __ispa_kind__ = "LangAPI.AssignCounter"
    def __new__(cls, v=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "v": v,
        })

class ResetPosCounter:
    """LangAPI::ResetPosCounter IR node."""
    __ispa_kind__ = "LangAPI.ResetPosCounter"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class PushPosCounter:
    """LangAPI::PushPosCounter IR node."""
    __ispa_kind__ = "LangAPI.PushPosCounter"
    def __new__(cls, name=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "name": name,
        })

class PopPosCounter:
    """LangAPI::PopPosCounter IR node."""
    __ispa_kind__ = "LangAPI.PopPosCounter"
    def __new__(cls):
        return __ispa_internal__.construct(cls.__ispa_kind__, {})

class SkipSpaces:
    """LangAPI::SkipSpaces IR node."""
    __ispa_kind__ = "LangAPI.SkipSpaces"
    def __new__(cls, isToken=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "isToken": isToken,
        })

class DfaLookup:
    """LangAPI::DfaLookup IR node."""
    __ispa_kind__ = "LangAPI.DfaLookup"
    def __new__(cls, dfa_count=None, output_name=None, return_type=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "dfa_count": dfa_count,
            "output_name": output_name,
            "return_type": return_type,
        })

class ReportError:
    """LangAPI::ReportError IR node."""
    __ispa_kind__ = "LangAPI.ReportError"
    def __new__(cls, message=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "message": message,
        })

class Lambda:
    """LangAPI::Lambda IR node."""
    __ispa_kind__ = "LangAPI.Lambda"
    def __new__(cls, parameters=None, statements=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "parameters": parameters,
            "statements": statements,
        })

class DFADebug:
    """LangAPI::DFADebug IR node."""
    __ispa_kind__ = "LangAPI.DFADebug"
    def __new__(cls, member=None, token_name=None, position_in_token=None, call=None, group=None, rule_run=None, offset=None, length=None, ch=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "member": member,
            "token_name": token_name,
            "position_in_token": position_in_token,
            "call": call,
            "group": group,
            "rule_run": rule_run,
            "offset": offset,
            "length": length,
            "ch": ch,
        })

class ExpressionValue:
    """LangAPI::ExpressionValue IR node."""
    __ispa_kind__ = "LangAPI.ExpressionValue"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class ConditionalElement:
    """LangAPI::ConditionalElement IR node."""
    __ispa_kind__ = "LangAPI.ConditionalElement"
    def __new__(cls, expr=None, stmt=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "expr": expr,
            "stmt": stmt,
        })

class If:
    """LangAPI::If IR node."""
    __ispa_kind__ = "LangAPI.If"
    def __new__(cls, expr=None, stmt=None, else_stmt=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "expr": expr,
            "stmt": stmt,
            "else_stmt": else_stmt,
        })

class While:
    """LangAPI::While IR node."""
    __ispa_kind__ = "LangAPI.While"
    def __new__(cls, expr=None, stmt=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "expr": expr,
            "stmt": stmt,
        })

class DoWhile:
    """LangAPI::DoWhile IR node."""
    __ispa_kind__ = "LangAPI.DoWhile"
    def __new__(cls, expr=None, stmt=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "expr": expr,
            "stmt": stmt,
        })

class Switch:
    """LangAPI::Switch IR node."""
    __ispa_kind__ = "LangAPI.Switch"
    def __new__(cls, expression=None, cases=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "expression": expression,
            "cases": cases,
        })

class Throw:
    """LangAPI::Throw IR node."""
    __ispa_kind__ = "LangAPI.Throw"
    def __new__(cls, throw_value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "throw_value": throw_value,
        })

class Comment:
    """LangAPI::Comment IR node."""
    __ispa_kind__ = "LangAPI.Comment"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

class Statement:
    """LangAPI::Statement IR node."""
    __ispa_kind__ = "LangAPI.Statement"
    def __new__(cls, value=None):
        return __ispa_internal__.construct(cls.__ispa_kind__, {
            "value": value,
        })

__all__ = ['ExpressionElement', 'OperatorType', 'ValueType', 'RValueType', 'ExpressionValueType', 'ArrayMethods', 'Visibility', 'ObjectMethods', 'Language', 'StdlibExports', 'ImplementationKind', 'Declarations', 'Statements', 'Expression', 'Char', 'Int', 'Bool', 'Float', 'String', 'Array', 'FixedSizeArray', 'Map', 'Pos', 'ArrayMethodCall', 'FunctionCall', 'IspaLibSymbol', 'Symbol', 'StorageOffset', 'Property', 'StorageSymbol', 'IspaLibFunctionCall', 'Inheritance', 'Token', 'Rule', 'TokenResult', 'RuleResult', 'IspaLibDfaTransition', 'IspaLibDfaState', 'IspaLibDfaSpanCharState', 'IspaLibDfaSpanMultiTableState', 'IspaLibDfaEmptyState', 'IspaLibDfaSpan', 'Reference', 'Span', 'EmptyInitializer', 'MakeTuple', 'GetVariant', 'CheckVariant', 'CharToStringConstructor', 'ToString', 'Cast', 'GetFromBox', 'RValue', 'Type', 'ForwardDeclaredClass', 'Function', 'GenericParameter', 'TypeConstraint', 'AssociatedType', 'AssociatedConstant', 'Interface', 'Implementation', 'Class', 'Namespace', 'TypeAlias', 'Enum', 'Variable', 'Declaration', 'Break', 'Continue', 'Return', 'StringCompare', 'VariableAssignment', 'CounterIncreament', 'AssignCounter', 'ResetPosCounter', 'PushPosCounter', 'PopPosCounter', 'SkipSpaces', 'DfaLookup', 'ReportError', 'Lambda', 'DFADebug', 'ExpressionValue', 'ConditionalElement', 'If', 'While', 'DoWhile', 'Switch', 'Throw', 'Comment', 'Statement']
