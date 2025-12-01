ifeq ($(OS),Windows_NT)
  ifdef MSYSTEM
    SHELL = /bin/sh
  else
    SHELL = cmd.exe
  endif
else
  SHELL = /bin/sh
endif
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -g
SRCDIR = src
SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/core/search.c $(SRCDIR)/core/criteria.c $(SRCDIR)/core/pattern.c \
          $(SRCDIR)/output/output.c $(SRCDIR)/output/preview.c \
          $(SRCDIR)/platform/platform.c $(SRCDIR)/platform/thread_pool.c \
          $(SRCDIR)/cli/cli.c $(SRCDIR)/cli/version.c \
          $(SRCDIR)/util/utils.c \
          $(SRCDIR)/regex/re.c $(SRCDIR)/regex/regex.c
TARGET = fq.exe
BUILDDIR = build
OUTFILE = $(BUILDDIR)/$(TARGET)
LIBS = -lshlwapi -lkernel32 -lshell32

# Debug build flags
DEBUG_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O0 -g -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
DEBUG_LDFLAGS = -fsanitize=address,undefined

.PHONY: all clean install test debug analyze msvc msvc-c11 msvc-debug

all: $(OUTFILE)

# MSVC build targets
# Note: For VS 2022 17.5+ with experimental C11 atomics, use:
# nmake msvc-c11 (requires /experimental:c11atomics)
msvc: CC = cl
msvc: CFLAGS = /W4 /O2 /MT /D_CRT_SECURE_NO_WARNINGS
msvc: LIBS = shlwapi.lib kernel32.lib
msvc: TARGET = fq_msvc.exe
msvc: OUTFILE = $(BUILDDIR)/$(TARGET)
msvc: $(OUTFILE)

msvc-c11: CC = cl
msvc-c11: CFLAGS = /std:c11 /experimental:c11atomics /W4 /O2 /MT /D_CRT_SECURE_NO_WARNINGS
msvc-c11: LIBS = shlwapi.lib kernel32.lib
msvc-c11: TARGET = fq_msvc_c11.exe
msvc-c11: OUTFILE = $(BUILDDIR)/$(TARGET)
msvc-c11: $(OUTFILE)

msvc-debug: CC = cl
msvc-debug: CFLAGS = /W4 /Od /Zi /MT /D_CRT_SECURE_NO_WARNINGS /DDEBUG
msvc-debug: LIBS = shlwapi.lib kernel32.lib
msvc-debug: TARGET = fq_msvc_debug.exe
msvc-debug: OUTFILE = $(BUILDDIR)/$(TARGET)
msvc-debug: $(OUTFILE)

ifeq ($(SHELL),cmd.exe)
  MKDIR_P = if not exist "$(BUILDDIR)" mkdir "$(BUILDDIR)"
  RMDIR = if exist "$(BUILDDIR)" rmdir /s /q "$(BUILDDIR)"
else
  MKDIR_P = mkdir -p "$(BUILDDIR)"
  RMDIR = rm -rf "$(BUILDDIR)"
endif

$(OUTFILE): $(SOURCES)
	@$(MKDIR_P)
ifeq ($(CC),cl)
	$(CC) $(CFLAGS) /I$(SRCDIR) $(SOURCES) /Fe:$(OUTFILE) /link $(LIBS)
else
	$(CC) $(CFLAGS) -I$(SRCDIR) $(SOURCES) $(LIBS) -o $(OUTFILE)
endif

clean:
	@$(RMDIR)

install: $(OUTFILE)
	copy "$(OUTFILE)" "C:\Windows\System32\"
