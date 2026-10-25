package dev.ispa.grammar

import com.intellij.psi.tree.IElementType
import java.nio.file.Files
import java.nio.file.Path
import java.util.concurrent.ConcurrentHashMap

internal data class EditorSuggestion(val name: String, val kind: String, val detail: String? = null)
data class EditorProblem(val start: Int, val end: Int, val message: String, val warning: Boolean = false)

/** A tolerant editor model. The generated parser remains the authority for syntax. */
internal object IspaEditorModel {
    private data class Lexeme(val start: Int, val end: Int, val type: IElementType, val python: Boolean)
    private val grammarImport = Regex("(?m)^\\s*import\\s+(?:(?:\\*|\\{[^}]*})\\s+from\\s+)?([A-Za-z_]\\w*(?:\\.[A-Za-z_]\\w*)*)(?:\\s+as\\s+([A-Za-z_]\\w*))?\\s*;")
    private val pythonImport = Regex("(?m)\\b(?:from\\s+([A-Za-z_]\\w*(?:\\.[A-Za-z_]\\w*)*)\\s+import\\s+([A-Za-z_]\\w*)|import\\s+([A-Za-z_]\\w*(?:\\.[A-Za-z_]\\w*)*)(?:\\s+as\\s+([A-Za-z_]\\w*))?)")
    private val assignment = Regex("(?m)\\b([A-Za-z_]\\w*)\\s*(?::\\s*(int|float|str|bool|list|dict|tuple)(?:\\[[^\\n]*\\])?)?\\s*=\\s*([^\\n;}]*)")
    private val typedDeclaration = Regex("(?m)\\b([A-Za-z_]\\w*)\\s*:\\s*(list|dict)(?:\\[[^\\n]*\\])?")
    private val function = Regex("(?m)\\b(?:def|class)\\s+([A-Za-z_]\\w*)")
    private val ctxMember = Regex("\\bctx\\.([A-Za-z_]\\w*)")
    private val pythonNumber = Regex("(?:0[xX][0-9a-fA-F_]+|0[bB][01_]+|0[oO][0-7_]+|[0-9][0-9_]*(?:\\.[0-9_]*)?(?:[eE][+-]?[0-9][0-9_]*)?)(?:[jJ])?")
    private val ctxMembers = listOf("pos", "start_pos", "rule", "failed_rule", "text", "line_prefix", "next_char", "ast", "shared", "at", "mark", "restore", "commit", "collect_braced", "error", "note", "sync", "sync_tokens", "skip", "insert", "replace", "retry", "abort", "emit")
    private val builtins = listOf("len", "str", "int", "float", "bool", "list", "dict", "tuple", "set", "enumerate", "print", "isinstance")
    private val listMethods = listOf("append", "clear", "copy", "count", "extend", "index", "insert", "pop", "remove", "reverse", "sort")
    private val dictMethods = listOf("clear", "copy", "fromkeys", "get", "items", "keys", "pop", "popitem", "setdefault", "update", "values")
    private val stdfunFunctions: List<EditorSuggestion> by lazy {
        val source = IspaEditorModel::class.java.getResourceAsStream("/ispa/stdfun.py")?.bufferedReader()?.use { it.readText() } ?: return@lazy emptyList()
        Regex("(?m)^def\\s+([A-Za-z_]\\w*)\\s*\\(([^)]*)\\)\\s*(?:->\\s*([^:]+))?:")
            .findAll(source).map { match ->
                EditorSuggestion(match.groupValues[1], "function", "(${match.groupValues[2]}) -> ${match.groupValues[3].trim().ifEmpty { "Any" }}")
            }.toList()
    }
    private val grammarCache = ConcurrentHashMap<Path, Pair<Long, List<IspaSymbol>>>()
    private val pythonCache = ConcurrentHashMap<Path, Pair<Long, List<EditorSuggestion>>>()
    private val sourceCache = ConcurrentHashMap<Path, Pair<Long, List<String>>>()
    private val standardModules = listOf("collections", "datetime", "decimal", "functools", "itertools", "json", "math", "os", "pathlib", "random", "re", "statistics", "subprocess", "sys", "time", "typing")

    private fun modules(file: Path?, root: Path?, extension: String): List<EditorSuggestion> {
        val found = ArrayList<EditorSuggestion>()
        for (directory in listOfNotNull(file?.parent, root).distinct()) {
            if (!Files.isDirectory(directory)) continue
            runCatching {
                Files.list(directory).use { entries ->
                    entries.limit(1000).forEach { path ->
                        val filename = path.fileName.toString()
                        if (filename.endsWith(extension)) found += EditorSuggestion(filename.removeSuffix(extension), "module")
                        else if (Files.isDirectory(path) && Files.isRegularFile(path.resolve("__init__${extension}")))
                            found += EditorSuggestion(filename, "module")
                    }
                }
            }
        }
        if (extension == ".py") found += standardModules.map { EditorSuggestion(it, "module") }
        return found.distinctBy { it.name }
    }

