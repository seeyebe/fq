# fq

`fq` is a small Windows search tool. It walks directories with a thread pool, matches files and folders by name (glob or regex), and filters by extension, type, size, date, depth, and hidden/symlink rules. Single executable, no dependencies.

## Usage
```
fq <root> <pattern> [options]
```
- `<pattern>` can be empty (`""`) to match everything.
- Use `--glob` for glob patterns, `--regex` for regex. Without either, substring match is used.
- `--folders` or `--folders-only` include directories in results. Default results are files only.

## Examples
```bash
# All C sources under a tree
fq C:\Dev "*.c" --glob

# Files over 100MB anywhere on C:
fq C:\ "" --size +100M

# Recent PDFs or DOCX
fq . report --ext pdf,docx --after 2025-01-01

# Watch thread stats while searching
fq D:\ "backup" --stats --threads 12

# Find folders named "build"
fq . build --folders
```

## Common options
- Matching: `--glob`, `--regex`, `--case`
- Directories: `--folders`, `--folders-only`, `--files-only`, `--max-depth <n>`
- Filters: `--ext <list>`, `--type <text|image|video|audio|archive>`, `--min/--max/--size <size>`, `--after/--before <YYYY-MM-DD>`
- Traversal: `--include-hidden`, `--follow-symlinks`, `--no-skip` (don’t skip common dirs)
- Output: `--json`, `--preview [n]`, `--out <file>`
- Performance: `--threads <n>`, `--timeout <ms>`, `--max-results <n>`, `--stats`

## Build
```bash
# GCC / MinGW
gcc -std=c11 -O3 src/*.c src/regex/*.c -lshlwapi -lkernel32 -o fq.exe
make          # uses the Makefile (defaults to -std=c11)
```
```cmd
:: MSVC
nmake msvc        :: default
nmake msvc-c11    :: /experimental:c11atomics (VS 2022 17.5+)
nmake msvc-debug  :: debug build
```

If your GCC is older and rejects `-std=c11`, try `-std=gnu11` or update the toolchain.

## License
MIT
