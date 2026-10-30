package dev.ispa.grammar

import com.intellij.lexer.Lexer
import com.intellij.lexer.LexerBase
import com.intellij.openapi.editor.DefaultLanguageHighlighterColors as C
import com.intellij.openapi.editor.colors.TextAttributesKey
import com.intellij.ui.JBColor
import com.intellij.openapi.editor.markup.TextAttributes
import com.intellij.openapi.fileTypes.SyntaxHighlighterBase
import com.intellij.openapi.fileTypes.SyntaxHighlighterFactory
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.VirtualFile
import com.intellij.psi.TokenType
import com.intellij.psi.tree.IElementType
import java.awt.Color
import java.awt.Font

object IspaTokens {
    val MODULE = IElementType("MODULE", IspaLanguage.INSTANCE)
    val PARAMETER = IElementType("PARAMETER", IspaLanguage.INSTANCE)
    val TOKEN_DECLARATION = IElementType("TOKEN_DECLARATION", IspaLanguage.INSTANCE)
    val TOKEN_REFERENCE = IElementType("TOKEN_REFERENCE", IspaLanguage.INSTANCE)
    val ID = IElementType("ID", IspaLanguage.INSTANCE)
    val RULE_DECLARATION = IElementType("RULE_DECLARATION", IspaLanguage.INSTANCE)
    val STRING = IElementType("STRING", IspaLanguage.INSTANCE)
    val NUMBER = IElementType("NUMBER", IspaLanguage.INSTANCE)
    val COMMENT = IElementType("COMMENT", IspaLanguage.INSTANCE)
    val QUANTIFIER = IElementType("QUANTIFIER", IspaLanguage.INSTANCE)
    val CAPTURE = IElementType("CAPTURE", IspaLanguage.INSTANCE)
    val CHAR_CLASS = IElementType("CHAR_CLASS", IspaLanguage.INSTANCE)
    val OPERATOR = IElementType("OPERATOR", IspaLanguage.INSTANCE)
    val KEYWORD = IElementType("KEYWORD", IspaLanguage.INSTANCE)
    val PY_IDENTIFIER = IElementType("PY_IDENTIFIER", IspaLanguage.INSTANCE)
    val PY_CALL = IElementType("PY_CALL", IspaLanguage.INSTANCE)
    val PY_ATTRIBUTE = IElementType("PY_ATTRIBUTE", IspaLanguage.INSTANCE)
    val PY_BUILTIN = IElementType("PY_BUILTIN", IspaLanguage.INSTANCE)
    val PY_KEYWORD = IElementType("PY_KEYWORD", IspaLanguage.INSTANCE)
    val PY_STRING = IElementType("PY_STRING", IspaLanguage.INSTANCE)
    val PY_NUMBER = IElementType("PY_NUMBER", IspaLanguage.INSTANCE)
    val PY_COMMENT = IElementType("PY_COMMENT", IspaLanguage.INSTANCE)
    val PY_OPERATOR = IElementType("PY_OPERATOR", IspaLanguage.INSTANCE)
    val PY_PUNCTUATION = IElementType("PY_PUNCTUATION", IspaLanguage.INSTANCE)
    val PY_BRACE = IElementType("PY_BRACE", IspaLanguage.INSTANCE)
}

class IspaLexer : LexerBase() {
    private val expressionFlag = 1 shl 16
    private var buffer: CharSequence = ""
    private var limit = 0
    private var begin = 0
    private var end = 0
    private var state = 0
    private var nextState = 0
    private var token: IElementType? = null
    private val keywords = setOf("module", "import", "from", "as", "detach", "feature", "fail", "match", "inline", "skip", "extern", "global", "auto")
    private val pythonKeywords = setOf("False", "None", "True", "and", "as", "assert", "async", "await", "break", "class", "continue", "def", "del", "elif", "else", "except", "finally", "for", "from", "global", "if", "import", "in", "is", "lambda", "match", "nonlocal", "not", "or", "pass", "raise", "return", "try", "while", "with", "yield")
    private val pythonBuiltins = setOf("abs", "all", "any", "bool", "dict", "enumerate", "filter", "float", "int", "isinstance", "len", "list", "map", "max", "min", "object", "open", "print", "range", "reversed", "set", "sorted", "str", "sum", "super", "tuple", "type", "zip")

