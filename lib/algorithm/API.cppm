export module Algorithm.API;
import AST.Tree;
import AST.types;
import LexerBuilder;
import LangAPI;
import LangRepr.Holder;
import std;

export namespace Algorithm {
    inline constexpr std::uint32_t interface_version = 2;

    // Binary contents belong to the algorithm. Removing a key removes its file.
    struct Metadata {
        std::unordered_map<std::string, std::string> cache;
    };
    struct BuildRequest {
        AST::Tree& tree;
        const AST::TreeMap& changed_rules;
        bool auto_conflicts = true;
        std::size_t conflict_lookahead = 3;
        std::function<void(std::size_t, std::size_t)> progress;
    };
    struct ConversionRequest {
        LexerBuilder& lexer;
        AST::Tree& tree;
        LangAPI::Language language;
        const std::string& namespace_name;
    };
    struct Result {
        std::shared_ptr<void> output;
        LangRepr::Holder (*to_lang_repr)(void*, ConversionRequest&) = nullptr;
    };
    // Loaded libraries and the host must use the same C++ toolchain and API.
    // The version checks the protocol; it does not provide a portable C ABI.
    struct Descriptor {
        std::uint32_t version = interface_version;
        const char* id;
        std::uint32_t cache_version;
        Result (*build)(const BuildRequest&, Metadata&);
    };
    using EntryFn = const Descriptor*();
}