    private fun <T> cached(path: Path, cache: ConcurrentHashMap<Path, Pair<Long, List<T>>>, scan: (String) -> List<T>): List<T> {
        val stamp = runCatching {
            if (!Files.isRegularFile(path) || Files.size(path) > 1_000_000) return emptyList()
            Files.getLastModifiedTime(path).toMillis()
        }.getOrNull() ?: return emptyList()
        cache[path]?.let { if (it.first == stamp) return it.second }
        val value = runCatching { scan(Files.readString(path)) }.getOrDefault(emptyList())
        if (cache.size > 128) cache.clear()
        cache[path] = stamp to value
        return value
    }

    private fun lex(source: String): List<Lexeme> {
        val lexer = IspaLexer()
        lexer.start(source, 0, source.length, 0)
        val result = ArrayList<Lexeme>()
        while (lexer.tokenType != null) {
            result += Lexeme(lexer.tokenStart, lexer.tokenEnd, lexer.tokenType!!, lexer.state ushr 8 != 0)
            lexer.advance()
        }
        return result
    }

    private fun pythonAt(tokens: List<Lexeme>, offset: Int): Boolean =
        tokens.firstOrNull { offset in it.start until it.end }?.python == true ||
            tokens.lastOrNull { it.end == offset }?.python == true

    private fun importedGrammar(source: String, file: Path?, root: Path?): List<IspaSymbol> {
        if (file == null) return emptyList()
        val found = ArrayList<IspaSymbol>()
        for (match in grammarImport.findAll(source)) {
            val module = match.groupValues[1]
            val relative = module.replace('.', '/') + ".isc"
            val candidates = listOfNotNull(file.parent?.resolve(relative), root?.resolve(relative), root?.resolve("parser/ispa/grammar")?.resolve(relative))
            val path = candidates.firstOrNull { Files.isRegularFile(it) } ?: continue
            val imported = cached(path, grammarCache, IspaSymbols::declarations)
            val selected = Regex("\\{([^}]*)}").find(match.value)?.groupValues?.get(1)?.split(',')?.map { it.trim().substringAfterLast("->") }?.toSet()
            found += imported.filter { selected == null || it.name in selected }
        }
        return found
    }

    private fun relatedGrammarSources(source: String, file: Path?, root: Path?): List<String> {
        val sources = arrayListOf(source)
        for (match in grammarImport.findAll(source)) {
            val relative = match.groupValues[1].replace('.', '/') + ".isc"
            val path = listOfNotNull(file?.parent?.resolve(relative), root?.resolve(relative),
                root?.resolve("parser/ispa/grammar")?.resolve(relative)).firstOrNull { Files.isRegularFile(it) } ?: continue
            sources += cached(path, sourceCache) { listOf(it) }
        }
        return sources
    }

    private fun dynamicMembers(source: String, scope: String, file: Path?, root: Path?): List<EditorSuggestion> {
        val result = LinkedHashMap<String, EditorSuggestion>()
        val field = Regex("\\bctx\\.$scope\\.([A-Za-z_]\\w*)(?:\\s*:\\s*([A-Za-z_]\\w*(?:\\[[^\\n]*\\])?))?")
        for (text in relatedGrammarSources(source, file, root)) {
            for (match in field.findAll(text)) {
                val name = match.groupValues[1]
                val detail = match.groupValues[2].takeIf { it.isNotEmpty() }
                if (detail != null || name !in result) result[name] = EditorSuggestion(name, "field", detail)
            }
            if (scope == "ast") {
                for (block in Regex("&=\\s*\\{([^}]*)}").findAll(text))
                    for (match in Regex("\\b([A-Za-z_]\\w*)\\s*:").findAll(block.groupValues[1]))
                        result.putIfAbsent(match.groupValues[1], EditorSuggestion(match.groupValues[1], "field"))
                for (block in Regex("@\\{([^}]*)}").findAll(text))
                    for (name in block.groupValues[1].split(',')) {
                        val member = name.trim()
                        if (member.matches(Regex("[A-Za-z_]\\w*"))) result.putIfAbsent(member, EditorSuggestion(member, "field"))
                    }
            }
        }
        return result.values.toList()
    }

