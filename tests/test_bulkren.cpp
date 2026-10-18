#include <fstream>
#include <iostream>
#include <random>

#include "bulkren/renamer.hpp"

using namespace bulkren;

static int failures = 0;
#define CHECK(cond)                                                                 \
    do {                                                                            \
        if (!(cond)) {                                                              \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; \
            ++failures;                                                             \
        }                                                                           \
    } while (0)

static void touch(const fs::path& p) { std::ofstream(p) << "x"; }

static fs::path temp_dir() {
    std::random_device rd;
    fs::path d = fs::temp_directory_path() / ("bulkren_test_" + std::to_string(rd()));
    fs::create_directories(d);
    return d;
}

int main() {
    {  // basic transforms keep the extension
        Options o; o.lower = true; o.no_spaces = true;
        CHECK(transform_name("Hello World.TXT", o, 1) == "hello_world.txt");
    }
    {
        Options o; o.replaces = {{"old", "new"}};
        CHECK(transform_name("old_old.md", o, 1) == "new_new.md");
    }
    {
        Options o; o.regexes = {{"IMG_(\\d+)", "photo-$1"}};
        CHECK(transform_name("IMG_0042.jpg", o, 1) == "photo-0042.jpg");
    }
    {
        Options o; o.prefix = "a_"; o.suffix = "_z";
        CHECK(transform_name("file.tar.gz", o, 1) == "a_file.tar_z.gz");
        CHECK(transform_name(".bashrc", o, 1) == "a_.bashrc_z");
    }
    {
        Options o; o.seq_template = "img_{n}"; o.pad = 2; o.start = 5;
        CHECK(transform_name("whatever.jpg", o, 7) == "img_07.jpg");
    }
    {
        Options o; o.regexes = {{"(", "x"}};
        bool threw = false;
        try { transform_name("a.txt", o, 1); } catch (const std::runtime_error&) { threw = true; }
        CHECK(threw);
    }

    {  // end to end: sequence + reverse
        fs::path d = temp_dir();
        touch(d / "x.jpg"); touch(d / "y.jpg"); touch(d / "z.png"); touch(d / ".hidden.jpg");
        Options o; o.seq_template = "img_{n}"; o.pad = 2;
        auto files = collect_files(d, false, {"jpg"});
        CHECK(files.size() == 2);  // hidden + png excluded
        auto plan = build_plan(files, o);
        CHECK(check_plan(plan).empty());
        apply_plan(plan);
        CHECK(fs::exists(d / "img_01.jpg") && fs::exists(d / "img_02.jpg"));
        CHECK(!fs::exists(d / "x.jpg"));

        std::vector<Plan> reverse;
        for (auto& p : plan) reverse.push_back({p.to, p.from});
        apply_plan(reverse);
        CHECK(fs::exists(d / "x.jpg") && fs::exists(d / "y.jpg"));
        fs::remove_all(d);
    }
    {  // swap works thanks to two-phase rename
        fs::path d = temp_dir();
        touch(d / "b.txt");
        std::ofstream(d / "a.txt") << "AAA";
        std::vector<Plan> plan = {{d / "a.txt", d / "b.txt"}, {d / "b.txt", d / "a.txt"}};
        CHECK(check_plan(plan).empty());
        apply_plan(plan);
        std::string content;
        std::ifstream(d / "b.txt") >> content;
        CHECK(content == "AAA");
        fs::remove_all(d);
    }
    {  // collisions are refused
        fs::path d = temp_dir();
        touch(d / "a.txt"); touch(d / "b.txt"); touch(d / "keep.txt");
        CHECK(!check_plan({{d / "a.txt", d / "keep.txt"}}).empty());                           // exists
        CHECK(!check_plan({{d / "a.txt", d / "c.txt"}, {d / "b.txt", d / "c.txt"}}).empty());  // dup targets
        fs::remove_all(d);
    }

    if (failures == 0) std::cout << "All tests passed\n";
    return failures == 0 ? 0 : 1;
}
