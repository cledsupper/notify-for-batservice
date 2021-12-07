/*
 *    Strahs! - /pretends to be a high level C dialect-like/
 *
 *    Copyright (C) 2021 Cledson Ferreira
 *
 *    This library is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.

 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.

 *    You should have received a copy of the GNU General Public License
 *    along with this library.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "s.h"

#define S_BUF_SIZE 256

string_t * s_new(int len) {
  string_t * s = le_new(string_t, 1);
  if (len < 0) len = 0;

  s->arr = le_new(char, len+1);
  s->arr[0] = 0;
  s->size = len+1;
  return s;
}

string_t * s_newc(const cstring_t str) {
  string_t * s = s_new(strlen(str));
  strcpy(s->arr, str);
  return s;
}

string_t * s_res(string_t * s, int len) {
  if (!s) return s_new(len);
  if (len < 0) len = 0;

  s->arr = le_resize(s->arr, len+1);
  s->arr[len] = 0;
  s->size = len+1;
  return s;
}

void s_free(string_t * s) {
  free(s->arr);
  memset(s, 0, sizeof(string_t));
  free(s);
}

string_t * s_trim(string_t * s) {
  s->size = strlen(s->arr)+1;
  s->arr = le_resize(s->arr, s->size);
  return s;
}

string_t * s_mount(string_t * s, cstring_t str) {
  if (s->arr == str) return s;
  free(s->arr);
  s->arr = str;
  s->size = strlen(str) + 1;
  return s;
}

cstring_t s_umount(string_t * s) {
  cstring_t str = s->arr;
  s->arr = le_new(char, 1);
  s->arr[0] = 0;
  s->size = 1;
  return str;
}

cstring_t s_tocstr(string_t * s) {
  cstring_t arr = s->arr;
  s->arr = le_new(char, s->size);
  strcpy(s->arr, arr);
  return arr;
}

string_t * s_setc(string_t * s, const cstring_t str) {
  s_res(s, strlen(str));
  strcpy(s->arr, str);
  return s;
}

string_t * s_dup(const string_t * co) {
  string_t * dup = s_new(co->size-1);
  strcpy(dup->arr, co->arr);
  return dup;
}

string_t * s_fline(string_t * s, FILE * f) {
  int m=1, len=0;
  s_res(s, S_BUF_SIZE);
  int c = fgetc(f);
  while (c != '\n' && c != EOF) {
    s->arr[len++] = c;
    if (len >= s->size)
      s_res(s, S_BUF_SIZE*(++m));
    c = fgetc(f);
  }
  return s_res(s, len);
}

string_t * s_add(string_t * dest, const string_t * base) {
  int dlen = dest->size-1;
  int blen = base->size-1;

  s_res(dest, dlen+blen);
  strcat(dest->arr, base->arr);

  return dest;
}

string_t * s_builderv(const string_t * s1, ...) {
  va_list vl;
  const string_t * sx = s1;
  string_t * sr = s_dup(s1);

  va_start(vl, s1);
  while ( (sx = va_arg(vl, string_t *)) )
    s_add(sr, sx);

  va_end(vl);
  return sr;
}


static string_t ** _stmp = NULL;
static int _stmp_s = 0;

void S_tmp_free() {
  int i;
  if (!_stmp_s) return;

  for (i=0; i < _stmp_s; ++i)
    if (_stmp[i]) s_free(_stmp[i]);
  free(_stmp);
  _stmp = NULL;
  _stmp_s = 0;
}

int S_tmp_size() {
  return _stmp_s;
}

string_t * S_forget(string_t * s) {
  int i;
  for (i = _stmp_s-1; i >= 0; --i)
    if (_stmp[i] == s) {
      _stmp[i] = NULL;
      break;
    }
  return s;
}

string_t * S(string_t * s) {
  _stmp = le_resize(_stmp, 1+_stmp_s);
  _stmp[_stmp_s++] = s;
  return s;
}

string_t * s_num(double n) {
  string_t * s = S(s_new(25));
  return s_res(s, sprintf(s->arr, "%lg", n));
}
