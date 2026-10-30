export module LangRepr.Converter;

import LangRepr.Holder;
import Converter.Construct;
import LangAPI;
import cpuf.dlib;
import std;

export namespace LangRepr {
    class Converter {
        dlib converter_lib;
        const Holder &holder;
        std::string namespace_name;
        bool incremental;
        bool debug_instrumentation;
        bool text_view;
        std::vector<std::pair<std::string, std::string>> files;
        std::string lang;
        ::Converter::ConstructBase *builder;
    public:
        Converter(const Holder &holder, const std::string &lang, const std::string &namespace_name,
                  bool incremental, bool debug_instrumentation = true,
                  const std::string& library_prefix = {}, bool text_view = false)
            : converter_lib(library_prefix.empty() ? "libispa-converter-" + lang : library_prefix),
              holder(holder), namespace_name(namespace_name), incremental(incremental),
              debug_instrumentation(debug_instrumentation), text_view(text_view), lang(lang) {}

        auto init() -> void {
            auto construct = converter_lib.loadfun<::Converter::ConstructBase*, const LangAPI::Declarations *, const char*, bool, bool, bool>(lang + "_construct");
            builder = construct(&holder.get(), namespace_name.c_str(), incremental, debug_instrumentation, text_view);
        }
        auto construct() -> void {
            builder->construct();
            files = builder->get_files();
            for (auto& [name, source] : files) {
                if (name.ends_with(".py")) {
                    std::ostringstream imports;
                    for (const auto& header : holder.getIncludes()) {
                        std::string file = header;
                        if (file.starts_with('<') && file.ends_with('>')) file = file.substr(1, file.size() - 2);
                        if ((!file.ends_with(".h") && !file.ends_with(".py")) || file.find('/') != std::string::npos) continue;
                        const auto module = file.substr(0, file.rfind('.'));
                        if (!module.empty()) imports << "import " << module << '\n';
                    }
                    constexpr std::string_view future_line = "from __future__ import annotations\n";
                    const auto future = source.find(future_line);
                    source.insert(future == std::string::npos ? 0 : future + future_line.size(), imports.str());
                    continue;
                }
                if (!name.ends_with(".h")) continue;
                std::ostringstream headers;
                for (const auto& header : holder.getIncludes()) {
                    headers << "#include ";
                    if (header.starts_with('<') && header.ends_with('>')) headers << header;
                    else headers << std::quoted(header);
                    headers << '\n';
                }
                source.insert(source.starts_with("#pragma once\n") ? 13 : 0, headers.str());
            }
        }
        [[nodiscard]] auto get() const -> const std::vector<std::pair<std::string, std::string>>& {
            return files;
        }
        ~Converter() {
            converter_lib.loadfun<void, ::Converter::ConstructBase*>(lang + "_construct_finish")(builder);
        }
    };
}
