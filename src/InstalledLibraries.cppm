export module InstalledLibraries;
import std;

export namespace InstalledLibraries {
namespace fs = std::filesystem;

struct Discovery {
    std::vector<std::string> source_directories;
    std::map<std::string, std::string> dependencies;
};

// The installed executable lives at <install>/versions/<tag>/ispa[.exe].
// Build-tree executables instead use the platform's normal data directory.
fs::path root(const fs::path& executable) {
    if (const char* override_path = std::getenv("ISPA_PKG_ROOT"); override_path && *override_path)
        return fs::absolute(override_path);
    if (const char* install_path = std::getenv("ISPA_INSTALL_DIR"); install_path && *install_path)
        return fs::absolute(fs::path(install_path) / "libraries");
    std::error_code error;
    const auto binary = fs::weakly_canonical(executable, error);
    if (!error && binary.parent_path().parent_path().filename() == "versions")
        return binary.parent_path().parent_path().parent_path() / "libraries";
#ifdef _WIN32
    if (const char* local = std::getenv("LOCALAPPDATA"); local && *local)
        return fs::path(local) / "Programs" / "ISPA" / "libraries";
#else
    if (const char* data = std::getenv("XDG_DATA_HOME"); data && *data)
        return fs::path(data) / "ispa" / "libraries";
#endif
    if (const char* home = std::getenv("HOME"); home && *home)
        return fs::path(home) / ".local" / "share" / "ispa" / "libraries";
    return {};
}

Discovery discover(const fs::path& library_root) {
    Discovery result;
    std::error_code error;
    if (!fs::is_directory(library_root, error)) return result;
    std::vector<fs::path> packages;
    for (const auto& entry : fs::directory_iterator(library_root))
        if (entry.is_directory() && !entry.path().filename().string().starts_with('.'))
            packages.push_back(entry.path());
    std::sort(packages.begin(), packages.end());

    std::map<std::string, std::set<std::string>> candidates;
    for (const auto& package : packages) {
        for (const auto& kind : {"grammar", "python"}) {
            const auto directory = package / "ispalib" / kind;
            if (fs::is_directory(directory)) result.source_directories.push_back(directory.string());
        }
        std::vector<fs::path> manifests;
        for (const auto& item : fs::recursive_directory_iterator(package))
            if (item.is_regular_file() && item.path().extension() == ".ispa-export")
                manifests.push_back(item.path());
        std::sort(manifests.begin(), manifests.end());
        for (const auto& manifest : manifests) {
            auto prefix = manifest;
            prefix.replace_extension();
            if (!fs::is_regular_file(prefix.string() + ".h") &&
                !fs::is_regular_file(prefix.string() + ".py")) continue;
            candidates[manifest.stem().string()].insert(prefix.string());
        }
        if (manifests.size() == 1) {
            auto prefix = manifests.front();
            prefix.replace_extension();
            if (fs::is_regular_file(prefix.string() + ".h") ||
                fs::is_regular_file(prefix.string() + ".py"))
                candidates[package.filename().string()].insert(prefix.string());
        }
    }
    for (const auto& [name, prefixes] : candidates)
        if (prefixes.size() == 1) result.dependencies.emplace(name, *prefixes.begin());
    return result;
}

// A standard-library module (`--stdlib`) announces itself with ispa-stdlib.yaml in its directory:
//   module: core          the name `--stdlib core` selects
//   language: cpp         cpp, python, or rust (omitted means cpp for older manifests)
//   namespace: ISPA_CORE  namespace the generated parser links through (--stdlib-name overrides)
//   include: ispa_core    include directory the generated sources use (default: the lowercased namespace)
struct Stdlib {
    bool found = false;
    fs::path directory;
    std::string module;
    std::string language;
    std::string ns = "ISPA_CORE";
    std::string include;
};

Stdlib readStdlib(const fs::path& directory) {
    Stdlib result;
    std::ifstream file(directory / "ispa-stdlib.yaml");
    if (!file) return result;
    result.found = true;
    result.directory = directory;
    for (std::string line; std::getline(file, line);) {
        if (const auto hash = line.find('#'); hash != std::string::npos) line.erase(hash);
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const auto trim = [](std::string text) {
            const auto first = text.find_first_not_of(" \t\r");
            if (first == std::string::npos) return std::string{};
            return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
        };
        const auto key = trim(line.substr(0, colon));
        const auto value = trim(line.substr(colon + 1));
        if (key == "module") result.module = value;
        else if (key == "language") result.language = value;
        else if (key == "namespace" && !value.empty()) result.ns = value;
        else if (key == "include") result.include = value;
    }
    return result;
}

// `request` is a directory, or the name of an installed module.
Stdlib stdlib(const fs::path& library_root, const std::string& request,
              const std::string& language = "cpp") {
    // Older manifests describe C++ libraries. Never select one for a different target.
    const auto compatible = [&](const Stdlib& library) {
        return (library.language.empty() ? "cpp" : library.language) == language;
    };
    std::error_code error;
    if (fs::is_directory(request, error)) {
        auto library = readStdlib(fs::absolute(request));
        return compatible(library) ? library : Stdlib{};
    }
    if (!fs::is_directory(library_root, error)) return {};
    std::vector<fs::path> packages;
    for (const auto& entry : fs::directory_iterator(library_root))
        if (entry.is_directory() && !entry.path().filename().string().starts_with('.'))
            packages.push_back(entry.path());
    std::sort(packages.begin(), packages.end());
    for (const auto& package : packages) {
        const auto targets = package / "ispatarget";
        if (!fs::is_directory(targets, error)) continue;
        std::vector<fs::path> directories;
        for (const auto& entry : fs::directory_iterator(targets))
            if (entry.is_directory()) directories.push_back(entry.path());
        std::sort(directories.begin(), directories.end());
        for (const auto& directory : directories)
            if (auto found = readStdlib(directory); found.found && found.module == request && compatible(found)) return found;
    }
    return {};
}

std::string pluginPrefix(const fs::path& executable_directory, const std::string& kind,
                         const std::string& name) {
#ifdef _WIN32
    constexpr auto extension = ".dll";
#elif defined(__APPLE__)
    constexpr auto extension = ".dylib";
#else
    constexpr auto extension = ".so";
#endif
    for (const auto& prefix : {"libispa-", "ispa-"}) {
        const auto candidate = executable_directory / (std::string(prefix) + kind + "-" + name);
        if (fs::is_regular_file(candidate.string() + extension)) return candidate.string();
    }
    return "libispa-" + kind + "-" + name;
}
}
