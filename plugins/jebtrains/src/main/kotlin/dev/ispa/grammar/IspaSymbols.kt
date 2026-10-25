package dev.ispa.grammar

import com.intellij.psi.tree.IElementType

internal data class IspaSymbol(val name: String, val offset: Int, val token: Boolean)

/** A tolerant, file-local index. It also works while the grammar is incomplete. */
internal object IspaSymbols {
    private val declaration = Regex("(?m)(?:^|;)[ \\t]*(?:#\\[[^]\\n]*]\\s*)*#?[ \\t]*([A-Za-z_][A-Za-z_0-9]*)(?:[ \\t]*<[^>\\n]*>)?[ \\t]*:")
    private val advancedDeclaration = Regex("(?m)(?:^|;|})[ \\t]*(?:#\\[[^]\\n]*]\\s*)*([A-Za-z_][A-Za-z_0-9]*)(?:[ \\t]*<[^>\\n]*>)?[ \\t]*\\{")
    private val failDeclaration = Regex("(?m)^[ \\t]*fail[ \\t]+([A-Za-z_][A-Za-z_0-9]*)[ \\t]*\\{")

    private data class Word(val start: Int, val end: Int, val type: IElementType, val state: Int)

    private fun words(source: String): List<Word> {
        val lexer = IspaLexer()
        lexer.start(source, 0, source.length, 0)
        val result = ArrayList<Word>()
        while (lexer.tokenType != null) {
            result += Word(lexer.tokenStart, lexer.tokenEnd, lexer.tokenType!!, lexer.state)
            lexer.advance()
        }
        return result
    }

    fun declarations(source: String): List<IspaSymbol> {
        val words = words(source)
        val byStart = words.associateBy { it.start }
        return (declaration.findAll(source) + advancedDeclaration.findAll(source) + failDeclaration.findAll(source)).mapNotNull { match ->
            val group = match.groups[1] ?: return@mapNotNull null
            val word = byStart[group.range.first] ?: return@mapNotNull null
            if (word.state ushr 8 != 0 || word.type in setOf(IspaTokens.COMMENT, IspaTokens.STRING, IspaTokens.CHAR_CLASS)) return@mapNotNull null
            IspaSymbol(group.value, word.start, group.value.any(Char::isLetter) && group.value.all { !it.isLetter() || it.isUpperCase() })
        }.distinctBy { it.offset }.sortedBy { it.offset }.toList()
    }

    fun referenceAt(source: String, offset: Int): String? {
        val word = words(source).firstOrNull { offset in it.start until it.end } ?: return null
        if (word.state ushr 8 != 0 || word.type !in setOf(IspaTokens.ID, IspaTokens.TOKEN_REFERENCE, IspaTokens.RULE_DECLARATION, IspaTokens.TOKEN_DECLARATION)) return null
        if (declarations(source).any { it.offset == word.start }) return null
        return source.substring(word.start, word.end)
    }

    fun acceptsCompletion(source: String, offset: Int): Boolean {
        if (offset !in 0..source.length) return false
        val word = words(source).firstOrNull { offset >= it.start && offset < it.end || offset == source.length && it.end == offset }
        if (word != null && (word.state ushr 8 != 0 || word.type in setOf(IspaTokens.COMMENT, IspaTokens.STRING, IspaTokens.CHAR_CLASS, IspaTokens.MODULE, IspaTokens.KEYWORD))) return false
        val start = maxOf(source.lastIndexOf('\n', (offset - 1).coerceAtLeast(0)), source.lastIndexOf(';', (offset - 1).coerceAtLeast(0))) + 1
        val statement = source.substring(start, offset)
        if (Regex("^\\s*(?:module|import|detach|feature)\\b").containsMatchIn(statement)) return false
        return true
    }
}
