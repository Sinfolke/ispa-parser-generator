package dev.ispa.grammar

import com.intellij.psi.tree.IElementType
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import java.nio.file.Files

class HighlightingTest {
    private data class Lexeme(val start: Int, val end: Int, val state: Int, val type: IElementType)
    private fun lex(text: String, start: Int = 0, state: Int = 0): List<Lexeme> {
        val lexer = IspaLexer()
        lexer.start(text, start, text.length, state)
        val result = mutableListOf<Lexeme>()
        while (lexer.tokenType != null) {
            assertTrue(lexer.tokenEnd > lexer.tokenStart)
            result += Lexeme(lexer.tokenStart, lexer.tokenEnd, lexer.state, lexer.tokenType!!)
            lexer.advance()
        }
        return result
    }
    @Test fun highlightsModulesCommentsAndDeclarations() {
        val text = "module core; /* comment */ TOKEN : [a-z]+; entry: TOKEN?;"
        val tokens = lex(text).associate { text.substring(it.start, it.end) to it.type }
        assertEquals(IspaTokens.KEYWORD, tokens["module"])
        assertEquals(IspaTokens.MODULE, tokens["core"])
        assertEquals(IspaTokens.COMMENT, tokens["/* comment */"])
        assertEquals(IspaTokens.TOKEN_REFERENCE, tokens["TOKEN"])
        assertEquals(IspaTokens.RULE_DECLARATION, tokens["entry"])
        assertEquals(IspaTokens.CHAR_CLASS, tokens["[a-z]"])
    }
    @Test fun embeddedPythonRestartsAtEveryTokenBoundary() {
        val text = "entry { if True: value = {'x': [1, 2]} # } ignored\nreturn value }; tail: 'y';"
        val all = lex(text)
        for ((index, token) in all.withIndex()) assertEquals(all.drop(index), lex(text, token.start, token.state))
        val tokens = all.associate { text.substring(it.start, it.end) to it.type }
        assertEquals(IspaTokens.PY_KEYWORD, tokens["if"])
        assertEquals(IspaTokens.PY_KEYWORD, tokens["return"])
        assertEquals(IspaTokens.PY_COMMENT, tokens["# } ignored"])
        assertEquals(IspaTokens.RULE_DECLARATION, tokens["tail"])
    }
    @Test fun decoderKeepsFailedParseTokensAndConvertsUnicodeOffsets() {
        val source = "// 😀\nentry"
        val json = """{"ok":false,"tokens":[{"kind":"modules.common.ID","role":"declaration","start":8,"length":5}],"diagnostics":[{"message":"Expected \";\"","start":13,"length":0}]}"""
        val result = NativeParserBridge.decode(source, json)
        assertEquals(6, result.tokens.single().start)
        assertEquals(source.length, result.diagnostics.single().start)
        assertEquals("Expected \";\"", result.diagnostics.single().message)
    }
    @Test fun nativeAnnotationsDoNotRepaintPythonIdentifiers() {
        val source = "fail recover { ctx.insert() }"
        val start = source.indexOf("ctx")
        val json = """{"tokens":[{"kind":"ID","role":"","start":$start,"length":3}],"diagnostics":[]}"""
        assertTrue(NativeParserBridge.decode(source, json).tokens.isEmpty())
    }
    @Test fun externalImportRestartsAtEveryTokenBoundary() {
        val text = "import extern python ispa_python as py; #[global] entry: 'x';"
        val all = lex(text)
        for ((index, token) in all.withIndex()) assertEquals(all.drop(index), lex(text, token.start, token.state))
        val tokens = all.associate { text.substring(it.start, it.end) to it.type }
        assertEquals(IspaTokens.KEYWORD, tokens["extern"])
        assertEquals(IspaTokens.MODULE, tokens["python"])
        assertEquals(IspaTokens.MODULE, tokens["ispa_python"])
        assertEquals(IspaTokens.MODULE, tokens["py"])
        assertEquals(IspaTokens.KEYWORD, tokens["global"])
        assertEquals(IspaTokens.RULE_DECLARATION, tokens["entry"])
    }
    @Test fun directPythonBlocksHavePythonColors() {
        val source = "fail recover {\n    if ctx.at('x'):\n        ctx.insert()\n}\nentry { match TOKEN; return ctx.ast }\nvalue: ${'$'}{ctx.at('x')} {ctx.ast};"
        val tokens = lex(source)
        fun typeAt(needle: String): IElementType = tokens.first { it.start == source.indexOf(needle) }.type
        assertEquals(IspaTokens.PY_KEYWORD, typeAt("if ctx"))
        assertEquals(IspaTokens.PY_IDENTIFIER, typeAt("ctx.at"))
        assertEquals(IspaTokens.PY_CALL, typeAt("at('x')"))
        assertEquals(IspaTokens.PY_KEYWORD, typeAt("return ctx"))
        assertEquals(IspaTokens.PY_IDENTIFIER, typeAt("ctx.ast"))
        for ((index, token) in tokens.withIndex()) assertEquals(tokens.drop(index), lex(source, token.start, token.state))
    }
    @Test fun entirePythonBodyHasDistinctColors() {
        val source = "fail recover {\n    total = len(ctx.items) + 12\n    ctx.label = {'name': total}\n    # done\n}"
        val tokens = lex(source)
        fun typeAt(text: String) = tokens.first { it.start == source.indexOf(text) }.type
        assertEquals(IspaTokens.PY_IDENTIFIER, typeAt("total ="))
        assertEquals(IspaTokens.PY_BUILTIN, typeAt("len("))
        assertEquals(IspaTokens.PY_IDENTIFIER, typeAt("ctx.items"))
        assertEquals(IspaTokens.PY_ATTRIBUTE, typeAt("items"))
        assertEquals(IspaTokens.PY_ATTRIBUTE, typeAt("label"))
        assertEquals(IspaTokens.PY_NUMBER, typeAt("12"))
        assertEquals(IspaTokens.PY_STRING, typeAt("'name'"))
        assertEquals(IspaTokens.PY_COMMENT, typeAt("# done"))
        assertEquals(IspaTokens.PY_OPERATOR, typeAt("+ 12"))
        assertEquals(IspaTokens.PY_PUNCTUATION, typeAt(": total"))
        assertEquals(IspaTokens.PY_BRACE, typeAt("{'name'"))
        for (token in tokens.filter { it.state ushr 8 > 0 && !source.substring(it.start, it.end).isBlank() }) {
            assertTrue(IspaSyntaxHighlighter.getTokenHighlights(token.type).isNotEmpty(), "Missing color for ${source.substring(token.start, token.end)}")
        }
    }
    @Test fun unbracedPythonExpressionStaysColoredUntilGrammarBoundary() {
        val source = "entry: 'a' =len(ctx.value) + 1; tail: 'b';"
        val tokens = lex(source)
        fun typeAt(text: String) = tokens.first { it.start == source.indexOf(text) }.type
        assertEquals(IspaTokens.PY_OPERATOR, typeAt("=len"))
        assertEquals(IspaTokens.PY_BUILTIN, typeAt("len("))
        assertEquals(IspaTokens.PY_IDENTIFIER, typeAt("ctx.value"))
        assertEquals(IspaTokens.PY_ATTRIBUTE, typeAt("value"))
        assertEquals(IspaTokens.PY_NUMBER, typeAt("1;"))
        assertEquals(IspaTokens.RULE_DECLARATION, typeAt("tail:"))
        for ((index, token) in tokens.withIndex()) assertEquals(tokens.drop(index), lex(source, token.start, token.state))
    }
    @Test fun completionIndexSkipsPythonAndFindsNestedRules() {
        val source = "module demo;\nentry: TOKEN\n    #child: entry;\n;\nTOKEN: [a-z];\nfail recover {\n    fake: int = 1\n}\n"
        assertEquals(listOf("entry", "child", "TOKEN", "recover"), IspaSymbols.declarations(source).map { it.name })
        assertEquals("TOKEN", IspaSymbols.referenceAt(source, source.indexOf("TOKEN")))
        assertEquals(null, IspaSymbols.referenceAt(source, source.indexOf("fake")))
        assertEquals(false, IspaSymbols.acceptsCompletion(source, source.indexOf("fake") + 2))
        assertEquals(listOf("A", "main"), IspaSymbols.declarations("A: 'a'; main: A;").map { it.name })
    }
    @Test fun completionUsesTheCurrentLanguageAndPythonScope() {
        val source = "entry: TOKEN {count: int = 1\nctx.}; TOKEN: 'x';"
        assertEquals(listOf("entry", "TOKEN"), IspaEditorModel.suggestions(source, source.indexOf("TOKEN {")).map { it.name })
        val python = IspaEditorModel.suggestions(source, source.indexOf("ctx.") + 2).map { it.name }
        assertTrue("count" in python)
        assertTrue("ctx" in python)
        assertTrue("TOKEN" !in python)
        val members = IspaEditorModel.suggestions(source, source.indexOf("ctx.") + 4).map { it.name }
        assertTrue("at" in members)
        assertTrue("ast" in members)
        val scoped = "import grammar_module; first { old_name = 1 } second { new_name = 2\nnew_ }"
        val scopedNames = IspaEditorModel.suggestions(scoped, scoped.indexOf("new_ }" ) + 4).map { it.name }
        assertTrue("new_name" in scopedNames)
        assertTrue("old_name" !in scopedNames)
        assertTrue("grammar_module" !in scopedNames)
        assertEquals(listOf("first", "second"), IspaSymbols.declarations(scoped).map { it.name })
        assertTrue("dict" !in scopedNames)
        val expression = "entry { value = 1\nother = val }"
        val expressionNames = IspaEditorModel.suggestions(expression, expression.indexOf("= val") + 5).map { it.name }
        assertTrue("value" in expressionNames)
        assertTrue("dict" in expressionNames)
        val annotationNames = IspaEditorModel.suggestions("entry { value: in }", 17).map { it.name }
        assertTrue("int" in annotationNames)
        assertTrue("ctx" !in annotationNames)
    }
    @Test fun listAndDictMembersFollowInferredTypes() {
        val listSource = "entry { values: list[int] = []\nvalues.ap }"
        val listNames = IspaEditorModel.suggestions(listSource, listSource.indexOf("values.ap") + 9).map { it.name }
        assertTrue("append" in listNames)
        assertTrue("keys" !in listNames)
        val dictSource = "entry { mapping = {'a': 1}\nmapping.ke }"
        val dictNames = IspaEditorModel.suggestions(dictSource, dictSource.indexOf("mapping.ke") + 10).map { it.name }
        assertTrue("keys" in dictNames)
        assertTrue("append" !in dictNames)
    }
    @Test fun sharedAndAstMembersComeFromGrammarSource() {
        val source = "first { ctx.shared.indents: list[int]\nctx.ast.value = 1 }\nsecond { ctx.shared. }\nthird { ctx.ast. }\nfourth { ctx.shared.indents.ap }"
        val shared = IspaEditorModel.suggestions(source, source.indexOf("ctx.shared. }") + 11).map { it.name }
        assertTrue("indents" in shared)
        val ast = IspaEditorModel.suggestions(source, source.indexOf("ctx.ast. }") + 8).map { it.name }
        assertTrue("value" in ast)
        val methods = IspaEditorModel.suggestions(source, source.indexOf("indents.ap") + 10).map { it.name }
        assertTrue("append" in methods)
    }
    @Test fun stdfunFunctionsComeFromBundledSource() {
        val source = "entry { result = ran }"
        val names = IspaEditorModel.suggestions(source, source.indexOf("ran") + 3)
        assertTrue(names.any { it.name == "range" && it.detail?.contains("list[int]") == true })
        assertTrue(names.any { it.name == "zip" && it.detail?.contains("dict") == true })
    }
    @Test fun localSyntaxDiagnosticsWorkWithoutNativeHelper() {
        assertTrue(IspaEditorModel.syntaxProblems("entry: 'x'").any { "Expected ';'" in it.message })
        assertTrue(IspaEditorModel.syntaxProblems("entry: 'x'", includeGrammarFallback = false).none { "Expected ';'" in it.message })
        assertTrue(IspaEditorModel.syntaxProblems("entry { value").any { "Unclosed Python" in it.message })
        assertTrue(IspaEditorModel.syntaxProblems("entry: 'unfinished").any { "Unterminated string" in it.message })
        val invalid = "NEWLINE {\n    234234sdf[]w[]r\n    l = []\n}"
        val numeric = IspaEditorModel.syntaxProblems(invalid).first { "Invalid Python numeric" in it.message }
        assertEquals(invalid.indexOf("234234sdf"), numeric.start)
    }
    @Test fun sidecarAndGrammarImportsOfferSymbols() {
        val dir = Files.createTempDirectory("ispa-editor-")
        try {
            Files.writeString(dir.resolve("common.isc"), "TOKEN: 'x'; imported: TOKEN; helper { ctx.shared.external: list[int] }")
            Files.writeString(dir.resolve("helper.py"), "def convert(value):\n    return value\n")
            val grammar = "import common; entry: TOKEN; action { import helper\nhelper. }"
            val grammarNames = IspaEditorModel.suggestions(grammar, grammar.indexOf("TOKEN;") + 1, dir.resolve("main.isc"), dir).map { it.name }
            assertTrue("imported" in grammarNames)
            val pythonNames = IspaEditorModel.suggestions(grammar, grammar.indexOf("helper.") + 7, dir.resolve("main.isc"), dir).map { it.name }
            assertTrue("convert" in pythonNames)
            val grammarModules = IspaEditorModel.suggestions("import co", 9, dir.resolve("main.isc"), dir).map { it.name }
            assertTrue("common" in grammarModules)
            assertTrue("TOKEN" !in grammarModules)
            val pythonModules = IspaEditorModel.suggestions("action { import he }", 18, dir.resolve("main.isc"), dir).map { it.name }
            assertTrue("helper" in pythonModules)
            assertTrue("dict" !in pythonModules)
            val importedNames = IspaEditorModel.suggestions("action { from helper import con }", 31, dir.resolve("main.isc"), dir).map { it.name }
            assertTrue("convert" in importedNames)
            val dynamic = "import common; action { ctx.shared. }"
            val shared = IspaEditorModel.suggestions(dynamic, dynamic.indexOf("ctx.shared.") + 11, dir.resolve("main.isc"), dir).map { it.name }
            assertTrue("external" in shared)
        } finally {
            Files.deleteIfExists(dir.resolve("common.isc"))
            Files.deleteIfExists(dir.resolve("helper.py"))
            Files.deleteIfExists(dir)
        }
    }
    @Test fun editorSemanticChecksFollowCompilerCodes() {
        val source = "TOKEN: 'x'; TOKEN: 'y'; #[skip] entry: @TOKEN; action { value: int = 'wrong'\nvalue = 'wrong'\nctx.invalid() }"
        val messages = IspaEditorModel.problems(source).map { it.message }
        assertTrue(messages.any { "Duplicate rule" in it })
        assertTrue(messages.any { "G305" in it })
        assertTrue(messages.any { "PY245" in it })
        assertTrue(messages.any { "PY261" in it })
        assertTrue(messages.any { "GRM102" in it })
        assertTrue(messages.any { "PY210" in it })
    }
    @Test fun compilerJsonMapsUtf8OffsetsToEditorOffsets() {
        val file = java.nio.file.Path.of("/tmp/semantic.isc")
        val source = "// 😀\nentry: 'x';"
        val offset = source.substringBefore("entry").toByteArray(Charsets.UTF_8).size
        val json = """{"diagnostics":[{"severity":"warning","code":"G103","message":"unused capture","file":"/tmp/semantic.isc","offset":$offset,"length":5,"line":0,"column":0}]}"""
        val result = IspaCompilerAnnotator.parse(source, file, json)!!
        assertEquals(source.indexOf("entry"), result.problems.single().start)
        assertTrue(result.problems.single().warning)
    }
    @Test fun packagedNativeHelperValidatesPythonAndExternalImports() {
        val valid = NativeParserBridge.parse("import extern python ispa_python as python; entry: 'x' {value};")
        assertTrue(valid != null, "Packaged native helper must run")
        assertTrue(valid.diagnostics.isEmpty(), valid.diagnostics.toString())
        val invalid = NativeParserBridge.parse("entry: 'x' {value + };")
        assertTrue(invalid != null, "Packaged native helper must run")
        assertTrue(invalid.diagnostics.isNotEmpty())
    }
    @Test fun unfinishedLexemesStillCoverWholeFile() {
        for (text in listOf("entry: 'unfinished", "/* incomplete", "entry { '''long\nstring", "entry: [a\\]")) {
            assertEquals(text.length, lex(text).last().end)
        }
    }
    @Test fun packagedNativeHelperReportsDiagnostics() {
        val result = NativeParserBridge.parse("entry: 'x'")
        assertTrue(result != null, "Packaged native helper must run")
        assertTrue(result.tokens.isNotEmpty())
        assertEquals(10, result.diagnostics.single().start)
        assertEquals("Expected ';' to end the regular rule.", result.diagnostics.single().message)
    }
}
