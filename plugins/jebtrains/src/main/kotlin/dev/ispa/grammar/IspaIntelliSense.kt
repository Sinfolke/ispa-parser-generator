package dev.ispa.grammar

import com.intellij.codeInsight.completion.CompletionContributor
import com.intellij.codeInsight.completion.CompletionParameters
import com.intellij.codeInsight.completion.CompletionResultSet
import com.intellij.codeInsight.lookup.LookupElementBuilder
import com.intellij.icons.AllIcons
import com.intellij.patterns.PlatformPatterns.psiElement
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiReference
import com.intellij.psi.PsiReferenceBase
import com.intellij.psi.PsiReferenceContributor
import com.intellij.psi.PsiReferenceProvider
import com.intellij.psi.PsiReferenceRegistrar
import com.intellij.util.ProcessingContext
import java.nio.file.Path

class IspaCompletionContributor : CompletionContributor() {
    override fun fillCompletionVariants(parameters: CompletionParameters, result: CompletionResultSet) {
        val file = parameters.originalFile
        if (file.language != IspaLanguage.INSTANCE) return
        val source = file.text
        val offset = parameters.offset.coerceAtMost(source.length)
        val path = file.virtualFile?.path?.let { runCatching { Path.of(it) }.getOrNull() }
        val root = file.project.basePath?.let { runCatching { Path.of(it) }.getOrNull() }
        for (symbol in IspaEditorModel.suggestions(source, offset, path, root)) {
            val icon = when (symbol.kind) {
                "module" -> AllIcons.Nodes.Package
                "rule", "function", "builtin", "method" -> AllIcons.Nodes.Method
                "token" -> AllIcons.Nodes.Constant
                "class", "type" -> AllIcons.Nodes.Class
                "capture", "parameter" -> AllIcons.Nodes.Parameter
                "ctx member" -> AllIcons.Nodes.Field
                else -> AllIcons.Nodes.Variable
            }
            result.addElement(LookupElementBuilder.create(symbol.name).withTypeText(symbol.detail ?: symbol.kind).withIcon(icon))
        }
    }
}

class IspaReferenceContributor : PsiReferenceContributor() {
    override fun registerReferenceProviders(registrar: PsiReferenceRegistrar) {
        registrar.registerReferenceProvider(psiElement().withLanguage(IspaLanguage.INSTANCE), object : PsiReferenceProvider() {
            override fun getReferencesByElement(element: PsiElement, context: ProcessingContext): Array<PsiReference> {
                val file = element.containingFile ?: return PsiReference.EMPTY_ARRAY
                val source = file.text
                val offset = element.textRange.startOffset
                val name = IspaSymbols.referenceAt(source, offset) ?: return PsiReference.EMPTY_ARRAY
                if (element.text != name || IspaSymbols.declarations(source).none { it.name == name }) return PsiReference.EMPTY_ARRAY
                return arrayOf(object : PsiReferenceBase<PsiElement>(element) {
                    override fun resolve(): PsiElement? {
                        val target = IspaSymbols.declarations(file.text).firstOrNull { it.name == name } ?: return null
                        return file.findElementAt(target.offset)
                    }

                    override fun getVariants(): Array<Any> = IspaSymbols.declarations(file.text)
                        .map { LookupElementBuilder.create(it.name) }.toTypedArray()
                })
            }
        })
    }
}
