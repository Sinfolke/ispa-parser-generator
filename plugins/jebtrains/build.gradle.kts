import org.jetbrains.intellij.platform.gradle.TestFrameworkType

plugins {
    kotlin("jvm") version "2.0.21"
    id("org.jetbrains.intellij.platform") version "2.2.1"
}

group = "dev.ispa"
version = "0.3.4"

repositories { mavenCentral(); intellijPlatform { defaultRepositories() } }

dependencies {
    intellijPlatform {
        intellijIdeaCommunity("2024.3.5")
        testFramework(TestFrameworkType.Platform)
    }
    testImplementation(kotlin("test-junit"))
}

kotlin { jvmToolchain(21) }

intellijPlatform {
    pluginConfiguration {
        ideaVersion { sinceBuild = "243"; untilBuild = provider { null } }
        changeNotes = "Precise Python action errors and dynamic completion for ctx.shared and ctx.ast fields in local and imported grammars."
    }
}

val bridgeBuild = layout.buildDirectory.dir("native-build")
val isWindowsHost = System.getProperty("os.name").lowercase().contains("windows")
val bridgeFileName = "ispa-grammar-bridge" + if (isWindowsHost) ".exe" else ""
val bridgeExecutable = bridgeBuild.map { it.file(bridgeFileName) }
val hostArch = when (System.getProperty("os.arch").lowercase()) {
    "amd64", "x86_64" -> "x64"
    "aarch64", "arm64" -> "arm64"
    else -> error("Unsupported native bridge architecture: ${System.getProperty("os.arch")}")
}
val hostPlatform = when {
    isWindowsHost -> "windows-$hostArch"
    System.getProperty("os.name").lowercase().contains("mac") -> "macos-$hostArch"
    System.getProperty("os.name").lowercase().contains("linux") -> "linux-$hostArch"
    else -> error("Unsupported native bridge OS: ${System.getProperty("os.name")}")
}
// A release build supplies binaries from the four CI runners in this directory.
val releaseBridges = providers.gradleProperty("ispaReleaseBridges")
val compileIspaBridge by tasks.registering(Exec::class) {
    inputs.files("CMakeLists.txt", "parser_bridge.cpp", "../../parser/CMakeLists.txt")
    inputs.files(fileTree("../../parser/ispa") { include("**/*.h", "**/*.hpp", "**/*.cpp") })
    inputs.files(fileTree("../../parser/python") { include("**/*.h", "**/*.hpp", "**/*.cpp") })
    inputs.files(fileTree("../../parser/stdlibs") { include("CMakeLists.txt", "**/*.h", "**/*.hpp", "**/*.cpp") })
    listOf("CC", "CXX", "CFLAGS", "CXXFLAGS", "LDFLAGS", "LD").forEach { key -> inputs.property(key, System.getenv(key) ?: "") }
    outputs.file(bridgeBuild.map { it.file("CMakeCache.txt") })
    doFirst {
        bridgeBuild.get().asFile.deleteRecursively()
        bridgeBuild.get().asFile.mkdirs()
    }
    commandLine("cmake", "-S", projectDir.absolutePath, "-B", bridgeBuild.get().asFile.absolutePath,
        "-DISPA_ROOT=${projectDir.parentFile.parentFile.absolutePath}")
}
val buildIspaBridge by tasks.registering(Exec::class) {
    dependsOn(compileIspaBridge)
    inputs.files("CMakeLists.txt", "parser_bridge.cpp", "../../parser/CMakeLists.txt")
    inputs.files(fileTree("../../parser/ispa") { include("**/*.h", "**/*.hpp", "**/*.cpp") })
    inputs.files(fileTree("../../parser/python") { include("**/*.h", "**/*.hpp", "**/*.cpp") })
    inputs.files(fileTree("../../parser/stdlibs") { include("CMakeLists.txt", "**/*.h", "**/*.hpp", "**/*.cpp") })
    outputs.file(bridgeExecutable)
    commandLine("cmake", "--build", bridgeBuild.get().asFile.absolutePath, "--config", "Release")
}
val stageIspaBridge by tasks.registering(Copy::class) {
    doFirst { delete(layout.buildDirectory.dir("generated/ispa-resources/native")) }
    if (releaseBridges.isPresent) {
        from(releaseBridges.map { file(it) })
    } else {
        dependsOn(buildIspaBridge)
        from(bridgeExecutable) { into(hostPlatform) }
    }
    into(layout.buildDirectory.dir("generated/ispa-resources/native"))
}
val stageStdfun by tasks.registering(Copy::class) {
    from("../../parser/stdlibs/python/stdfun.py")
    into(layout.buildDirectory.dir("generated/ispa-resources/ispa"))
}
sourceSets.main { resources.srcDir(layout.buildDirectory.dir("generated/ispa-resources")) }
tasks.processResources { dependsOn(stageIspaBridge, stageStdfun) }

// This plugin contributes no searchable settings pages.
tasks.named("buildSearchableOptions") { enabled = false }