    private fun startsPythonBlock(offset: Int): Boolean {
        val lineStart = (begin - 1 downTo 0).firstOrNull { buffer[it] == '\n' }?.plus(1) ?: 0
        val statementStart = (offset - 1 downTo lineStart).firstOrNull { buffer[it] == ';' }?.plus(1) ?: lineStart
        val prefix = buffer.subSequence(statementStart, offset).toString()
        if (Regex("^\\s*(?:import|feature)\\b").containsMatchIn(prefix)) return false
        val previous = prefix.lastOrNull { !it.isWhitespace() }
        if (previous in listOf('@', '%', '&')) return false
        return true
    }

    override fun start(buffer: CharSequence, startOffset: Int, endOffset: Int, initialState: Int) {
        this.buffer = buffer; limit = endOffset; begin = startOffset; state = initialState; locate()
    }
    private fun at(i: Int): Char = if (i < limit) buffer[i] else '\u0000'
    private fun locate() {
        nextState = state; end = begin + 1
        if (begin >= limit) { end = limit; token = null; return }
        val c = at(begin)
        val expression = state and expressionFlag != 0
        val pythonDepth = state ushr 8 and 0xff
        val python = expression || pythonDepth > 0
        token = when {
            c.isWhitespace() -> { while (at(end).isWhitespace() && end < limit) end++; TokenType.WHITE_SPACE }
            !python && c == '/' && at(end) == '/' || python && c == '#' -> {
                while (end < limit && at(end) != '\n') end++; if (python) IspaTokens.PY_COMMENT else IspaTokens.COMMENT
            }
            !python && c == '/' && at(end) == '*' -> {
                end++
                while (end < limit && !(at(end) == '*' && at(end + 1) == '/')) end++
                end = minOf(limit, end + 2); IspaTokens.COMMENT
            }
            c == '\'' || c == '"' -> {
                val triple = python && at(end) == c && at(end + 1) == c
                if (triple) end += 2
                while (end < limit) {
                    if (at(end) == '\\') { end = minOf(limit, end + 2); continue }
                    if (at(end) == c && (!triple || at(end + 1) == c && at(end + 2) == c)) {
                        end += if (triple) 3 else 1; break
                    }
                    end++
                }
                if (python) IspaTokens.PY_STRING else IspaTokens.STRING
            }
            !python && c == '$' && at(end) == '{' -> {
                end++; nextState = state + 256; IspaTokens.OPERATOR
            }
            !python && c == '{' && startsPythonBlock(begin) -> {
                nextState = state + 256; IspaTokens.PY_BRACE
            }
            python && c == '{' -> { nextState = state + 256; IspaTokens.PY_BRACE }
            python && c == '}' && pythonDepth > 0 -> { nextState = state - 256; IspaTokens.PY_BRACE }
            expression && c == '}' -> { nextState = state and expressionFlag.inv(); IspaTokens.OPERATOR }
            !python && c == '=' && at(end) != '>' && (begin == 0 || at(begin - 1) !in "&%!=<>") -> {
                nextState = state or expressionFlag; IspaTokens.PY_OPERATOR
            }
            c.isDigit() -> {
                while (end < limit && (at(end).isLetterOrDigit() || at(end) in "._")) end++
                if (python) IspaTokens.PY_NUMBER else IspaTokens.NUMBER
            }
            c.isLetter() || c == '_' -> {
                while (end < limit && (at(end).isLetterOrDigit() || at(end) == '_')) end++
                val word = buffer.subSequence(begin, end).toString()
                var look = end
                while (look < limit && at(look).isWhitespace()) look++
                when {
                    python -> when {
                        word in pythonKeywords -> IspaTokens.PY_KEYWORD
                        word in pythonBuiltins -> IspaTokens.PY_BUILTIN
                        at(look) == '(' -> IspaTokens.PY_CALL
                        begin > 0 && at(begin - 1) == '.' -> IspaTokens.PY_ATTRIBUTE
                        else -> IspaTokens.PY_IDENTIFIER
                    }
                    word in keywords -> {
                        nextState = when (word) { "module", "import", "from", "as" -> 1; "feature" -> 2; "detach" -> 1; else -> state }
                        IspaTokens.KEYWORD
                    }
                    state == 1 || state == 2 -> { if (state == 2) nextState = 0; IspaTokens.MODULE }
                    at(look) == ':' -> if (word.all { !it.isLetter() || it.isUpperCase() }) IspaTokens.TOKEN_DECLARATION else IspaTokens.RULE_DECLARATION
                    begin > 0 && at(begin - 1) in "@%&" -> IspaTokens.CAPTURE
                    word.all { !it.isLetter() || it.isUpperCase() } -> IspaTokens.TOKEN_REFERENCE
                    else -> IspaTokens.ID
                }
            }
            !python && c == '[' && !(begin > 0 && at(begin - 1) == '#') -> {
                while (end < limit) {
                    if (at(end) == '\\') end = minOf(limit, end + 2)
                    else if (at(end++) == ']') break
                }
                IspaTokens.CHAR_CLASS
            }
            c == ';' && (!python || expression && pythonDepth == 0) -> { nextState = 0; IspaTokens.OPERATOR }
            python && c in "()[]" -> IspaTokens.PY_BRACE
            python && c in ",:.;" -> IspaTokens.PY_PUNCTUATION
            python -> IspaTokens.PY_OPERATOR
            c in "@%&#" && !python -> IspaTokens.CAPTURE
            c in "*+?" && !python -> IspaTokens.QUANTIFIER
            else -> IspaTokens.OPERATOR
        }
    }
    override fun getState() = state
    override fun getTokenType() = token
    override fun getTokenStart() = begin
    override fun getTokenEnd() = end
    override fun advance() { begin = end; state = nextState; locate() }
    override fun getBufferSequence() = buffer
    override fun getBufferEnd() = limit
}

