module;
module args;
import logging;
import cpuf.printf;
import cpuf.op;
import CLI;
import std;

Args parse_args(int argc, char** argv) {
    Args args;
    CLI::App app{"ISPA Parser Generator"};
    std::string algorithm;
    app.allow_extras();
    app.add_flag("-v,--version",  args.version, "Print version");
    app.add_option("-d,--dir", args.dir, "Recursively scan a module source directory")->expected(1, -1)->allow_extra_args(false);
    app.add_option("-o", args.output, "Directory where to output generated files");
    app.add_option("--dep", args.dependencies, "Use an external parser: NAME OUTPUT_PREFIX")->type_size(2)->expected(1)->take_all()->allow_extra_args(false);
    app.add_flag("--no-dep", args.no_dep, "Generate external dependencies as separate parsers in the output directory");
    app.add_flag("--check-only", args.check_only, "Run parsing and semantic checks without generating output");
    app.add_option("--ai-recovery", args.ai_recovery_file, "Load AI recovery messages from this JSON file");
    app.add_flag("--no-ai-recovery", args.no_ai_recovery, "Do not load AI recovery messages");
    app.add_option("-l,--lang", args.language_str, "Set build language")->required();
    app.add_option("-a,--algorithm", algorithm, "Set one of algorithms: LL, LR(0), LR(1), LALR, LR(*)");
    app.add_option("--debug", args.debug, "Output debug logs into Logs directory");
    app.add_flag("--debug-instrumentation,!--no-debug-instrumentation", args.debug_instrumentation,
        "Emit ispadbg source checkpoints and typed parser state");
    app.add_flag("--logs", args.logs, "Output logs into Logs directory");
    app.add_flag("--nfa-debug,!--no-nfa-debug", args.nfa_debug,
                 "Track TNFA provenance and trace capture boundaries through TNFA/TDFA generation");
    app.add_option("--dump", args.dump, "Dump certain parts of program")->expected(0, -1);
    app.add_option("--ddall", args.dump_all, "Dump everything from program")->expected(0);
    app.add_option("--dd", args.dump_dir, "Directory where to dump");
    app.add_option("--dump-nfa-from-rule", args.dump_nfa_from_rule, "Dump nfa from rule")->expected(0);
    app.add_flag("--incremental,!--no-incremental", args.incremental, "Output a file-per-production.");
    app.add_flag("--auto-conflicts,!--no-auto-conflicts", args.auto_conflicts,
        "Resolve LR conflicts with extra lookahead and deferred runtime branching");
    app.add_option("--lr-conflict-lookahead", args.conflict_lookahead,
        "Maximum automatic compile-time LR lookahead before runtime fallback (1..8)");
    try {
        app.parse(argc, argv);
    } catch (CLI::CallForAllHelp&) {
        std::cout << app.help("") << std::endl;
        std::exit(0);
    } catch (CLI::CallForHelp &help) {
        std::cout << app.help(help.get_name()) << std::endl;
        std::exit(0);
    } catch (CLI::ArgumentMismatch &missmatch) {
        std::cerr << "Improperly passed argument " << missmatch.what() << std::endl;
        std::exit(1);
    } catch (CLI::RequiredError &required) {
        if (args.version)
            return args;
        std::cerr << "Argument " << required.what() << std::endl;
        std::exit(1);
    } catch (CLI::ParseError& e) {
        std::cerr << "Parsing error: " << e.what() << std::endl;
        std::exit(1);
    }

    if (args.conflict_lookahead < 1 || args.conflict_lookahead > 8)
        throw Error("--lr-conflict-lookahead must be between 1 and 8");
    if (args.no_ai_recovery && !args.ai_recovery_file.empty())
        throw Error("--ai-recovery and --no-ai-recovery cannot be used together");

    // Map algorithm string to enum
    if (!args.language_str.empty()) {
        if (args.language_str == "cpp") args.language = Args::Language::Cpp;
        else if (args.language_str == "rust") args.language = Args::Language::Rust;
        else if (args.language_str == "python" || args.language_str == "py") args.language = Args::Language::Python;
        else throw Error("Unknown language {}", args.language_str);
    }
    if (!algorithm.empty()) {
        if (algorithm == "LL") args.algorithm = Args::Algorithm::LL;
        else if (algorithm == "LR" || algorithm == "LR0") args.algorithm = Args::Algorithm::LR;
        else if (algorithm == "LALR" || algorithm == "LR1") args.algorithm = Args::Algorithm::LALR;
        else if (algorithm == "LR(*)" || algorithm == "LR*" || algorithm == "LR_STAR") args.algorithm = Args::Algorithm::LR_STAR;
        else throw Error("Unknown algorithm {}", algorithm);
    }
    args.files = app.remaining();
    return args;
}
auto to_string(Args::Algorithm algorithm) -> std::string  {
    switch (algorithm) {
        case Args::Algorithm::LL:
            return "LL";
        case Args::Algorithm::LR:
            return "LR";
        case Args::Algorithm::LALR:
            return "LALR";
        case Args::Algorithm::LR_STAR:
            return "LR_STAR";
        default:
            throw Error("Undefined algorithm {}: while converting to string representation", (int) algorithm);
    }
}
auto to_string(Args::Language language) -> std::string {
    switch (language) {
        case Args::Language::Cpp:
            return "cpp";
        case Args::Language::Rust:
            return "rust";
        case Args::Language::Python:
            return "python";
        default:
            throw Error("Undefined language {}: while converting to string representation", (int) language);
    }
}
