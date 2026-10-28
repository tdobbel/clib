#ifndef _JSON_H_
#define _JSON_H_

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef STRING_IMPLEMENTATION
#define STRING_IMPLEMENTATION
#endif

#include "string8.h"

#ifndef VECTOR_IMPLEMENTATION
#define VECTOR_IMPLEMENTATION
#endif

#include "vector.h"

#ifndef HASHMAP_IMPLEMENTATION
#define HASHMAP_IMPLEMENTATION
#endif

#include "hash_map.h"

enum __json_value_type { Object, Array, Float, Int, String, Bool, Null };
typedef enum __json_value_type ValueType;

typedef struct {
  ValueType type;
  void *value;
} JsonValue;

static const char *__str_true = "true";
static const char *__str_false = "false";
static const char *__str_null = "null";

JsonValue *json_parse(string8 s);
void json_free(JsonValue *js);
void json_print(const JsonValue *js, const JsonValue *parent, u8 indent);
u64 __json_parse_object(JsonValue *js, string8 s);
u64 __json_parse_array(JsonValue *js, string8 s);
u64 __json_parse_string(JsonValue *js, string8 s);
u64 __json_parse_number(JsonValue *js, string8 s);
u64 __json_parse_bool(JsonValue *js, string8 s);
u64 __json_parse_null(JsonValue *js, string8 s);

#define JSON_PRINT(js) json_print(js, NULL, 0)

#ifdef JSON_IMPLEMENTATION

static u32 __parse_hex(string8 hex) {
  u32 out = 0;
  for (u64 i = 0; i < hex.size; ++i) {
    u8 byte = hex.str[i];
    if ('0' <= byte && byte <= '9') {
      byte -= '0';
    } else if (byte >= 'a' && byte <= 'f') {
      byte += 10 - 'a';
    } else if (byte >= 'A' && byte <= 'F') {
      byte += 10 - 'A';
    } else {
      fprintf(stderr, "Invalid hex: " STR8_FMT "\n", STR8_UNWRAP(hex));
      exit(1);
    }
    out = (out << 4) | byte;
  }
  return out;
}

static i32 __encode_utf(u32 code_point, u8 encoded[4]) {
  if (code_point <= 0x007f) {
    encoded[0] = (u8)code_point;
    return 1;
  }

  if (code_point <= 0x07ff) {
    encoded[0] = (u8)(0xc0 | (code_point >> 6));
    encoded[1] = (u8)(0x80 | (code_point & 0x3f));
    return 2;
  }

  if (code_point <= 0xffff) {
    if (0xd800 <= code_point && code_point <= 0xdbff)
      return 0; // surrogate pair
    encoded[0] = (u8)(0xE0 | (code_point >> 12));
    encoded[1] = (u8)(0x80 | ((code_point >> 6) & 0x3F));
    encoded[2] = (u8)(0x80 | (code_point & 0x3F));
    return 3;
  }

  if (code_point <= 0x10FFFF) {
    encoded[0] = (u8)(0xF0 | (code_point >> 18));
    encoded[1] = (u8)(0x80 | ((code_point >> 12) & 0x3F));
    encoded[2] = (u8)(0x80 | ((code_point >> 6) & 0x3F));
    encoded[3] = (u8)(0x80 | (code_point & 0x3F));
    return 4;
  }

  return -1;
}

JsonValue *json_parse(string8 s) {
  string8 s2 = str_trim(s);
  JsonValue *js = (JsonValue *)malloc(sizeof(JsonValue));
  u64 n_parsed = 0;
  if (s2.str[0] == '{') {
    n_parsed = __json_parse_object(js, s2);
  } else if (s2.str[0] == '[') {
    n_parsed = __json_parse_array(js, s2);
  } else {
    fprintf(stderr, "Invalid json string\n");
  }
  if (n_parsed < s2.size) {
    fprintf(stderr, "Could not parse the whole string");
    exit(1);
  }
  return js;
}

