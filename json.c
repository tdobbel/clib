#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define STRING_IMPLEMENTATION
#include "string8.h"
#define VECTOR_IMPLEMENTATION
#include "vector.h"
#define HASHMAP_IMPLEMENTATION
#include "hash_map.h"

typedef enum _json_value_type ValueType;

enum _json_value_type { Object, Array, Float, Int, String, Bool, Null };

typedef struct {
  ValueType type;
  void *value;
} JsonValue;

string8 _str_true = STR8_LIT("true");
string8 _str_false = STR8_LIT("false");
string8 _str_null = STR8_LIT("null");

JsonValue *json_parse(string8 s);
void json_free(JsonValue *js);
u64 _json_parse_object(JsonValue *js, string8 s);
u64 _json_parse_array(JsonValue *js, string8 s);
u64 _json_parse_string(JsonValue *js, string8 s);
u64 _json_parse_number(JsonValue *js, string8 s);
u64 _json_parse_bool(JsonValue *js, string8 s);
u64 _json_parse_null(JsonValue *js, string8 s);

int main(void) {
  string8 s = STR8_LIT("{\"a\": 10, \"b\": -1.5e-3, \"x\": [\"a\", 1, null], "
                       "\"c\": \"coucou\", \"d\": null, \"e\": true}");
  // string8 s = STR8_LIT("{\"a\": null}");
  JsonValue *js = json_parse(s);
  hash_map *hm = (hash_map *)js->value;
  kv_iterator kvi = hm_iterator(hm);
  while (get_next(&kvi)) {
    string8 key = *(string8 *)kvi.key_ptr;
    JsonValue *val = *(JsonValue **)kvi.value_ptr;
    switch (val->type) {
    case Int:
      i64 vint = *(i64 *)val->value;
      printf(STR8_FMT " => %ld\n", STR8_UNWRAP(key), vint);
      break;
    case Float:
      f64 vfloat = *(f64 *)val->value;
      printf(STR8_FMT " => %f\n", STR8_UNWRAP(key), vfloat);
      break;
    case String:
      string8 str8 = *(string8 *)val->value;
      printf(STR8_FMT " => " STR8_FMT "\n", STR8_UNWRAP(key),
             STR8_UNWRAP(str8));
      break;
    case Bool:
      b8 flag = *(b8 *)val->value;
      printf(STR8_FMT " => %hu\n", STR8_UNWRAP(key), flag);
      break;
    case Null:
      printf(STR8_FMT " => null\n", STR8_UNWRAP(key));
      break;
    case Array:
      vector *vec = (vector *)val->value;
      printf(STR8_FMT " => array of size %lu and capacity %lu\n", STR8_UNWRAP(key), vec->size, vec->capacity);
      break;
    case Object:
      break;
    }
  }
  json_free(js);

  return 0;
}

JsonValue *json_parse(string8 s) {
  string8 s2 = str_trim(s);
  JsonValue *js = (JsonValue *)malloc(sizeof(JsonValue));
  if (s2.str[0] == '{') {
    _json_parse_object(js, s2);
  } else if (s2.str[0] == '[') {
    _json_parse_array(js, s2);
  } else {
    fprintf(stderr, "Invalid json string\n");
  }
  return js;
}

void json_free(JsonValue *js) {
  switch (js->type) {
  case Bool:
  case Int:
  case Float:
    free(js->value);
    break;
  case String:
    string8 *s = (string8 *)js->value;
    free(s->str);
    free(js->value);
    break;
  case Object:
    hash_map *hm = (hash_map *)js->value;
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
    vector *vec = (vector *)js->value;
    JsonValue **entries = (JsonValue **)vec->data;
    for (u64 i = 0; i < vec->size; ++i) {
      json_free(entries[i]);
    }
    vector_free(vec);
  }
  free(js);
}

static b8 is_valid(u8 c) {
  b8 isnum = c >= '0' && c <= '9';
  return c == 'e' || c == 'E' || c == '+' || c == '-' || isnum || c == '.';
}

u64 _json_parse_number(JsonValue *js, string8 s) {
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

u64 _json_parse_string(JsonValue *js, string8 s) {
  assert(s.str[0] == '"');
  u64 n = 1;
  while (n < s.size && s.str[n] != '"') {
    n++;
  }
  if (n == s.size || s.str[n] != '"') {
    fprintf(stderr, "Could not parse string\n");
    exit(1);
  }
  string8 dup = str_dup(NULL, (string8){.str = s.str + 1, s.size = n - 1});
  js->type = String;
  js->value = malloc(sizeof(string8));
  *(string8 *)js->value = dup;
  return n + 1;
};

u64 _json_parse_bool(JsonValue *js, string8 s) {
  if (str_starts_with(s, _str_true)) {
    js->type = Bool;
    js->value = malloc(1);
    *(u8 *)js->value = 1;
    return 4;
  }
  if (str_starts_with(s, _str_false)) {
    js->type = Bool;
    js->value = malloc(1);
    *(u8 *)js->value = 0;
    return 5;
  }
  fprintf(stderr, "Could not parse entry (expected boolean)\n");
  exit(1);
}

u64 _json_parse_null(JsonValue *js, string8 s) {
  if (str_starts_with(s, _str_null)) {
    js->type = Null;
    js->value = NULL;
    return 4;
  }
  fprintf(stderr, "Could not parse entry (expected null)\n");
  exit(1);
}

u64 _json_parse_array(JsonValue *js, string8 s) {
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
      n_parsed = _json_parse_object(value, rhs);
      break;
    case '[':
      n_parsed = _json_parse_array(value, rhs);
      break;
    case '"':
      n_parsed = _json_parse_string(value, rhs);
      break;
    case 't':
    case 'f':
      n_parsed = _json_parse_bool(value, rhs);
      break;
    case 'n':
      n_parsed = _json_parse_null(value, rhs);
      break;
    case '-':
    case '0' ... '9':
      n_parsed = _json_parse_number(value, rhs);
      break;
    default:
      fprintf(stderr, "Could not parse json entry\n");
      exit(1);
    }
    VEC_PUSH(vec, JsonValue *, value);
    printf("Add entry to array\n");
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
  printf("%lu entries added\n", vec->size);
  return n;
}

u64 _json_parse_object(JsonValue *js, string8 s) {
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
      n_parsed = _json_parse_object(value, rhs);
      break;
    case '[':
      n_parsed = _json_parse_array(value, rhs);
      break;
    case '"':
      n_parsed = _json_parse_string(value, rhs);
      break;
    case 't':
    case 'f':
      n_parsed = _json_parse_bool(value, rhs);
      break;
    case 'n':
      n_parsed = _json_parse_null(value, rhs);
      break;
    case '-':
    case '0' ... '9':
      n_parsed = _json_parse_number(value, rhs);
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
