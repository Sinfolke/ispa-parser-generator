module Semantic;
import corelib;
import logging;
import cpuf.op;
import std;

auto Semantic::checkDependency(
    const stdu::vector<std::string> &dep,
    const stdu::vector<std::string> &check_dep,
    std::set<stdu::vector<std::string>> &visited
) -> bool {
    // If we have already evaluated this dependency branch, stop to prevent infinite loops!
    if (visited.contains(dep)) {
        return false;
    }
    visited.insert(dep);

    const auto &use = tree.getUsePlacesTable();
    if (!use.contains(dep)) {
        return false;
    }

    const auto &dep_list = use.at(dep); // Use const& to prevent copying the vector
    if (std::find(dep_list.begin(), dep_list.end(), check_dep) != dep_list.end()) {
        return true;
    }

    for (const auto &new_dep : dep_list) {
        // Pass the visited set down along the chain
        if (checkDependency(new_dep, check_dep, visited)) {
            return true;
        }
    }

    return false;
}

auto Semantic::checkTokenRecursion() -> void {
    for (const auto &[name, dependencies] : tree.getUsePlacesTable()) {
        if (corelib::text::isLower(name.back()))
            continue;
        for (const auto &dep : dependencies) {
            // Create a fresh visited tracker for each unique root dependency check
            std::set<stdu::vector<std::string>> visited;

            if (checkDependency(dep, name, visited)) {
                throw Error("Circular token dependency detected involving: {} -> {}", name, dep);
            }
        }
    }
}