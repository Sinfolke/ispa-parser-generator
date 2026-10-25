package dev.ispa.grammar

import com.google.gson.JsonParser
import com.intellij.lang.annotation.AnnotationHolder
import com.intellij.lang.annotation.ExternalAnnotator
import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.openapi.util.TextRange
import com.intellij.psi.PsiFile
import java.nio.charset.StandardCharsets.UTF_8
import java.nio.file.Files
import java.nio.file.Path
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import java.util.concurrent.ConcurrentHashMap

data class CompilerSnapshot(val source: String, val file: Path?, val root: Path?)
data class CompilerResult(val source: String, val problems: List<EditorProblem>)

/** Full compiler checks run only for saved snapshots, off the UI thread. */
class IspaCompilerAnnotator : ExternalAnnotator<CompilerSnapshot, CompilerResult>() {
    companion object {
        private val cache = ConcurrentHashMap<String, CompilerResult>()

        private fun executable(root: Path?): Path? {
            val configured = System.getProperty("ispa.compiler.path") ?: System.getenv("ISPA_COMPILER")
            if (!configured.isNullOrBlank()) return Path.of(configured).takeIf(Files::isExecutable)
            if (root == null) return null
            return listOf("cmake-build-release/ispa", "cmake-build-debug/ispa", "build/ispa")
                .map(root::resolve).firstOrNull(Files::isExecutable)
        }

        private fun utf16Offset(source: String, byteOffset: Int): Int {
            if (byteOffset <= 0) return 0
            var bytes = 0
            var chars = 0
            while (chars < source.length && bytes < byteOffset) {
                val count = Character.charCount(source.codePointAt(chars))
                bytes += source.substring(chars, chars + count).toByteArray(UTF_8).size
                chars += count
            }
            return chars
        }

        internal fun parse(source: String, file: Path, output: String): CompilerResult? {
            val line = output.lineSequence().lastOrNull { it.startsWith("{\"diagnostics\":") } ?: return null
            val items = JsonParser.parseString(line).asJsonObject.getAsJsonArray("diagnostics")
            val canonical = file.toAbsolutePath().normalize()
            val local = IspaEditorModel.problems(source).mapTo(HashSet()) { it.message }
            val problems = items.mapNotNull { item ->
                val obj = item.asJsonObject
                val target = runCatching { Path.of(obj["file"].asString).toAbsolutePath().normalize() }.getOrNull()
                if (target != canonical) return@mapNotNull null
                val raw = obj["offset"].asInt
                val start = if (raw >= 0) utf16Offset(source, raw) else {
                    val row = obj["line"].asInt
                    val column = obj["column"].asInt
                    if (row <= 0) return@mapNotNull null
                    val lineStart = source.lineSequence().take(row - 1).sumOf { it.length + 1 }
                    (lineStart + (column - 1).coerceAtLeast(0)).coerceAtMost(source.length)
                }
                val length = obj["length"].asInt.coerceAtLeast(1)
                val end = utf16Offset(source, raw.coerceAtLeast(0) + length).coerceAtLeast(start + 1).coerceAtMost(source.length)
                val code = obj["code"].asString
                if ("$code: ${obj["message"].asString}" in local) return@mapNotNull null
                EditorProblem(start.coerceAtMost(source.length), end, "$code: ${obj["message"].asString}", obj["severity"].asString != "error")
            }
            return CompilerResult(source, problems)
        }
    }

    override fun collectInformation(file: PsiFile): CompilerSnapshot {
        val path = file.virtualFile?.path?.let { runCatching { Path.of(it) }.getOrNull() }
        val root = file.project.basePath?.let { runCatching { Path.of(it) }.getOrNull() }
        return CompilerSnapshot(file.text, path, root)
    }

    override fun doAnnotate(collectedInfo: CompilerSnapshot?): CompilerResult? {
        val snapshot = collectedInfo ?: return null
        val file = snapshot.file ?: return null
        val compiler = executable(snapshot.root) ?: return null
        if (snapshot.source.length > 1_000_000 || !Files.isRegularFile(file)) return null
        if (runCatching { Files.readString(file) }.getOrNull() != snapshot.source) return null
        val stamp = runCatching { Files.getLastModifiedTime(compiler).toMillis() }.getOrNull() ?: return null
        val key = "${file.toAbsolutePath().normalize()}:${snapshot.source.hashCode()}:$stamp"
        cache[key]?.let { return it }
        var process: Process? = null
        return try {
            val running = ProcessBuilder(compiler.toString(), "--check-only", "-l", "cpp", file.toString())
                .directory(file.parent.toFile()).start()
            process = running
            val stdout = CompletableFuture.supplyAsync { running.inputStream.bufferedReader(UTF_8).use { it.readText() } }
            val stderr = CompletableFuture.runAsync { running.errorStream.use { it.copyTo(java.io.OutputStream.nullOutputStream()) } }
            if (!running.waitFor(1500, TimeUnit.MILLISECONDS)) return null
            val result = parse(snapshot.source, file, stdout.get(200, TimeUnit.MILLISECONDS)) ?: return null
            stderr.get(200, TimeUnit.MILLISECONDS)
            if (cache.size > 128) cache.clear()
            cache[key] = result
            result
        } catch (_: Exception) { null }
        finally { process?.destroyForcibly() }
    }

    override fun apply(file: PsiFile, annotationResult: CompilerResult?, holder: AnnotationHolder) {
        val result = annotationResult ?: return
        if (file.text != result.source) return
        for (problem in result.problems) {
            holder.newAnnotation(if (problem.warning) HighlightSeverity.WARNING else HighlightSeverity.ERROR, problem.message)
                .range(TextRange(problem.start, problem.end)).create()
        }
    }
}
