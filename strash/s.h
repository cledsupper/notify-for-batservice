/*
 *    Strahs! (header file) - /pretends to be a high level C dialect-like/
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

#ifndef LESSO_STRASHS
#define LESSO_STRASHS

#include <stdio.h>

typedef char * cstring_t;
typedef struct string_type string_t;

#define le_new(TYPE, N)  (TYPE *) malloc(sizeof(TYPE)*(N))
#define le_resize(POINTER, N)  (typeof(POINTER)) realloc(POINTER, sizeof(* POINTER)*(N))

struct string_type {
  char * arr;
  int size;
};

string_t * s_new(int len);

string_t * s_from(const cstring_t str);

string_t * s_res(string_t * s, int len);

void s_free(string_t * s);

string_t * s_refresh(string_t * s);

string_t * s_mount(string_t * s, cstring_t str);

cstring_t s_umount(string_t * s);

string_t * s_setc(string_t * s, const cstring_t str);

cstring_t s_dupc(string_t * s);

string_t * s_fline(string_t * s, FILE * f);

string_t * s_add(string_t * dest, const string_t * base);

string_t * s_builderv(const string_t * s1, ...);

void S_tmp_free();

int S_tmp_size();

string_t * S_recover(string_t * s);

string_t * S(string_t * s);

string_t * s_num(double n);

#endif
