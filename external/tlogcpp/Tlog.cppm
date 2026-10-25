
export module Tlog.Logger;
import Tlog.oss;
import cpuf.op;
import std;
import std.compat;

export namespace Tlog {
    struct Branch_data {
        std::filesystem::path path;
        Oss oss;
        bool isExcluded = false;
    };
    class Logger {
        std::filesystem::path base_path;
        std::unordered_map<std::string, Branch_data> branches;
        std::vector<Branch_data> branches_stack;
        std::string file;
        std::size_t line;
#ifdef DEBUG
        bool isExcludeAll = false;
#else
        bool isExcludeAll = true;
#endif
        std::vector<std::string> exclude_branches;
        std::vector<std::string> exclude_paths;
        std::vector<std::string> include_branches;
        std::vector<std::string> include_paths;
        std::string getFormattedPath(std::string path) {
            std::string final_path;
            for (auto it = path.begin(); it != path.end(); ++it) {
                if (*it == '$' && std::next(it) != path.end() && *(std::next(it)) == '{') {
                    // skip $ and {
                    it += 2;
                    std::string branch_name;
                    while (it != path.end() && *it != '}') {
                        branch_name += *it;
                        ++it;
                    }
                    if (it == path.end()) {
                        throw std::runtime_error("Unclosed ${ in path");
                    }
                    if (branch_name.empty()) {
                        throw std::runtime_error("Empty branch name in ${}");
                    }
                    if (branches.find(branch_name) == branches.end()) {
                        throw std::runtime_error("Referenced branch '" + branch_name + "' does not exist");
                    }
                    // Append root_directory as string
                    auto &data = branches.at(branch_name);
                    final_path += data.path.root_directory().string();
                    // After '}', continue normal processing, it will ++ in for loop
                } else {
                    final_path += *it;
                }
            }
            final_path.insert(0, base_path.string() + "/");
            return final_path;
        }
        std::string get_relative_source_path(const std::string full_path) {
            namespace fs = std::filesystem;

            static const fs::path source_root = __SOURCE_ROOT__;

            fs::path full = fs::path(full_path).lexically_normal();

            // If full path starts with source_root, remove the prefix
            if (std::mismatch(source_root.begin(), source_root.end(), full.begin()).first == source_root.end()) {
                return fs::relative(full, source_root).string();
            } else {
                return full.string(); // fallback: not under source root
            }
        }
        std::vector<bool> exclude_stack;
        void exclude_if_found(const std::filesystem::path &path) {
            if (!exclude_paths.empty()) {
                if (std::find(exclude_paths.begin(), exclude_paths.end(), path.string()) != exclude_paths.end()) {
                    exclude_stack.push_back(true);
                }
            }
        }
        void exclude_if_found(const std::string &branch) {
            if (!exclude_branches.empty()) {
                if (std::find(exclude_branches.begin(), exclude_branches.end(), branch) != exclude_paths.end()) {
                    branches.at(branch).isExcluded = true;
                }
            }
        }
    public:
        explicit Logger(const std::filesystem::path path) : base_path(path) {}
        Logger() : base_path("./") {}

        void createBranch(const std::string& name, const std::string& path, bool append = true) {
            if (branches.count(name) != 0)
                throw std::runtime_error("Branch '" + name + "' already exists");
            exclude_if_found(name);
            auto final_path = getFormattedPath(name);
            branches.insert_or_assign(name, Branch_data{final_path, Oss(final_path, append)});
        }
        void pushBranch(std::string path, bool append = true) {
            auto final_path = getFormattedPath(path);
            branches_stack.emplace_back(final_path, Oss(final_path, append));
        }
        void popBranch() {
            if (branches_stack.empty())
                throw std::runtime_error("Cannot pop branch, stack is empty");
            branches_stack.pop_back();
        }
        void closeBranch(std::string b) {
            branches.at(b).oss.close();
        }
        template<typename Format, typename... Args>
        void log(Format str, Args&&... args) {
            if (!enabled()) return;
            //branches_stack.back().oss.vprintf("[{}, {}] ", file, line);
            branches_stack.back().oss.print(str, std::forward<Args>(args)...);
        }
        template<typename Format, typename ...Args>
        void dlog(Format str, Args ...args) {
            if (!enabled()) return;
            //branches_stack.back().oss.vprintf("[{}, {}] ", file, line);
            branches_stack.back().oss.dprint(str, std::forward<Args>(args)...);
        }
        template<typename Format, typename ...Args>
        void logToBranch(std::string name, Format str, Args ...args) {
            branches[name].oss.print(str, args...);
        }
        template<typename Format, typename ...Args>
        void dlogToBranch(std::string name, Format str, Args ...args) {
            branches[name].oss.dprint(str, args...);
        }
        void increaseIndentLevel() {
            if (!enabled()) return;
            branches_stack.back().oss.increaseIndentLevel();
        }
        void increaseIndentLevel(std::string branch) {
            branches.at(branch).oss.increaseIndentLevel();
        }
        void decreaseIndentLevel() {
            if (!enabled()) return;
            branches_stack.back().oss.decreaseIndentLevel();
        }
        void decreaseIndentLevel(std::string branch) {
            branches.at(branch).oss.decreaseIndentLevel();
        }
        void clearLogDirectory() {
            std::filesystem::remove_all(base_path);
        }
        void record_location(std::source_location loc = std::source_location::current()) {
            file = get_relative_source_path(loc.file_name());
            line = loc.line();
        }
        void set_location(std::string file, std::size_t line) {
            this->file = get_relative_source_path(file);
            this->line = line;
        }
        bool enabled() {
            return !isExcludeAll || (isExcludeAll && !include_branches.empty() && !include_paths.empty());
        }
        void excludeBranch(std::string branch) {
            exclude_branches.push_back(branch);
        }
        void exclude(std::string path) {
            exclude_paths.push_back(path);
        }
        void includeBranch(std::string branch) {
            include_branches.push_back(branch);
        }
        void include(std::string path) {
            include_paths.push_back(path);
        }
        void excludeAll() {
            isExcludeAll = true;
        }
        void includeAll() {
            isExcludeAll = false;
        }
        void clearExcludes() {
            exclude_branches.clear();
            exclude_paths.clear();
        }
    };
    class Branch {
        Logger *logger = nullptr;
    public:
        Branch(Logger &logger, std::string path, bool append = true, std::source_location loc = std::source_location::current()) {
            if (!logger.enabled()) return;
            this->logger = &logger;
            logger.set_location(loc.file_name(), loc.line());
            this->logger->pushBranch(path, append);
        }
        Branch() = default;
        ~Branch() {
            if (logger == nullptr)
                return;
            logger->popBranch();
        }
        void open(Logger &logger, std::string path, bool append = true, std::source_location loc = std::source_location::current()) {
            if (!logger.enabled()) return;
            this->logger = &logger;
            this->logger->pushBranch(path, append);
        }
        void close() {
            if (logger == nullptr)
                return;
            logger->popBranch();
        }
        template<typename Format, typename... Args>
        void log(Format str, Args&&... args) {
            if (logger == nullptr)
                return;
            //branches_stack.back().oss.vprintf("[{}, {}] ", file, line);
            logger->log(str, std::forward<Args>(args)...);
        }
        template<typename Format, typename ...Args>
        void dlog(Format str, Args ...args) {
            if (logger == nullptr)
                return;
            //branches_stack.back().oss.vprintf("[{}, {}] ", file, line);
            logger->dlog(str, std::forward<Args>(args)...);
        }
    };
};