    private fun pythonNames(source: String, before: Int, tokens: List<Lexeme>): List<EditorSuggestion> {
        val prefix = source.substring(0, before)
        val ruleStart = IspaSymbols.declarations(source).lastOrNull { it.offset < before }?.offset ?: 0
        val names = ArrayList<EditorSuggestion>()
        names += EditorSuggestion("ctx", "context")
        for (match in assignment.findAll(prefix)) if (match.range.first >= ruleStart && pythonAt(tokens, match.range.first))
            names += EditorSuggestion(match.groupValues[1], "variable", match.groupValues[2].takeIf { it.isNotEmpty() })
        for (match in function.findAll(prefix)) if (match.range.first >= ruleStart && pythonAt(tokens, match.range.first))
            names += EditorSuggestion(match.groupValues[1], if (match.value.startsWith("class")) "class" else "function")
        for (match in pythonImport.findAll(prefix)) if (match.range.first >= ruleStart && pythonAt(tokens, match.range.first)) {
            val name = match.groupValues[4].ifEmpty { match.groupValues[3].substringAfterLast('.').ifEmpty { match.groupValues[2] } }
            if (name.isNotEmpty()) names += EditorSuggestion(name, "module")
        }
        // Captures are visible to actions within their rule.
        val capture = Regex("[@%&]([A-Za-z_]\\w*)")
        for (match in capture.findAll(source.substring(ruleStart, before))) names += EditorSuggestion(match.groupValues[1], "capture")
        val parameters = Regex("\\bdef\\s+\\w+\\s*\\(([^)]*)\\)")
        for (match in parameters.findAll(prefix.substring(ruleStart))) if (pythonAt(tokens, ruleStart + match.range.first))
            for (parameter in match.groupValues[1].split(',')) {
                val name = parameter.trim().substringBefore(':').substringBefore('=').trim()
                if (name.matches(Regex("[A-Za-z_]\\w*"))) names += EditorSuggestion(name, "parameter")
            }
        return names.distinctBy { it.name }
    }

    private fun pythonTypes(source: String, before: Int, tokens: List<Lexeme>): Map<String, String> {
        val ruleStart = IspaSymbols.declarations(source).lastOrNull { it.offset < before }?.offset ?: 0
        val prefix = source.substring(ruleStart, before)
        val types = HashMap<String, String>()
        for (match in typedDeclaration.findAll(prefix)) if (pythonAt(tokens, ruleStart + match.range.first))
            types[match.groupValues[1]] = match.groupValues[2]
        for (match in assignment.findAll(prefix)) if (pythonAt(tokens, ruleStart + match.range.first)) {
            val name = match.groupValues[1]
            val annotated = match.groupValues[2].substringBefore('[')
            val value = match.groupValues[3].trim()
            val inferred = when {
                annotated.isNotEmpty() -> annotated
                value.startsWith("[") || Regex("^(?:list|range|sorted|reversed)\\s*\\(").containsMatchIn(value) -> "list"
                value.startsWith("{") || Regex("^(?:dict|zip)\\s*\\(").containsMatchIn(value) -> "dict"
                value.matches(Regex("[A-Za-z_]\\w*")) -> types[value]
                else -> null
            }
            if (inferred != null) types[name] = inferred
        }
        return types
    }

