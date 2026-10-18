#pragma once
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace bulkren {
namespace fs = std::filesystem;

struct Options {
    std::vector<std::pair<std::string, std::string>> replaces;  // literal find/replace
    std::vector<std::pair<std::string, std::string>> regexes;   // ECMAScript regex, $1 groups
    bool lower = false;
    bool upper = false;
    bool no_spaces = false;  // spaces -> underscores
    std::string prefix;
    std::string suffix;       // added before the extension
    std::string seq_template; // e.g. "holiday_{n}" replaces the whole name (extension kept)
    int pad = 3;
    int start = 1;
};

struct Plan {
    fs::path from;
    fs::path to;
};

// Compute the new file name for `filename`. `index` is the sequence number used by {n}.
// Throws std::runtime_error on an invalid regex.
std::string transform_name(const std::string& filename, const Options& opts, int index);

// Gather regular files in `dir` (hidden files skipped), sorted by path.
// `exts` (lowercase, no dot) limits which extensions are included; empty = all.
std::vector<fs::path> collect_files(const fs::path& dir, bool recursive,
                                    const std::vector<std::string>& exts);

// Build the list of renames that actually change something.
std::vector<Plan> build_plan(const std::vector<fs::path>& files, const Options& opts);

// Returns "" if the plan is safe, otherwise a human-readable problem.
std::string check_plan(const std::vector<Plan>& plan);

// Perform the renames in two phases so swaps/chains (a->b, b->a) work.
// Rolls back if the first phase fails. Throws on error.
void apply_plan(const std::vector<Plan>& plan);

}  // namespace bulkren