void json_free(JsonValue *js) {
  hash_map *hm = NULL;
  string8 *s = NULL;
  vector *vec = NULL;
  switch (js->type) {
  case Bool:
  case Int:
  case Float:
    free(js->value);
    break;
  case String:
    s = (string8 *)js->value;
    free(s->str);
    free(js->value);
    break;
  case Object:
    hm = (hash_map *)js->value;
    kv_iterator kvi = hm_iterator(hm);
    while (get_next(&kvi)) {
      string8 s = *(string8 *)kvi.key_ptr;
      free(s.str);
      JsonValue *val = *(JsonValue **)kvi.value_ptr;
      json_free(val);
    }
    hm_deinit(hm);
    break;
  case Null:
    break;
  case Array:
    vec = (vector *)js->value;
    JsonValue **entries = (JsonValue **)vec->data;
    for (u64 i = 0; i < vec->size; ++i) {
      json_free(entries[i]);
    }
    vector_free(vec);
  }
  free(js);
}

void json_print(const JsonValue *js, const JsonValue *parent, u8 indent) {
  hash_map *hm = NULL;
  string8 s = {0};
  vector *vec = NULL;
  b8 flag;
  i64 inum;
  f64 fnum;
  char prefix[256];
  for (u8 i = 0; i < indent; ++i) {
    prefix[i] = ' ';
  }
  b8 inside_object = parent != NULL && parent->type == Object;
  prefix[indent] = '\0';
  switch (js->type) {
  case Null:
    printf("%snull", inside_object ? "" : prefix);
    break;
  case Bool:
    flag = *(b8 *)js->value;
    printf("%s%s", inside_object ? "" : prefix, flag ? "true" : "false");
    break;
  case String:
    s = *(string8 *)js->value;
    printf("%s\"" STR8_FMT "\"", inside_object ? "" : prefix, STR8_UNWRAP(s));
    break;
  case Int:
    inum = *(i64 *)js->value;
    printf("%s%ld", inside_object ? "" : prefix, inum);
    break;
  case Float:
    fnum = *(f64 *)js->value;
    printf("%s%f", inside_object ? "" : prefix, fnum);
    break;
  case Array:
    vec = (vector *)js->value;
    if (vec->size == 0) {
      printf("%s[]", inside_object ? "" : prefix);
      break;
    }
    JsonValue **values = (JsonValue **)vec->data;
    printf("%s[\n", inside_object ? "" : prefix);
    for (u64 i = 0; i < vec->size; ++i) {
      json_print(values[i], js, indent + 2);
      if (i < vec->size - 1)
        printf(",");
      printf("\n");
    }
    printf("%s]", prefix);
    break;
  case Object:
    hm = (hash_map *)js->value;
    if (hm->size == 0) {
      printf("%s{}", inside_object ? "" : prefix);
      break;
    }
    kv_iterator kvi = hm_iterator(hm);
    printf("%s{\n", inside_object ? "" : prefix);
    u64 cntr = 0;
    while (get_next(&kvi)) {
      string8 key = *(string8 *)kvi.key_ptr;
      JsonValue *value = *(JsonValue **)kvi.value_ptr;
      printf("%s  \"" STR8_FMT "\": ", prefix, STR8_UNWRAP(key));
      json_print(value, js, indent + 2);
      cntr++;
      if (cntr < hm->size)
        printf(",");
      printf("\n");
    }
    printf("%s}", prefix);
    break;
  }
  if (parent == NULL)
    printf("\n");
}

static b8 is_valid(u8 c) {
  b8 isnum = c >= '0' && c <= '9';
  return c == 'e' || c == 'E' || c == '+' || c == '-' || isnum || c == '.';
}

