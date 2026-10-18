# bulkren

Rename lots of files at once, safely. Preview first, apply when happy, undo if not.
Written in modern C++17 with **no dependencies**.

## Build

With plain g++ (works on Linux, macOS, and Windows MSYS2):

```bash
g++ -std=c++17 -O2 -Iinclude src/main.cpp src/renamer.cpp -o bulkren
```

Or with CMake:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage

By default `bulkren` only **previews** changes. Add `--apply` to rename.

```bash
bulkren --lower --no-spaces ~/Downloads             # preview tidy-up
bulkren --seq "holiday_{n}" --ext jpg --apply pics/ # holiday_001.jpg, holiday_002.jpg ...
bulkren --regex "IMG_(\d+)" "photo-$1" --apply .     # IMG_0042.jpg -> photo-0042.jpg
bulkren --replace " (1)" "" --apply .                # remove " (1)" from names
bulkren --prefix "2026_" --recursive --apply .
bulkren --undo --apply .                             # revert the last --apply here
```

Example preview:

```
  Holiday Photo 1.JPG  ->  holiday_photo_1.jpg
  Holiday Photo 2.JPG  ->  holiday_photo_2.jpg

2 file(s) would be renamed. Dry run - add --apply to do it.
```

### Options

| Option | What it does |
| --- | --- |
| `--replace A B` | Replace text A with B (repeatable) |
| `--regex PAT REP` | Regex replace (ECMAScript, `$1` groups) |
| `--seq TEMPLATE` | Rename to a template; `{n}` is the counter |
| `--pad N`, `--start N` | Counter width (default 3) and first number (default 1) |
| `--prefix`, `--suffix` | Add text at the start / before the extension |
| `--no-spaces` | Spaces become underscores |
| `--lower`, `--upper` | Change case |
| `-r`, `--recursive` | Include sub-folders |
| `--ext EXT` | Only these extensions (repeatable) |
| `--apply` | Do it for real |
| `--undo` | Revert the last `--apply` in the directory |

## Safety

- Dry run by default.
- Refuses to run if a new name already exists, two files would get the same name, or a name is invalid.
- Two-phase renaming, so swaps like `a -> b, b -> a` work, with rollback if the first phase fails.
- Hidden files are skipped. Extensions are never changed by rename rules (only by `--lower` / `--upper`).
- Every `--apply` writes `.bulkren-undo.tsv` in the directory so it can be undone.

## Notes

`--lower` / `--upper` only affect ASCII letters. File names containing tabs or newlines are not supported by the undo log.

## License

MIT
