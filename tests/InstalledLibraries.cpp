#include <gtest/gtest.h>
import std;
import InstalledLibraries;

TEST(InstalledLibraries, CoreSelectionMatchesTargetLanguage) {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / ("ispa-stdlib-selection-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    struct Cleanup { fs::path path; ~Cleanup() { fs::remove_all(path); } } cleanup{root};
    for (const auto* language : {"cpp", "python", "rust"}) {
        const auto directory = root / language / "ispatarget" / "core";
        fs::create_directories(directory);
        std::ofstream manifest(directory / "ispa-stdlib.yaml");
        manifest << "module: core\nnamespace: ispa_core\n";
        // Existing C++ manifests have no language field.
        if (std::string_view(language) != "cpp") manifest << "language: " << language << '\n';
    }
    for (const auto* language : {"cpp", "python", "rust"}) {
        const auto library = InstalledLibraries::stdlib(root, "core", language);
        ASSERT_TRUE(library.found) << language;
        EXPECT_EQ(library.directory, root / language / "ispatarget" / "core");
        EXPECT_TRUE(InstalledLibraries::stdlib(root, library.directory.string(), language).found);
    }
    EXPECT_FALSE(InstalledLibraries::stdlib(root, "core", "unknown").found);
    EXPECT_FALSE(InstalledLibraries::stdlib(root,
        (root / "cpp" / "ispatarget" / "core").string(), "rust").found);
}
