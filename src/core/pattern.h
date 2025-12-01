#ifndef PATTERN_H
#define PATTERN_H

#include <stdbool.h>

bool pattern_match_glob(const char *text, const char *pattern, bool case_sensitive);

bool pattern_match_char_class(const char *text_char, const char *class_pattern, bool case_sensitive);

bool pattern_match_brace_set(const char *text, const char *brace_pattern, bool case_sensitive);

bool pattern_matches(const char *text, const char *pattern, bool case_sensitive, bool use_glob, bool use_regex);

typedef struct pattern_compiled pattern_compiled_t;
pattern_compiled_t* pattern_compile(const char *pattern, bool case_sensitive, bool use_glob, bool use_regex);
bool pattern_match_compiled(const char *text, const pattern_compiled_t *compiled);
void pattern_free_compiled(pattern_compiled_t *compiled);

extern const char g_ascii_tolower[256];

#endif