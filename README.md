# rq

**rq** is a fast file search tool for Windows that doesn't suck. Built in C17 because life's too short for slow searches.

## Why rq?

- **Actually fast** - Multi-threaded search that uses all your CPU cores
- **Smart filtering** - Find files by size, date, type, whatever you need
- **Real glob support** - Because `*` should just work
- **No bloat** - Single executable, no dependencies, no BS

## Quick Examples

```bash
# Find all those C files you've been looking for
rq C:\Dev "*.c" --glob

# Hunt down massive files eating your disk space
rq C:\ "" --size +100M

# Find recent work documents
rq . report --ext pdf,docx --after 2025-01-01

# Search with some actual feedback
rq D:\ "backup" --stats --threads 12
```

---

## Usage

```
rq <where to look> <what to find> [how to find it]
```

### Basic Search
- `rq C:\Users\me pictures` - Find anything with "pictures" in the name
- `rq . "*.jpg" --glob` - All JPG files in current directory and subdirs
- `rq D:\ config --case` - Case-sensitive search for "config"

### File Filters
```bash
--ext jpg,png,gif           # Only these file types
--type image                # Built-in type filters (image, video, audio, etc.)
--size +500K                # Bigger than 500KB
--size -1M                  # Smaller than 1MB
--after 2025-01-01          # Modified after this date
--before 2024-12-31         # Modified before this date
--max-depth 2               # Don't go too deep into folders
```

### Search Options
```bash
--glob                      # Enable wildcards (* ? [] {})
--regex                     # Use regex patterns (for the brave)
--include-hidden            # Don't ignore hidden files
--follow-symlinks           # Follow symbolic links
--no-skip                   # Search everywhere (including .git, node_modules, etc.)
```

### Output & Performance
```bash
--json                      # Machine-readable output
--preview                   # Show file previews for text files
--stats                     # Real-time search statistics
--threads 8                 # Use 8 worker threads
--timeout 30000             # Give up after 30 seconds
--max-results 1000          # Stop after finding 1000 files
```

---

## Building This Thing

### The Easy Way (GCC/MinGW)
```bash
# One-liner that just works
gcc -std=c17 -O3 src/*.c src/regex/*.c -lshlwapi -lkernel32 -o rq.exe

# Or use the Makefile like a civilized person
make
```

### The Microsoft Way (MSVC)
```cmd
:: Basic build (works on any MSVC version)
nmake msvc

:: Fancy build with C11 atomics (VS 2022 17.5+ only)
nmake msvc-c11

:: When things go wrong
nmake msvc-debug
```

**MSVC Heads Up:** Microsoft's C11 atomics support is... complicated. Older versions don't have it, newer versions hide it behind experimental flags. The code handles this automatically, but if you want the fancy atomic operations, you need VS 2022 17.5 or later with `/experimental:c11atomics`.

### 32-bit Build (Why Though?)
```bash
i686-w64-mingw32-gcc -std=c17 -O3 src/*.c src/regex/*.c -lshlwapi -lkernel32 -o rq.exe
```

---

## Contributing

Found a bug? Have an idea? PRs welcome! This is a small project, so don't expect enterprise-level process.

## License

MIT License - do whatever you want with this code.

---