#include "pattern.h"
#include "regex/regex.h"
#include "compat.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <stdio.h>

const char g_ascii_tolower[256] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
    64,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,
    96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

bool pattern_match_char_class(const char *text_char, const char *class_pattern, bool case_sensitive) {
    if (!text_char || !class_pattern || *class_pattern != '[') return false;

    char test_char = case_sensitive ? *text_char : g_ascii_tolower[(unsigned char)*text_char];
    bool negate = false;
    bool match = false;

    const char *p = class_pattern + 1;

    if (*p == '!' || *p == '^') {
        negate = true;
        p++;
    }

    while (*p && *p != ']') {
        if (p[1] == '-' && p[2] != ']' && p[2] != '\0') {
            char start = case_sensitive ? *p : g_ascii_tolower[(unsigned char)*p];
            char end = case_sensitive ? p[2] : g_ascii_tolower[(unsigned char)p[2]];
            if (test_char >= start && test_char <= end) {
                match = true;
                break;
            }
            p += 3;
        } else {
            char class_char = case_sensitive ? *p : g_ascii_tolower[(unsigned char)*p];
            if (test_char == class_char) {
                match = true;
                break;
            }
            p++;
        }
    }

    return negate ? !match : match;
}

bool pattern_match_brace_set(const char *text, const char *brace_pattern, bool case_sensitive) {
    if (!text || !brace_pattern || *brace_pattern != '{') return false;

    const char *p = brace_pattern + 1;
    const char *option_start = p;

    while (*p && *p != '}') {
        if (*p == ',' || p[1] == '}') {
            size_t option_len = p - option_start;
            if (*p == '}') option_len++;

            char *option = malloc(option_len + 1);
            if (option) {
                memcpy(option, option_start, option_len);
                option[option_len] = '\0';

                bool matches;
                if (case_sensitive) {
                    matches = (strcmp(text, option) == 0);
                } else {
                    matches = (_stricmp(text, option) == 0);
                }

                free(option);
                if (matches) return true;
            }

            if (*p == ',') {
                p++;
                option_start = p;
            } else {
                break;
            }
        } else {
            p++;
        }
    }

    return false;
}

bool pattern_match_glob(const char *text, const char *pattern, bool case_sensitive) {
    if (!text || !pattern) return false;

    const char *t = text;
    const char *p = pattern;
    const char *star_pattern = NULL;
    const char *star_text = NULL;

    while (*t) {
        if (*p == '\\' && p[1] != '\0') {
            p++;
            char pc = case_sensitive ? *p : g_ascii_tolower[(unsigned char)*p];
            char tc = case_sensitive ? *t : g_ascii_tolower[(unsigned char)*t];
            if (tc != pc) {
                if (star_pattern) {
                    p = star_pattern;
                    t = ++star_text;
                    continue;
                }
                return false;
            }
        } else if (*p == '*') {
            star_pattern = ++p;
            star_text = t;
            continue;
        } else if (*p == '?') {
            // Single character wildcard
            // Do nothing, just advance both pointers
        } else if (*p == '[') {
            const char *class_end = strchr(p, ']');
            if (class_end && !pattern_match_char_class(t, p, case_sensitive)) {
                if (star_pattern) {
                    p = star_pattern;
                    t = ++star_text;
                    continue;
                }
                return false;
            }
            if (class_end) {
                p = class_end;
            }
        } else {
            char pc = case_sensitive ? *p : g_ascii_tolower[(unsigned char)*p];
            char tc = case_sensitive ? *t : g_ascii_tolower[(unsigned char)*t];
            if (tc != pc) {
                if (star_pattern) {
                    p = star_pattern;
                    t = ++star_text;
                    continue;
                }
                return false;
            }
        }

        t++;
        p++;
    }

    while (*p == '*') p++;

    return *p == '\0';
}