u64 __json_parse_number(JsonValue *js, string8 s) {
  u64 n = 0;
  b8 isint = 1;
  while (n < s.size && is_valid(s.str[n])) {
    if (s.str[n] < '0' || s.str[n] > '9')
      isint = 0;
    n++;
  }
  if (isint) {
    i64 num = str_parse_signed((string8){.str = s.str, .size = n});
    js->type = Int;
    js->value = malloc(sizeof(i64));
    *(i64 *)js->value = num;
  } else {
    f64 num = str_parse_float((string8){.str = s.str, .size = n});
    js->type = Float;
    js->value = malloc(sizeof(f64));
    *(f64 *)js->value = num;
  }
  return n;
}

u64 __json_parse_string(JsonValue *js, string8 s) {
  assert(s.str[0] == '"');
  u64 n = 1;
  vector *content;
  content = VEC_CREATE(u8);
  while (n < s.size && s.str[n] != '"') {
    if (s.str[n] != '\\') {
      VEC_PUSH(content, u8, s.str[n]);
      n++;
      continue;
    }
    n++; // skip backslash
    switch (s.str[n]) {
    case '\\':
      VEC_PUSH(content, u8, '\\');
      n++;
      break;
    case '/':
      VEC_PUSH(content, u8, '/');
      n++;
      break;
    case '"':
      VEC_PUSH(content, u8, '"');
      n++;
      break;
    case 'b':
      VEC_PUSH(content, u8, '\b');
      n++;
      break;
    case 'f':
      VEC_PUSH(content, u8, '\f');
      n++;
      break;
    case 'n':
      VEC_PUSH(content, u8, '\n');
      n++;
      break;
    case 't':
      VEC_PUSH(content, u8, '\t');
      n++;
      break;
    case 'u':
      n++;
      string8 hex;
      hex = (string8){.str = s.str + n, .size = 4};
      u32 code_point = __parse_hex(hex);
      u8 encoded[4];
      i32 n_byte = __encode_utf(code_point, encoded);
      n += 4;
      if (n_byte == 0) { // surrogate
        assert(s.str[n] == '\\' && s.str[n + 1] == 'u');
        n += 2;
        u32 high = code_point;
        hex = (string8){.str = s.str + n, .size = 4};
        u32 low = __parse_hex(hex);
        if (low < 0xdc00 || low > 0xdfff) {
          fprintf(stderr, "Invalid surrogate pair\n");
          exit(1);
        }
        code_point = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
        n += 4;
        n_byte = __encode_utf(code_point, encoded);
      }
      if (n_byte <= 0) {
        fprintf(stderr, "Invalid code point\n");
        exit(1);
      }
      for (u8 ibyte = 0; ibyte < n_byte; ++ibyte) {
        VEC_PUSH(content, u8, encoded[ibyte]);
      }
      break;
    default:
      fprintf(stderr, "Invalid escape sequence \\%c\n", s.str[n]);
      exit(1);
    }
  }
  if (n == s.size || s.str[n] != '"') {
    fprintf(stderr, "Could not parse string\n");
    exit(1);
  }
  js->type = String;
  js->value = malloc(sizeof(string8));
  *(string8 *)js->value =
      (string8){.str = content->data, .size = content->size};
  free(content);
  return n + 1;
}

u64 __json_parse_bool(JsonValue *js, string8 s) {
  if (str_starts_with(s, STR8_LIT(__str_true))) {
    js->type = Bool;
    js->value = malloc(1);
    *(u8 *)js->value = 1;
    return 4;
  }
  if (str_starts_with(s, STR8_LIT(__str_false))) {
    js->type = Bool;
    js->value = malloc(1);
    *(u8 *)js->value = 0;
    return 5;
  }
  fprintf(stderr, "Could not parse entry (expected boolean)\n");
  exit(1);
}

u64 __json_parse_null(JsonValue *js, string8 s) {
  if (str_starts_with(s, STR8_LIT(__str_null))) {
    js->type = Null;
    js->value = NULL;
    return 4;
  }
  fprintf(stderr, "Could not parse entry (expected null)\n");
  exit(1);
}

