#include <fstream>
#include <iostream>
#include <stdexcept>

#include "bulkren/renamer.hpp"

using namespace bulkren;

static const char* kVersion = "0.1.0";
static const char* kUndoLog = ".bulkren-undo.tsv";

static const char* kUsage = R"HELP(bulkren - rename many files at once (dry run by default)

Usage: bulkren [options] [directory]

Rename rules (applied to the file name, extension is kept):
  --replace A B      replace text A with B (repeatable)
  --regex PAT REP    regex replace, ECMAScript syntax, $1 for groups (repeatable)
  --seq TEMPLATE     replace the name with a template, e.g. "trip_{n}"
  --pad N            digits for {n} (default 3)
  --start N          first number for {n} (default 1)
  --prefix TEXT      add text at the start
  --suffix TEXT      add text before the extension
  --no-spaces        replace spaces with underscores
  --lower | --upper  change case of the whole name

Selection:
  -r, --recursive    include sub-folders
  --ext EXT          only files with this extension (repeatable, e.g. --ext jpg)

Actions:
  --apply            actually rename (otherwise just preview)
  --undo             revert the last --apply in that directory
  -h, --help         show this help
  -V, --version      show version

Examples:
  bulkren --lower --no-spaces ~/Downloads
  bulkren --seq "holiday_{n}" --ext jpg --apply photos/
  bulkren --regex "IMG_(\d+)" "photo-$1" --apply .
)HELP";

static std::string rel(const fs::path& p, const fs::path& base) {
    return fs::relative(p, base).generic_string();
}

static void print_plan(const std::vector<Plan>& plan, const fs::path& base) {
    for (const auto& p : plan)
        std::cout << "  " << rel(p.from, base) << "  ->  " << rel(p.to, base) << "\n";
}

static int run(int argc, char** argv) {
    Options opts;
    fs::path dir = ".";
    bool recursive = false, apply = false, undo = false;
    std::vector<std::string> exts;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](int n) {
            if (i + n >= argc) throw std::runtime_error("missing value(s) for " + a);
        };
        if (a == "-h" || a == "--help") { std::cout << kUsage; return 0; }
        else if (a == "-V" || a == "--version") { std::cout << "bulkren " << kVersion << "\n"; return 0; }
        else if (a == "-r" || a == "--recursive") recursive = true;
        else if (a == "--apply") apply = true;
        else if (a == "--undo") undo = true;
        else if (a == "--lower") opts.lower = true;
        else if (a == "--upper") opts.upper = true;
        else if (a == "--no-spaces") opts.no_spaces = true;
        else if (a == "--replace") { need(2); opts.replaces.emplace_back(argv[i + 1], argv[i + 2]); i += 2; }
        else if (a == "--regex") { need(2); opts.regexes.emplace_back(argv[i + 1], argv[i + 2]); i += 2; }
        else if (a == "--prefix") { need(1); opts.prefix = argv[++i]; }
        else if (a == "--suffix") { need(1); opts.suffix = argv[++i]; }
        else if (a == "--seq") { need(1); opts.seq_template = argv[++i]; }
        else if (a == "--pad") { need(1); opts.pad = std::stoi(argv[++i]); }
        else if (a == "--start") { need(1); opts.start = std::stoi(argv[++i]); }
        else if (a == "--ext") {
            need(1);
            std::string e = argv[++i];
            if (!e.empty() && e[0] == '.') e.erase(0, 1);
            for (auto& c : e) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            exts.push_back(e);
        }
        else if (!a.empty() && a[0] == '-') throw std::runtime_error("unknown option " + a + " (try --help)");
        else dir = a;
    }

    if (opts.lower && opts.upper) throw std::runtime_error("use only one of --lower / --upper");
    if (!fs::is_directory(dir)) throw std::runtime_error(dir.string() + " is not a directory");
    const fs::path log = dir / kUndoLog;

    std::vector<Plan> plan;
    if (undo) {
        std::ifstream in(log);
        if (!in) throw std::runtime_error("no undo log found in " + dir.string());
        std::string line;
        while (std::getline(in, line)) {
            auto tab = line.find('\t');
            if (tab == std::string::npos) continue;
            // log line: <new name> TAB <original name>; undo renames new -> original
            plan.push_back({dir / line.substr(0, tab), dir / line.substr(tab + 1)});
        }
    } else {
        plan = build_plan(collect_files(dir, recursive, exts), opts);
    }

    if (plan.empty()) { std::cout << "Nothing to rename.\n"; return 0; }

    print_plan(plan, dir);
    if (std::string problem = check_plan(plan); !problem.empty()) {
        std::cerr << "error: " << problem << "\nNothing was renamed.\n";
        return 2;
    }

    if (!apply) {
        std::cout << "\n" << plan.size()
                  << " file(s) would be renamed. Dry run - add --apply to do it.\n";
        return 0;
    }

    apply_plan(plan);
    if (undo) {
        fs::remove(log);
    } else {
        std::ofstream out(log, std::ios::trunc);
        for (const auto& p : plan) out << rel(p.to, dir) << '\t' << rel(p.from, dir) << '\n';
    }
    std::cout << "\nRenamed " << plan.size() << " file(s)."
              << (undo ? "" : " Undo with: bulkren --undo --apply") << "\n";
    return 0;
}

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