bool pattern_matches(const char *text, const char *pattern, bool case_sensitive, bool use_glob, bool use_regex) {
    if (!text || !pattern) return false;

    if (pattern[0] == '\0' || (pattern[0] == '*' && pattern[1] == '\0')) {
        return true;
    }

    if (use_regex) {
        if (case_sensitive) {
            return regex_test(pattern, text);
        } else {
            char *lower_text = _strdup(text);
            char *lower_pattern = _strdup(pattern);
            if (!lower_text || !lower_pattern) {
                free(lower_text);
                free(lower_pattern);
                return false;
            }

            for (int i = 0; lower_text[i]; i++) {
                lower_text[i] = (char)g_ascii_tolower[(unsigned char)lower_text[i]];
            }
            for (int i = 0; lower_pattern[i]; i++) {
                lower_pattern[i] = (char)g_ascii_tolower[(unsigned char)lower_pattern[i]];
            }

            bool result = regex_test(lower_pattern, lower_text);
            free(lower_text);
            free(lower_pattern);
            return result;
        }
    }

    if (use_glob) {
        if (strchr(pattern, '{') && strchr(pattern, '}')) {
            const char *brace_start = strchr(pattern, '{');
            const char *brace_end = strchr(brace_start, '}');

            if (brace_end) {
                size_t prefix_len = brace_start - pattern;
                size_t suffix_len = strlen(brace_end + 1);
                size_t brace_len = brace_end - brace_start + 1;

                char *prefix = NULL, *suffix = NULL, *brace_set = NULL;
                bool result = false;

                if (prefix_len > 0) {
                    prefix = malloc(prefix_len + 1);
                    if (prefix) {
                        memcpy(prefix, pattern, prefix_len);
                        prefix[prefix_len] = '\0';
                    }
                }

                if (suffix_len > 0) {
                    suffix = malloc(suffix_len + 1);
                    if (suffix) {
                        #ifdef _MSC_VER
                            strcpy_s(suffix, suffix_len + 1, brace_end + 1);
                        #else
                            strcpy(suffix, brace_end + 1);
                        #endif
                    }
                }

                brace_set = malloc(brace_len + 1);
                if (brace_set) {
                    memcpy(brace_set, brace_start, brace_len);
                    brace_set[brace_len] = '\0';

                    bool prefix_ok = !prefix || (strncmp(text, prefix, prefix_len) == 0);
                    bool suffix_ok = !suffix || (strlen(text) >= suffix_len &&
                                                strcmp(text + strlen(text) - suffix_len, suffix) == 0);

                    if (prefix_ok && suffix_ok) {
                        size_t middle_start = prefix_len;
                        size_t text_len = strlen(text);
                        if (text_len >= prefix_len + suffix_len) {
                            size_t middle_len = text_len - prefix_len - suffix_len;
                            if (middle_len > 0) {
                                char *middle = malloc(middle_len + 1);
                                if (middle) {
                                    memcpy(middle, text + middle_start, middle_len);
                                    middle[middle_len] = '\0';
                                    result = pattern_match_brace_set(middle, brace_set, case_sensitive);
                                    free(middle);
                                }
                            } else {
                                char empty_str[1] = {'\0'};
                                result = pattern_match_brace_set(empty_str, brace_set, case_sensitive);
                            }
                        }
                    }
                }

                free(prefix);
                free(suffix);
                free(brace_set);
                return result;
            }
        }

        return pattern_match_glob(text, pattern, case_sensitive);
    } else {
        if (case_sensitive) {
            return strstr(text, pattern) != NULL;
        } else {
            char *text_lower = malloc(strlen(text) + 1);
            char *pattern_lower = malloc(strlen(pattern) + 1);

            if (!text_lower || !pattern_lower) {
                free(text_lower);
                free(pattern_lower);
                return false;
            }

            #ifdef _MSC_VER
                strcpy_s(text_lower, strlen(text) + 1, text);
                strcpy_s(pattern_lower, strlen(pattern) + 1, pattern);
            #else
                strcpy(text_lower, text);
                strcpy(pattern_lower, pattern);
            #endif
            CharLowerBuffA(text_lower, (DWORD)strlen(text_lower));
            CharLowerBuffA(pattern_lower, (DWORD)strlen(pattern_lower));
            bool result = strstr(text_lower, pattern_lower) != NULL;
            free(text_lower);
            free(pattern_lower);
            return result;
        }
    }
}

struct pattern_compiled {
    char *pattern;
    bool case_sensitive;
    bool use_glob;
    bool use_regex;
    re_t compiled_regex;
};

pattern_compiled_t* pattern_compile(const char *pattern, bool case_sensitive, bool use_glob, bool use_regex) {
    if (!pattern) return NULL;

    pattern_compiled_t *compiled = malloc(sizeof(pattern_compiled_t));
    if (!compiled) return NULL;

    compiled->pattern = _strdup(pattern);
    if (!compiled->pattern) {
        free(compiled);
        return NULL;
    }

    compiled->case_sensitive = case_sensitive;
    compiled->use_glob = use_glob;
    compiled->use_regex = use_regex;
    compiled->compiled_regex = NULL;

    if (use_regex) {
        if (case_sensitive) {
            compiled->compiled_regex = regex_compile(pattern);
        } else {
            char *lower_pattern = _strdup(pattern);
            if (lower_pattern) {
                for (int i = 0; lower_pattern[i]; i++) {
                    lower_pattern[i] = (char)g_ascii_tolower[(unsigned char)lower_pattern[i]];
                }
                compiled->compiled_regex = regex_compile(lower_pattern);
                free(lower_pattern);
            }
        }
    }

    return compiled;
}

bool pattern_match_compiled(const char *text, const pattern_compiled_t *compiled) {
    if (!compiled) return false;

    if (compiled->use_regex && compiled->compiled_regex) {
        if (compiled->case_sensitive) {
            return regex_match(compiled->compiled_regex, text);
        } else {
            char *lower_text = _strdup(text);
            if (!lower_text) return false;

            for (int i = 0; lower_text[i]; i++) {
                lower_text[i] = (char)g_ascii_tolower[(unsigned char)lower_text[i]];
            }

            bool result = regex_match(compiled->compiled_regex, lower_text);
            free(lower_text);
            return result;
        }
    }

    return pattern_matches(text, compiled->pattern, compiled->case_sensitive, compiled->use_glob, compiled->use_regex);
}

void pattern_free_compiled(pattern_compiled_t *compiled) {
    if (!compiled) return;
    if (compiled->compiled_regex) {
        regex_free(compiled->compiled_regex);
    }
    free(compiled->pattern);
    free(compiled);
}
