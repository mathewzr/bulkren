#include "bulkren/renamer.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <stdexcept>
#include <system_error>

namespace bulkren {

namespace {

std::string zero_pad(int n, int width) {
    std::string s = std::to_string(n);
    if (static_cast<int>(s.size()) < width) s.insert(0, width - s.size(), '0');
    return s;
}

void replace_all(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

std::string lower_ascii(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string upper_ascii(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

}  // namespace

std::string transform_name(const std::string& filename, const Options& o, int index) {
    // Split "name.tar.gz" -> stem "name.tar", ext ".gz". Dotfiles have no extension.
    std::string stem = filename, ext;
    auto dot = filename.find_last_of('.');
    if (dot != std::string::npos && dot > 0) {
        stem = filename.substr(0, dot);
        ext = filename.substr(dot);
    }

    if (!o.seq_template.empty()) {
        std::string t = o.seq_template;
        if (t.find("{n}") == std::string::npos) t += "_{n}";
        stem = t;
        replace_all(stem, "{n}", zero_pad(index, o.pad));
    }
    for (const auto& [a, b] : o.replaces) replace_all(stem, a, b);
    for (const auto& [pat, rep] : o.regexes) {
        try {
            stem = std::regex_replace(stem, std::regex(pat), rep);
        } catch (const std::regex_error& e) {
            throw std::runtime_error("invalid regex '" + pat + "': " + e.what());
        }
    }
    if (o.no_spaces) std::replace(stem.begin(), stem.end(), ' ', '_');
    stem = o.prefix + stem + o.suffix;

    std::string out = stem + ext;
    if (o.lower) out = lower_ascii(out);
    if (o.upper) out = upper_ascii(out);
    return out;
}

std::vector<fs::path> collect_files(const fs::path& dir, bool recursive,
                                    const std::vector<std::string>& exts) {
    std::vector<fs::path> files;
    auto consider = [&](const fs::directory_entry& e) {
        if (!e.is_regular_file()) return;
        const std::string name = e.path().filename().string();
        if (!name.empty() && name[0] == '.') return;
        if (!exts.empty()) {
            std::string ext = lower_ascii(e.path().extension().string());
            if (!ext.empty()) ext.erase(0, 1);
            if (std::find(exts.begin(), exts.end(), ext) == exts.end()) return;
        }
        files.push_back(e.path());
    };
    if (recursive) {
        for (const auto& e : fs::recursive_directory_iterator(dir)) consider(e);
    } else {
        for (const auto& e : fs::directory_iterator(dir)) consider(e);
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::vector<Plan> build_plan(const std::vector<fs::path>& files, const Options& opts) {
    std::vector<Plan> plan;
    int index = opts.start;
    for (const auto& f : files) {
        std::string name = f.filename().string();
        std::string renamed = transform_name(name, opts, index++);
        if (renamed != name) plan.push_back({f, f.parent_path() / renamed});
    }
    return plan;
}

std::string check_plan(const std::vector<Plan>& plan) {
    std::set<fs::path> sources, targets;
    for (const auto& p : plan) sources.insert(p.from);

    for (const auto& p : plan) {
        const std::string name = p.to.filename().string();
        if (name.empty() || name == "." || name == "..")
            return "invalid new name for " + p.from.string();
        if (name.find_first_of("/\\") != std::string::npos)
            return "new name contains a path separator: " + name;
        if (!targets.insert(p.to).second)
            return "two files would both be renamed to " + p.to.string();
        std::error_code ec;
        if (fs::exists(p.to, ec) && !sources.count(p.to) && !fs::equivalent(p.from, p.to, ec))
            return p.to.string() + " already exists";
    }
    return "";
}

void apply_plan(const std::vector<Plan>& plan) {
    std::vector<fs::path> temps;
    temps.reserve(plan.size());
    try {
        for (std::size_t i = 0; i < plan.size(); ++i) {
            fs::path tmp = plan[i].from.parent_path() / (".bulkren_tmp_" + std::to_string(i));
            fs::rename(plan[i].from, tmp);
            temps.push_back(tmp);
        }
    } catch (...) {
        for (std::size_t i = 0; i < temps.size(); ++i) {
            std::error_code ec;
            fs::rename(temps[i], plan[i].from, ec);
        }
        throw;
    }
    for (std::size_t i = 0; i < plan.size(); ++i) fs::rename(temps[i], plan[i].to);
}

}  // namespace bulkren
