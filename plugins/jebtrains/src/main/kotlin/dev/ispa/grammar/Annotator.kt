package dev.ispa.grammar

import com.google.gson.JsonParser
import com.intellij.lang.annotation.AnnotationHolder
import com.intellij.lang.annotation.Annotator
import com.intellij.lang.annotation.ExternalAnnotator
import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.openapi.progress.ProcessCanceledException
import com.intellij.openapi.util.TextRange
import com.intellij.psi.PsiFile
import com.intellij.psi.PsiElement
import com.intellij.psi.tree.IElementType
import java.nio.charset.StandardCharsets.UTF_8
import java.nio.file.Files
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit

data class BridgeToken(val start: Int, val end: Int, val type: IElementType)
data class BridgeDiagnostic(val start: Int, val end: Int, val message: String)
data class BridgeResult(val source: String, val tokens: List<BridgeToken>, val diagnostics: List<BridgeDiagnostic>)

object NativeParserBridge {
    private val executable: java.io.File? by lazy {
        try {
            val os = System.getProperty("os.name").lowercase()
            val arch = when (System.getProperty("os.arch").lowercase()) {
                "amd64", "x86_64" -> "x64"
                "aarch64", "arm64" -> "arm64"
                else -> return@lazy null
            }
            val platform = when {
                os.contains("windows") -> "windows-$arch"
                os.contains("mac") -> "macos-$arch"
                os.contains("linux") -> "linux-$arch"
                else -> return@lazy null
            }
            val suffix = if (os.contains("windows")) ".exe" else ""
            val resource = "/native/$platform/ispa-grammar-bridge$suffix"
            val stream = IspaLexer::class.java.getResourceAsStream(resource) ?: return@lazy null
            val file = Files.createTempFile("ispa-grammar-bridge-", suffix).toFile()
            stream.use { input -> file.outputStream().use(input::copyTo) }
            file.setExecutable(true); file.deleteOnExit(); file
        } catch (_: Exception) { null }
    }

    val isAvailable: Boolean get() = executable != null

    internal fun decode(source: String, json: String): BridgeResult {
        val root = JsonParser.parseString(json).asJsonObject
        val pythonOffsets = BooleanArray(source.length)
        val lexer = IspaLexer()
        lexer.start(source, 0, source.length, 0)
        while (lexer.tokenType != null) {
            if (lexer.state ushr 8 != 0) {
                for (offset in lexer.tokenStart until lexer.tokenEnd) pythonOffsets[offset] = true
            }
            lexer.advance()
        }
        // Native ranges count UTF-8 bytes; editor ranges count UTF-16 code units.
        val offsets = IntArray(source.toByteArray(UTF_8).size + 1)
        var byte = 0
        var char = 0
        while (char < source.length) {
            val count = Character.charCount(source.codePointAt(char))
            val bytes = source.substring(char, char + count).toByteArray(UTF_8).size
            repeat(bytes) { offsets[byte++] = char }
            char += count
        }
        offsets[byte] = source.length
        fun range(start: Int, length: Int): Pair<Int, Int> {
            val a = start.coerceIn(0, offsets.lastIndex)
            val b = (start.toLong() + length.coerceAtLeast(0)).coerceIn(a.toLong(), offsets.lastIndex.toLong()).toInt()
            return offsets[a] to offsets[b]
        }
        val tokens = root.getAsJsonArray("tokens").mapNotNull { element ->
            val obj = element.asJsonObject
            val (start, end) = range(obj["start"].asInt, obj["length"].asInt)
            if (end <= start) return@mapNotNull null
            if (start < pythonOffsets.size && pythonOffsets[start]) return@mapNotNull null
            val word = source.substring(start, end)
            val upper = word.any { it.isLetter() } && word.all { !it.isLetter() || it.isUpperCase() }
            val kind = obj["kind"].asString.substringAfterLast('.').substringAfterLast('_')
            val type = when (obj.get("role")?.asString) {
                "declaration" -> if (upper) IspaTokens.TOKEN_DECLARATION else IspaTokens.RULE_DECLARATION
                "parameter" -> IspaTokens.PARAMETER
                "capture" -> IspaTokens.CAPTURE
                else -> when (kind) {
                    "ID" -> if (upper) IspaTokens.TOKEN_REFERENCE else IspaTokens.ID
                    else -> return@mapNotNull null // Preserve the tolerant lexer's strings, Python and punctuation.
                }
            }
            BridgeToken(start, end, type)
        }
        val diagnostics = root.getAsJsonArray("diagnostics")?.map { element ->
            val obj = element.asJsonObject
            val (start, end) = range(obj["start"].asInt, obj["length"].asInt)
            BridgeDiagnostic(start, end, obj["message"].asString)
        } ?: emptyList()
        return BridgeResult(source, tokens, diagnostics)
    }

    fun parse(source: String): BridgeResult? {
        val binary = executable ?: return null
        var process: Process? = null
        return try {
            val running = ProcessBuilder(binary.absolutePath).start()
            process = running
            val output = CompletableFuture.supplyAsync { running.inputStream.bufferedReader(UTF_8).use { it.readText() } }
            val errors = CompletableFuture.runAsync { running.errorStream.use { it.copyTo(java.io.OutputStream.nullOutputStream()) } }
            running.outputStream.use { it.write(source.toByteArray(UTF_8)) }
            if (!running.waitFor(2000, TimeUnit.MILLISECONDS)) return null
            val json = output.get(1, TimeUnit.SECONDS)
            errors.get(1, TimeUnit.SECONDS)
            if (running.exitValue() != 0) null else decode(source, json)
        } catch (e: ProcessCanceledException) { throw e }
        catch (_: Exception) { null }
        finally { process?.destroyForcibly() }
    }
}

class IspaNativeAnnotator : ExternalAnnotator<String, BridgeResult>() {
    override fun collectInformation(file: PsiFile): String = file.text
    override fun doAnnotate(collectedInfo: String?): BridgeResult? = collectedInfo?.let(NativeParserBridge::parse)
    override fun apply(file: PsiFile, annotationResult: BridgeResult?, holder: AnnotationHolder) {
        val result = annotationResult ?: return
        if (file.text != result.source) return
        for (token in result.tokens) {
            val key = IspaSyntaxHighlighter.getTokenHighlights(token.type).firstOrNull() ?: continue
            holder.newSilentAnnotation(HighlightSeverity.INFORMATION)
                .range(TextRange(token.start, token.end)).textAttributes(key).create()
        }
        for (error in result.diagnostics) {
            val annotation = holder.newAnnotation(HighlightSeverity.ERROR, error.message)
                .range(TextRange(error.start, error.end))
            if (error.start == result.source.length) annotation.afterEndOfLine()
            annotation.create()
        }
    }
}

class IspaLiveAnnotator : Annotator {
    override fun annotate(element: PsiElement, holder: AnnotationHolder) {
        val file = element as? PsiFile ?: return
        if (file.language != IspaLanguage.INSTANCE) return
        val source = file.text
        val syntax = IspaEditorModel.syntaxProblems(source, includeGrammarFallback = !NativeParserBridge.isAvailable)
        for (problem in syntax + IspaEditorModel.problems(source)) {
            holder.newAnnotation(if (problem.warning) HighlightSeverity.WARNING else HighlightSeverity.ERROR, problem.message)
                .range(TextRange(problem.start, problem.end)).create()
        }
    }
}