object IspaSyntaxHighlighter : SyntaxHighlighterBase() {
    private fun pythonColor(name: String, light: Int, dark: Int): TextAttributesKey =
        TextAttributesKey.createTextAttributesKey(name, TextAttributes(JBColor(Color(light), Color(dark)), null, null, null, Font.PLAIN))
    val RULE_DECLARATION_KEY = TextAttributesKey.createTextAttributesKey(
        "ISPA_RULE_DECLARATION",
        C.INTERFACE_NAME
    )
    val IDENTIFIER_KEY = TextAttributesKey.createTextAttributesKey(
        "ISPA_IDENTIFIER",
        C.PARAMETER
    )

    val KEYWORD_KEY = TextAttributesKey.createTextAttributesKey("ISPA_KEYWORD", C.KEYWORD)
    val CAPTURE_KEY = TextAttributesKey.createTextAttributesKey("ISPA_CAPTURE", C.METADATA)

    private val module = TextAttributesKey.createTextAttributesKey("ISPA_MODULE", C.CLASS_NAME)
    private val parameter = TextAttributesKey.createTextAttributesKey("ISPA_PARAMETER", C.PARAMETER)
    private val tokenDeclaration = TextAttributesKey.createTextAttributesKey("ISPA_TOKEN_DECLARATION", C.CONSTANT)
    private val tokenReference = TextAttributesKey.createTextAttributesKey("ISPA_TOKEN_REFERENCE", C.CONSTANT)
    private val string = TextAttributesKey.createTextAttributesKey("ISPA_STRING", C.STRING)
    private val comment = TextAttributesKey.createTextAttributesKey("ISPA_COMMENT", C.LINE_COMMENT)
    private val number = TextAttributesKey.createTextAttributesKey("ISPA_NUMBER", C.NUMBER)
    private val operator = TextAttributesKey.createTextAttributesKey("ISPA_OPERATOR", C.OPERATION_SIGN)
    private val quantifier = TextAttributesKey.createTextAttributesKey("ISPA_QUANTIFIER", C.KEYWORD)
    private val characterClass = TextAttributesKey.createTextAttributesKey("ISPA_CHARACTER_CLASS", C.STRING)
    private val pythonIdentifier = pythonColor("ISPA_PY_IDENTIFIER", 0x0451A5, 0x9CDCFE)
    private val pythonCall = pythonColor("ISPA_PY_CALL", 0x795E26, 0xDCDCAA)
    private val pythonAttribute = pythonColor("ISPA_PY_ATTRIBUTE", 0x267F99, 0x4EC9B0)
    private val pythonBuiltin = pythonColor("ISPA_PY_BUILTIN", 0x795E26, 0xDCDCAA)
    private val pythonKeyword = pythonColor("ISPA_PY_KEYWORD", 0xAF00DB, 0xC586C0)
    private val pythonString = pythonColor("ISPA_PY_STRING", 0xA31515, 0xCE9178)
    private val pythonNumber = pythonColor("ISPA_PY_NUMBER", 0x098658, 0xB5CEA8)
    private val pythonComment = pythonColor("ISPA_PY_COMMENT", 0x6A737D, 0x7F9A7F)
    private val pythonOperator = pythonColor("ISPA_PY_OPERATOR", 0x555555, 0xD4D4D4)
    private val pythonPunctuation = pythonColor("ISPA_PY_PUNCTUATION", 0x666666, 0xD4D4D4)
    private val pythonBrace = pythonColor("ISPA_PY_BRACE", 0x7A3E9D, 0xC586C0)