    fun suggestions(source: String, offset: Int, file: Path? = null, root: Path? = null): List<EditorSuggestion> {
        if (offset !in 0..source.length) return emptyList()
        val tokens = lex(source)
        val current = tokens.firstOrNull { offset in it.start until it.end }
        if (current?.type in setOf(IspaTokens.COMMENT, IspaTokens.STRING, IspaTokens.CHAR_CLASS, IspaTokens.PY_COMMENT, IspaTokens.PY_STRING)) return emptyList()
        val prefix = source.substring(0, offset)
        if (pythonAt(tokens, (offset - 1).coerceAtLeast(0))) {
            val line = prefix.substringAfterLast('\n').substringAfterLast('{')
            val importedNames = Regex("^\\s*from\\s+([\\w.]+)\\s+import\\s+\\w*$").find(line)
            if (importedNames != null && file != null) {
                val relative = importedNames.groupValues[1].replace('.', '/') + ".py"
                val path = listOfNotNull(file.parent?.resolve(relative), root?.resolve(relative)).firstOrNull { Files.isRegularFile(it) }
                if (path != null) return cached(path, pythonCache) { text ->
                    (function.findAll(text).map { EditorSuggestion(it.groupValues[1], "function") } +
                        assignment.findAll(text).map { EditorSuggestion(it.groupValues[1], "variable") }).distinctBy { it.name }.toList()
                }
            }
            if (Regex("^\\s*import\\s+[\\w.]*$").matches(line) || Regex("^\\s*from\\s+[\\w.]*$").matches(line)) return modules(file, root, ".py")
            val dynamic = Regex("ctx\\.(shared|ast)\\.\\w*$").find(prefix)
            if (dynamic != null) return dynamicMembers(source, dynamic.groupValues[1], file, root)
            val nestedDynamic = Regex("ctx\\.(shared|ast)\\.([A-Za-z_]\\w*)\\.\\w*$").find(prefix)
            if (nestedDynamic != null) {
                val member = dynamicMembers(source, nestedDynamic.groupValues[1], file, root)
                    .firstOrNull { it.name == nestedDynamic.groupValues[2] }
                return when (member?.detail?.substringBefore('[')) {
                    "list" -> listMethods.map { EditorSuggestion(it, "method", "list method") }
                    "dict" -> dictMethods.map { EditorSuggestion(it, "method", "dict method") }
                    else -> emptyList()
                }
            }
            if (Regex("ctx\\.\\w*$").containsMatchIn(prefix)) return ctxMembers.map { EditorSuggestion(it, "ctx member") }
            val receiver = Regex("([A-Za-z_]\\w*)\\.\\w*$").find(prefix)?.groupValues?.get(1)
            if (receiver != null) {
                when (pythonTypes(source, offset, tokens)[receiver]) {
                    "list" -> return listMethods.map { EditorSuggestion(it, "method", "list method") }
                    "dict" -> return dictMethods.map { EditorSuggestion(it, "method", "dict method") }
                }
                val imported = pythonImport.findAll(source).firstOrNull {
                    pythonAt(tokens, it.range.first) && (it.groupValues[4] == receiver || it.groupValues[3].substringAfterLast('.') == receiver)
                }
                if (imported != null && file != null) {
                    val module = imported.groupValues[3]
                    val path = file.parent?.resolve(module.replace('.', '/') + ".py")
                    if (path != null && Files.isRegularFile(path)) {
                        return cached(path, pythonCache) { text ->
                            (function.findAll(text).map { EditorSuggestion(it.groupValues[1], "function") } +
                                assignment.findAll(text).map { EditorSuggestion(it.groupValues[1], "variable") }).distinctBy { it.name }.toList()
                        }
                    }
                }
                return emptyList()
            }
            val names = pythonNames(source, offset, tokens)
            val expression = Regex("(?:=|\\breturn|\\byield|\\bif|\\belif|\\bwhile|\\(|,|\\bnot|\\band|\\bor)\\s*[A-Za-z_]*$").containsMatchIn(line)
            val annotation = Regex(":\\s*[A-Za-z_]*$").containsMatchIn(line) && !Regex("\\b(?:if|while|for|def)\\b").containsMatchIn(line)
            if (annotation) return listOf("int", "float", "str", "bool", "list", "dict", "tuple", "None").map { EditorSuggestion(it, "type") }
            if (!expression) return names.filter { it.kind in setOf("variable", "capture", "parameter", "context") }
            return (names + stdfunFunctions + builtins.map { EditorSuggestion(it, "builtin") }).distinctBy { it.name }
        }
        val statement = prefix.substringAfterLast(';').substringAfterLast('\n')
        if (Regex("^\\s*import\\s+[\\w.]*$").matches(statement) || Regex("\\bfrom\\s+[\\w.]*$").containsMatchIn(statement))
            return modules(file, root, ".isc")
        if (!IspaSymbols.acceptsCompletion(source, offset)) return emptyList()
        val local = IspaSymbols.declarations(source)
        return (local + importedGrammar(source, file, root)).distinctBy { it.name }
            .map { EditorSuggestion(it.name, if (it.token) "token" else "rule") }
    }

