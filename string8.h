#ifndef _STRING8_H_
#define _STRING8_H_

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_IMPLEMENTATION
#include "arena.h"

typedef uint64_t u64;
typedef uint8_t u8;
typedef double f64;
typedef u8 b8;

typedef struct {
  u8 *str;
  u64 size;
} string8;

#define STR8_LIT(s) ((string8){.str = (u8 *)(s), .size = strlen((s))})
#define STR8_FMT "%.*s"
#define STR8_UNWRAP(s) (int)(s).size, (char *)(s).str

b8 str_equal(string8 s1, string8 s2);

string8 str_trim_left(string8 s);
string8 str_trim_right(string8 s);
string8 str_trim(string8 s);
b8 str_ends_with(string8 s, string8 suffix);
string8 str_remove_suffix(string8 s, string8 suffix);
b8 str_starts_with(string8 s, string8 prefix);
string8 str_remove_prefix(string8 s, string8 prefix);
// Return index of first match if found, haystack.size otherwise
u64 str_contains(string8 haystack, string8 needle);

string8 str_to_lowercase(mem_arena *arena, string8 s);
string8 str_to_uppercase(mem_arena *arena, string8 s);

void str_read_file(mem_arena *arena, string8 *dst, const char *fname);
string8 str_dup(mem_arena *arena, string8 src);
b8 str_split_once(string8 splitted[2], string8 input, string8 delim);

void str_join(mem_arena *arena, string8 *dst, const string8 glue, u64 n_elem,
              const string8 *elems);

b8 str_parse_unsigned(u64 *v, string8 s);
f64 str_parse_float(const string8 s);

#ifdef STRING_IMPLEMENTATION

string8 str_trim_left(string8 s) {
  u64 i = 0;
  while (i < s.size && isspace(s.str[i])) {
    i++;
  }
  return (string8){.str = s.str + i, .size = s.size - i};
}

string8 str_trim_right(string8 s) {
  u64 i = s.size;
  while (i > 0 && isspace(s.str[i - 1])) {
    i--;
  }
  return (string8){.str = s.str, .size = i};
}

string8 str_trim(string8 s) { return str_trim_right(str_trim_left(s)); }

b8 str_equal(string8 s1, string8 s2) {
  if (s1.size != s2.size)
    return 0;
  for (u64 i = 0; i < s1.size; ++i) {
    if (s1.str[i] != s2.str[i])
      return 0;
  }
  return 1;
}

b8 str_ends_with(string8 s, string8 suffix) {
  if (suffix.size > s.size)
    return false;
  u64 indx = s.size - suffix.size;
  string8 tail = (string8){.str = s.str + indx, .size = suffix.size};
  return str_equal(tail, suffix);
}

string8 str_remove_suffix(string8 s, string8 suffix) {
  if (!str_ends_with(s, suffix)) {
    return s;
  }
  return (string8){.str = s.str, .size = s.size - suffix.size};
}

b8 str_starts_with(string8 s, string8 prefix) {
  if (prefix.size > s.size)
    return false;
  string8 head = (string8){.str = s.str, .size = prefix.size};
  return str_equal(head, prefix);
}

string8 str_remove_prefix(string8 s, string8 prefix) {
  if (!str_starts_with(s, prefix))
    return s;
  return (string8){.str = s.str + prefix.size, .size = (s.size - prefix.size)};
}

void str_read_file(mem_arena *arena, string8 *dst, const char *fname) {
  memset(dst, 0, sizeof(string8));
  FILE *fp = fopen(fname, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s\n", fname);
    exit(1);
  }
  fseek(fp, 0, SEEK_END);
  u64 size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  dst->str = (u8 *)(arena == NULL ? malloc(size) : PUSH_ARRAY(arena, u8, size));
  dst->size = size;
  u64 n_byte_read = fread(dst->str, 1, size, fp);
  if (n_byte_read != size) {
    fprintf(stderr, "Could not read %lu bytes from %s\n", size, fname);
    fclose(fp);
    exit(1);
  }
  fclose(fp);
}

u64 str_contains(string8 haystack, string8 needle) {
  if (haystack.size < needle.size)
    return haystack.size;
  for (u64 i = 0; i < haystack.size - needle.size; ++i) {
    string8 test = (string8){.str = haystack.str + i, .size = needle.size};
    if (str_equal(test, needle))
      return i;
  }
  return haystack.size;
}

b8 str_split_once(string8 splitted[2], string8 input, string8 delim) {
  if (delim.size == 0)
    return false;
  u64 indx = str_contains(input, delim);
  if (indx > input.size - delim.size)
    return false;
  splitted[0] = (string8){.str = input.str, .size = indx};
  u64 iright = indx + delim.size;
  splitted[1] =
      (string8){.str = input.str + iright, .size = input.size - iright};
  return true;
}

string8 str_dup(mem_arena *arena, string8 src) {
  u8 *str = (u8 *)(arena == NULL ? malloc(src.size)
                                 : PUSH_ARRAY(arena, u8, src.size));
  memcpy(str, src.str, src.size);
  return (string8){.str = str, .size = src.size};
}

string8 str_to_lowercase(mem_arena *arena, string8 s) {
  string8 lower = str_dup(arena, s);
  for (u64 i = 0; i < lower.size; ++i) {
    lower.str[i] = tolower(lower.str[i]);
  }
  return lower;
}

string8 str_to_uppercase(mem_arena *arena, string8 s) {
  string8 upper = str_dup(arena, s);
  for (u64 i = 0; i < upper.size; ++i) {
    upper.str[i] = toupper(upper.str[i]);
  }
  return upper;
}

void str_join(mem_arena *arena, string8 *dst, const string8 glue, u64 n_elem,
              const string8 *elems) {
  memset(dst, 0, sizeof(string8));
  if (n_elem == 0)
    return;
  u64 size = (n_elem - 1) * glue.size;
  for (u64 i = 0; i < n_elem; ++i) {
    size += elems[i].size;
  }
  dst->str = (u8 *)(arena == NULL ? malloc(size) : PUSH_ARRAY(arena, u8, size));
  dst->size = size;
  u64 iptr = 0;
  for (u64 i = 0; i < n_elem; ++i) {
    if (i > 0) {
      memcpy(dst->str + iptr, glue.str, glue.size);
      iptr += glue.size;
    }
    memcpy(dst->str + iptr, elems[i].str, elems[i].size);
    iptr += elems[i].size;
  }
}

b8 str_parse_unsigned(u64 *v, const string8 s) {
  *v = 0;
  for (u64 i = 0; i < s.size; ++i) {
    if (s.str[i] < '0' || s.str[i] > '9')
      return false;
    *v *= 10;
    *v += (u64)(s.str[i] - '0');
  }
  return true;
}

f64 str_parse_float(const string8 s) {
  assert(s.size < 128);
  char input[128];
  memcpy(input, s.str, s.size);
  input[s.size] = '\0';
  return atof(input);
}

#endif
#endif
