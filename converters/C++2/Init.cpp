module Cpp.Init;
import Cpp.CoreFunctions;
import std;
extern "C" void init(const char* name) {
    Core::output = &Core::h_file;
    Core::name = name;
    Core::cpp_file.writeln("#include \"{}.h\"", Core::name);
}
extern "C" void close() {
}
extern "C" auto getSource() -> std::optional<std::string>* {
    auto str = Core::cpp_file.get();
    if (str.empty()) return nullptr;
    return new std::optional<std::string>(str);
}