    override fun getHighlightingLexer(): Lexer = IspaLexer()

    override fun getTokenHighlights(tokenType: IElementType): Array<TextAttributesKey> = when (tokenType) {
        IspaTokens.RULE_DECLARATION -> pack(RULE_DECLARATION_KEY)
        IspaTokens.MODULE -> pack(module)
        IspaTokens.PARAMETER -> pack(parameter)
        IspaTokens.TOKEN_DECLARATION -> pack(tokenDeclaration)
        IspaTokens.TOKEN_REFERENCE -> pack(tokenReference)
        IspaTokens.ID -> pack(IDENTIFIER_KEY)
        IspaTokens.STRING -> pack(string)
        IspaTokens.COMMENT -> pack(comment)
        IspaTokens.NUMBER -> pack(number)
        IspaTokens.CAPTURE -> pack(CAPTURE_KEY)
        IspaTokens.CHAR_CLASS -> pack(characterClass)
        IspaTokens.QUANTIFIER -> pack(quantifier)
        IspaTokens.OPERATOR -> pack(operator)
        IspaTokens.KEYWORD -> pack(KEYWORD_KEY)
        IspaTokens.PY_IDENTIFIER -> pack(pythonIdentifier)
        IspaTokens.PY_CALL -> pack(pythonCall)
        IspaTokens.PY_ATTRIBUTE -> pack(pythonAttribute)
        IspaTokens.PY_BUILTIN -> pack(pythonBuiltin)
        IspaTokens.PY_KEYWORD -> pack(pythonKeyword)
        IspaTokens.PY_STRING -> pack(pythonString)
        IspaTokens.PY_NUMBER -> pack(pythonNumber)
        IspaTokens.PY_COMMENT -> pack(pythonComment)
        IspaTokens.PY_OPERATOR -> pack(pythonOperator)
        IspaTokens.PY_PUNCTUATION -> pack(pythonPunctuation)
        IspaTokens.PY_BRACE -> pack(pythonBrace)
        else -> emptyArray()
    }
}

class IspaSyntaxHighlighterFactory : SyntaxHighlighterFactory() {
    override fun getSyntaxHighlighter(project: Project?, virtualFile: VirtualFile?) = IspaSyntaxHighlighter
}