u64 __json_parse_array(JsonValue *js, string8 s) {
  assert(s.str[0] == '[');
  u64 n = 1;
  vector *vec = VEC_CREATE(JsonValue *);
  while (n < s.size && s.str[n] != ']') {
    while (n < s.size && isspace(s.str[n]))
      n++;
    if (s.str[n] == ']')
      break;

    JsonValue *value = (JsonValue *)malloc(sizeof(JsonValue));
    u64 n_parsed;
    string8 rhs = (string8){.str = s.str + n, .size = s.size - n};
    switch (s.str[n]) {
    case '{':
      n_parsed = __json_parse_object(value, rhs);
      break;
    case '[':
      n_parsed = __json_parse_array(value, rhs);
      break;
    case '"':
      n_parsed = __json_parse_string(value, rhs);
      break;
    case 't':
    case 'f':
      n_parsed = __json_parse_bool(value, rhs);
      break;
    case 'n':
      n_parsed = __json_parse_null(value, rhs);
      break;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      n_parsed = __json_parse_number(value, rhs);
      break;
    default:
      fprintf(stderr, "Could not parse json entry\n");
      exit(1);
    }
    VEC_PUSH(vec, JsonValue *, value);
    n += n_parsed;
    while (n < s.size && isspace(s.str[n]))
      n++;
    if (n >= s.size)
      break;
    if (s.str[n] == ',') {
      n++;
    } else if (s.str[n] == ']') {
      ;
    } else {
      fprintf(stderr, "Could not parse json\n");
      exit(1);
    }
  }
  assert(n < s.size && s.str[n++] == ']');
  js->type = Array;
  js->value = vec;
  return n;
}

u64 __json_parse_object(JsonValue *js, string8 s) {
  assert(s.str[0] == '{');
  hash_map *hm = STRING_HASHMAP(JsonValue *);
  u64 n = 1;
  while (n < s.size && s.str[n] != '}') {
    while (n < s.size && isspace(s.str[n])) {
      n++;
    }
    if (s.str[n] == '}')
      break;
    assert(n < s.size && s.str[n++] == '"');
    u64 start = n;
    while (n < s.size && s.str[n] != '"') {
      n++;
    }
    assert(n < s.size && n > start);
    string8 key =
        str_dup(NULL, (string8){.str = s.str + start, .size = n - start});
    n++;
    while (s.str[n] < s.size && isspace(s.str[n])) {
      n++;
    }
    assert(n < s.size && s.str[n++] == ':');
    while (n < s.size && isspace(s.str[n])) {
      n++;
    }
    JsonValue *value = (JsonValue *)malloc(sizeof(JsonValue));
    string8 rhs = (string8){.str = s.str + n, .size = s.size - n};
    u64 n_parsed;
    switch (s.str[n]) {
    case '{':
      n_parsed = __json_parse_object(value, rhs);
      break;
    case '[':
      n_parsed = __json_parse_array(value, rhs);
      break;
    case '"':
      n_parsed = __json_parse_string(value, rhs);
      break;
    case 't':
    case 'f':
      n_parsed = __json_parse_bool(value, rhs);
      break;
    case 'n':
      n_parsed = __json_parse_null(value, rhs);
      break;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      n_parsed = __json_parse_number(value, rhs);
      break;
    default:
      fprintf(stderr, "Could not parse json entry\n");
      exit(1);
    }
    hm_put(hm, &key, &value);
    n += n_parsed;
    while (n < s.size && isspace(s.str[n])) {
      n++;
    }
    if (n >= s.size)
      break;
    if (s.str[n] == ',') {
      n++;
    } else if (s.str[n] == '}') {
      ;
    } else {
      fprintf(stderr, "Could not parse json\n");
      exit(1);
    }
  }
  assert(n < s.size && s.str[n++] == '}');
  js->type = Object;
  js->value = hm;
  return n;
}

#endif
#endif
