#ifndef _STRING8_H_
#define _STRING8_H_

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef uint8_t u8;
typedef u8 b8;

typedef struct {
  u8 *str;
  u64 size;
} string8;

#define STR8_LIT(s) ((string8){.str = (u8 *)(s), .size = strlen((s))})
#define STR8_FMT "%.*s"
#define STR8_UNWRAP(s) (int)(s).size, (char *)(s).str

string8 str_trim_left(string8 s);
string8 str_trim_right(string8 s);
string8 str_trim(string8 s);
void str_read_file(const char *fname, string8 *dst);
b8 str_equal(string8 s1, string8 s2);

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

void str_read_file(const char *fname, string8 *dst) {
  memset(dst, 0, sizeof(string8));
  FILE *fp = fopen(fname, "rb");
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s\n", fname);
    return;
  }
  fseek(fp, 0, SEEK_END);
  u64 size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  dst->str = (u8 *)malloc(size);
  dst->size = size;
  u64 n_byte_read = fread(dst->str, 1, size, fp);
  if (n_byte_read != size) {
    fprintf(stderr, "Could not read %lu bytes from %s\n", size, fname);
  }
  fclose(fp);
}

#endif
#endif
