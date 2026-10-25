module Modules;
import std;

namespace Modules {
ExternalPackage readExternal(const std::string& prefix) {
    const auto path = std::filesystem::absolute(prefix + ".ispa-export");
    std::ifstream in(path);
    std::string magic;
    unsigned version = 0;
    if (!(in >> magic >> version) || magic != "ISPA_EXTERN" || version != 1)
        throw std::runtime_error("Missing or incompatible external parser manifest: " + path.string());
    ExternalPackage result;
    auto parsePath = [&](const std::string& value) {
        Path out;
        std::istringstream parts(value);
        for (std::string part; std::getline(parts, part, '.');) {
            if (part.empty() || !(std::isalpha(static_cast<unsigned char>(part.front())) || part.front() == '_') ||
                !std::ranges::all_of(part, [](unsigned char c) { return std::isalnum(c) || c == '_'; }))
                throw std::runtime_error("Invalid symbol path in " + path.string());
            out.push_back(std::move(part));
        }
        return out;
    };
    std::string record, header;
    while (in >> record) {
        std::string module, name;
        if (record == "header") {
            in >> std::quoted(header);
            if (header.size() >= 2 && header.front() == '<' && header.back() == '>')
                header = header.substr(1, header.size() - 2);
            const auto extension = std::filesystem::path(header).extension().string();
            if (extension != ".h" && extension != ".py")
                throw std::runtime_error("Invalid external parser artifact in " + path.string());
            // The dependency prefix selects the public artifact. Its directory
            // is supplied by the consumer, never embedded in generated code.
            const auto artifact = std::filesystem::absolute(prefix + extension);
            if (!std::filesystem::is_regular_file(artifact))
                throw std::runtime_error("External parser artifact does not exist: " + artifact.string());
            header = "<" + artifact.filename().string() + ">";
        } else if (record == "module") {
            in >> std::quoted(module) >> std::quoted(name);
            auto& entry = result[parsePath(module)].entry;
            if (!name.empty()) entry = parsePath(name);
        } else if (record == "symbol") {
            ExternalSymbol symbol;
            in >> std::quoted(module) >> std::quoted(name) >> std::quoted(symbol.cpp_type) >> std::quoted(symbol.cpp_binding);
            symbol.header = header;
            if (header.empty() || symbol.cpp_type.empty() || symbol.cpp_binding.empty() ||
                !result[parsePath(module)].symbols.emplace(parsePath(name), std::move(symbol)).second)
                throw std::runtime_error("Invalid or duplicate external symbol in " + path.string());
        } else throw std::runtime_error("Unknown external manifest record: " + record);
        if (!in) throw std::runtime_error("Truncated external parser manifest: " + path.string());
    }
    for (const auto& [name, module] : result)
        if (module.entry && !module.symbols.contains(*module.entry))
            throw std::runtime_error("External module entry is not exported: " + spelling(name));
    return result;
}
}
