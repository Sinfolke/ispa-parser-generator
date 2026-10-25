package dev.ispa.grammar

import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import kotlin.test.assertTrue

class IspaHighlightingIntegrationTest : BasePlatformTestCase() {
    fun testLiveErrorsReachTheEditor() {
        myFixture.configureByText(IspaFileType.INSTANCE, "entry: 'x'")
        val errors = myFixture.doHighlighting().filter { it.severity == HighlightSeverity.ERROR }
        assertTrue(errors.any { it.description?.contains("Expected ';'") == true }, errors.toString())
    }

    fun testBundledParserErrorsReachTheEditor() {
        myFixture.configureByText(IspaFileType.INSTANCE, "entry: 'x' + +;")
        val errors = myFixture.doHighlighting().filter { it.severity == HighlightSeverity.ERROR }
        assertTrue(errors.any { it.description?.contains("Expected") == true || it.description?.contains("Parsing stopped") == true }, errors.toString())
    }

    fun testMalformedPythonIsMarkedOnItsOwnLine() {
        val source = "NEWLINE {\n    234234sdf[]w[]r\n    l = []\n}"
        myFixture.configureByText(IspaFileType.INSTANCE, source)
        val errors = myFixture.doHighlighting().filter { it.severity == HighlightSeverity.ERROR }
        assertTrue(errors.any { it.description?.contains("Invalid Python numeric literal") == true && it.startOffset == source.indexOf("234234sdf") }, errors.toString())
    }
}