    fun problems(source: String): List<EditorProblem> {
        val tokens = lex(source)
        val result = ArrayList<EditorProblem>()
        val declarations = IspaSymbols.declarations(source)
        for ((_, duplicate) in declarations.groupBy { it.name }) for (item in duplicate.drop(1))
            result += EditorProblem(item.offset, item.offset + item.name.length, "Duplicate rule '${item.name}'")
        for (match in Regex("#\\[skip]\\s*([A-Za-z_]\\w*)\\s*:").findAll(source)) {
            val name = match.groupValues[1]
            val position = match.groups[1]!!.range.first
            if (tokens.any { position in it.start until it.end && !it.python } && !name.first().isUpperCase())
                result += EditorProblem(position, position + name.length, "GRM102: [skip] can only be applied to token rules")
        }
        val tokenNames = declarations.filter { it.token }.mapTo(HashSet()) { it.name }
        for (match in Regex("@([A-Za-z_]\\w*)").findAll(source)) {
            val start = match.range.first
            if (tokens.any { start in it.start until it.end && (it.python || it.type == IspaTokens.COMMENT || it.type == IspaTokens.STRING) }) continue
            if (match.groupValues[1] in tokenNames)
                result += EditorProblem(start, match.range.last + 1, "G305: '@${match.groupValues[1]}' names a capture; use '@ ${match.groupValues[1]}' to match the token", true)
        }
        for (match in ctxMember.findAll(source)) {
            val start = match.groups[1]!!.range.first
            if (tokens.any { start in it.start until it.end && it.python } && match.groupValues[1] !in ctxMembers)
                result += EditorProblem(start, start + match.groupValues[1].length, "PY261: unknown ctx member '${match.groupValues[1]}'")
        }
        val annotated = HashMap<String, String>()
        var assignmentRule = -1
        for (match in assignment.findAll(source)) {
            val start = match.range.first
            if (tokens.none { start in it.start until it.end && it.python }) continue
            val rule = declarations.lastOrNull { it.offset < start }?.offset ?: 0
            if (rule != assignmentRule) { annotated.clear(); assignmentRule = rule }
            val type = match.groupValues[2].substringBefore('[')
            val value = match.groupValues[3].trim()
            val actual = when {
                value.matches(Regex("[+-]?\\d+")) -> "int"
                value.matches(Regex("[+-]?\\d+\\.\\d+")) -> "float"
                value.startsWith("\"") || value.startsWith("'") -> "str"
                value == "True" || value == "False" -> "bool"
                value.startsWith("[") -> "list"
                value.startsWith("{") -> "dict"
                else -> ""
            }
            if (type.isNotEmpty() && actual.isNotEmpty() && actual != type && !(type == "float" && actual in setOf("int", "bool")) && !(type == "int" && actual == "bool")) {
                val at = match.groups[3]!!.range.first
                result += EditorProblem(at, at + value.length, "PY245: annotated assignment expects $type, got $actual")
            }
            if (type.isEmpty() && actual.isNotEmpty()) {
                val expected = annotated[match.groupValues[1]]
                if (expected != null && actual != expected && !(expected == "float" && actual in setOf("int", "bool")) && !(expected == "int" && actual == "bool")) {
                    val at = match.groups[3]!!.range.first
                    result += EditorProblem(at, at + value.length, "PY210: cannot assign $actual to '${match.groupValues[1]}' of type $expected", true)
                }
            }
            if (type.isNotEmpty()) annotated[match.groupValues[1]] = type
        }
        return result
    }

    fun syntaxProblems(source: String): List<EditorProblem> {
        if (source.isBlank()) return emptyList()
        val tokens = lex(source)
        val problems = ArrayList<EditorProblem>()
        for (token in tokens) {
            val value = source.substring(token.start, token.end)
            val message = when {
                token.type == IspaTokens.PY_NUMBER && !pythonNumber.matches(value) -> "Invalid Python numeric literal '$value'"
                token.type in setOf(IspaTokens.STRING, IspaTokens.PY_STRING) -> {
                    val quote = value.firstOrNull()
                    val triple = value.length >= 3 && value.take(3) == "$quote$quote$quote"
                    if (quote != null && (if (triple) value.length < 6 || !value.endsWith("$quote$quote$quote")
                        else value.length < 2 || value.last() != quote)) "Unterminated string" else null
                }
                token.type == IspaTokens.COMMENT && value.startsWith("/*") && !value.endsWith("*/") -> "Unterminated block comment"
                token.type == IspaTokens.CHAR_CLASS && !value.endsWith("]") -> "Unterminated character class"
                else -> null
            }
            if (message != null) problems += EditorProblem(token.start, token.end, message)
        }
        val lexer = IspaLexer()
        lexer.start(source, 0, source.length, 0)
        while (lexer.tokenType != null) lexer.advance()
        if (lexer.state ushr 8 != 0) problems += EditorProblem(source.length.coerceAtLeast(1) - 1, source.length, "Unclosed Python block or expression")
        val last = IspaSymbols.declarations(source).lastOrNull()
        if (last != null) {
            val suffix = source.substring(last.offset + last.name.length)
            if (suffix.trimStart().startsWith(":") && !suffix.trimEnd().endsWith(";"))
                problems += EditorProblem(source.length.coerceAtLeast(1) - 1, source.length, "Expected ';' after rule")
        } else if (source.trim().matches(Regex("[A-Za-z_]\\w*"))) {
            problems += EditorProblem(0, source.length, "Expected a rule declaration")
        }
        return problems
    }

}
