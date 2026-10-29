#include <ispastdlib/recovery_scan.hpp>
#include <iostream>
#include <sstream>
#include <string>
static std::string unhex(const std::string& h) {
    std::string out;
    for (size_t i = 0; i + 1 < h.size(); i += 2) out += (char)std::stoi(h.substr(i, 2), nullptr, 16);
    return out;
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string hex, bhex; size_t budget, begin; int nb;
        in >> hex >> budget >> begin >> nb;
        std::vector<std::string> bs;
        for (int i = 0; i < nb; ++i) { in >> bhex; bs.push_back(unhex(bhex)); }
        std::string src = unhex(hex);
        auto r = ISPA_STD::Recovery::scanBalanced(src.c_str() + begin, bs, budget);
        std::cout << (r.end - src.c_str()) << " " << (r.exhausted ? 1 : 0) << "\n";
    }
}
