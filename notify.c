/*
 *    Notify for BatService - a notify.sh replacement written in
 *    (almost) pure C language.
 *
 *    Copyright (C) 2021, 2022 Cledson Ferreira
 *
 *    Notify for BatService is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.

 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.

 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "strash/s.h"

#define LOG_SIZE_MAX 30000
#define LOG_LINES_TO_DEL 1700

char * LIB = NULL;
char * DATA = NULL;
char * DATA_CONFIG = NULL;
char * EXIT_FILE = NULL;
char * CACHE_FILE = NULL;
bool TERMUX_API = false;

bool SUPPRESS_LOGS = false;


static bool check_termux_api() {
  return !system("termux-notification -h >/dev/null 2>&1");
}

void finalize(int r);

void init() {
  string_t * lib, * data, * cache;
  char * NO_PERMS = getenv("NO_PERMS");
  if (NO_PERMS) {
    TERMUX_API = getenv("TERMUX_API") ? true : false;
    lib = S(s_from(getenv("LIB")));
    data = S(s_from(getenv("DATA")));
    cache = S(s_from(getenv("CACHE")));
  }
  else {
    TERMUX_API = check_termux_api();

    char * PREFIX = getenv("PREFIX");
    if (!PREFIX) {
      fprintf(stderr, "ERRO FATAL: PREFIX não definido!\n");
      exit(1);
    }

    string_t * prefix = S(s_from(PREFIX));

    if (!getenv("LIB_FIX")) {
      lib = S(s_builderv(
        prefix, S(s_from("/lib/batservice")), NULL
      ));
      data = S(s_builderv(
        prefix, S(s_from("/etc/batservice")), NULL
      ));
    }
    else {
      lib = S(s_from(getenv("LIB_FIX")));
      data = S(s_from(getenv("DATA_FIX")));
    }

    char * HOME = getenv("HOME");
    if (!HOME) {
      fprintf(stderr, "ERRO FATAL: HOME não definido!\n");
      finalize(1);
    }
    cache = S(s_builderv(
      S(s_from(HOME)), S(s_from("/.cache/BatService")), NULL
    ));
  }

  LIB = s_umount(lib);
  DATA = s_dupc(data);

  string_t * data_config = S(s_builderv(data, S(s_from("/config.txt")), NULL));
  string_t * exit_file = S(s_builderv(data, S(s_from("/exit.err")), NULL));
  DATA_CONFIG = s_umount(data_config);
  EXIT_FILE = s_umount(exit_file);

  system(S(s_builderv(
    S(s_from("mkdir -p \"")), cache, S(s_from("\"")), NULL
  ))->arr);

  string_t * cache_file = S(s_builderv(cache, S(s_from("/out.log")), NULL));
  CACHE_FILE = s_umount(cache_file);

  S_tmp_free();
}

void finalize(int r) {
  if (LIB) free(LIB);
  if (DATA) free(DATA);
  if (DATA_CONFIG) free(DATA_CONFIG);
  if (EXIT_FILE) free(EXIT_FILE);
  if (CACHE_FILE) free(CACHE_FILE);
  S_tmp_free();
  exit(r);
}


void spawn_and_kill(char * cmd) {
  int child_id;
  string_t * sh;

  sh = S(s_builderv(
    S(s_from(getenv("PREFIX"))),
    S(s_from("/bin/sh")),
    NULL
  ));

  signal(SIGCHLD, SIG_IGN);

  child_id = fork();
  if (!child_id) {
    char * argv[] = { "sh", "-c", cmd, NULL };
    execv(sh->arr, argv);
    fprintf(stderr, "ERR: Notify: erro ao executar processo!\n");
    finalize(-1);
  }
  else {
    FILE * fcomm;
    char comm[20];
    int tl;
    snprintf(comm, 20, "/proc/%d/comm", child_id);
    for (tl=7; tl > 0; --tl) {
      fcomm = fopen(comm, "r");
      if (!fcomm) break;
      fclose(fcomm);
      sleep(1);
    }
    // Ignorar processo conforme notify.sh
  }

  S_tmp_free();
}

void send_toast(const char * msg) {
  printf("ALERTA: %s\n", msg);
  if (!TERMUX_API)
    return;

  spawn_and_kill(
    S(s_builderv(
      S(s_from("termux-toast 'BatService: ")), S(s_from(msg)),
      S(s_from("'")),
      NULL
    ))->arr
  );
}

void send_message(const char * msg) {
  printf("MENSAGEM: %s\n", msg);
  if (!TERMUX_API)
    return;

  spawn_and_kill(
    S(s_builderv(
      S(s_from("termux-notification -i batservice-msg ")),
      S(s_from("--icon battery_std -t 'Mensagem do BatService' -c '")),
      S(s_from(msg)), S(s_from("'")), NULL
    ))->arr
  );
}

int get_charging_never_stop();

void send_status(const char * msg) {
  if (!TERMUX_API) {
    if (SUPPRESS_LOGS) printf("STATUS: %s\n", msg);
    return;
  }

  string_t * btn, * lib = S(s_from(LIB));
  if (get_charging_never_stop() == 1)
    btn = S(s_from("ᐅ"));
  else
    btn = S(s_from("□"));

  spawn_and_kill(S(s_builderv(
    S(s_from("termux-notification -i batservice --ongoing --alert-once ")),
    S(s_from("--icon battery_std -t 'Status do serviço' -c '")), S(s_from(msg)),
    S(s_from("' --button1 '")), btn,
    S(s_from("' --button1-action 'DATA_FIX=\"")), S(s_from(DATA)), S(s_from("\" LIB_FIX=\"")), S(s_from(LIB)), S(s_from("\" sh ")),
    lib, S(s_from("/notify.sh force-charge' ")),
    S(s_from("--button2 'X' --button2-action 'DATA_FIX=\"")), S(s_from(DATA)), S(s_from("\" LIB_FIX=\"")), S(s_from(LIB)), S(s_from("\" ")),
    lib, S(s_from("/notify quit'")),
    NULL
  ))->arr);
}

typedef struct status_type status_t;

struct status_type {
  string_t * status;
  int percent;
  int current;
  int voltage;
  int temp;
};

void notify_status(status_t status) {
  string_t * p;
  if (!strcmp(status.status->arr, "Not charging") || !strcmp(status.status->arr, "Charging"))
    p = S(s_builderv(S(s_from(" % (🔌 ")), s_num(status.current), S(s_from(" mA) ⚡ ")), NULL));
  else
    p = S(s_from(" % ⚡ "));

  string_t * statustxt = S(s_builderv(
    S(s_from("🔋 ")), s_num(status.percent), p, s_num(status.voltage),
    S(s_from(" mV 🌡 ")), s_num(status.temp), S(s_from(" ºC")), NULL
  ));

  send_status(statustxt->arr);
}


void log_cleanup() {
  if (SUPPRESS_LOGS) return;

  struct stat buf;
  FILE * cache = fopen(CACHE_FILE, "r");
  if (!cache) {
    fprintf(stderr, "ERRO FATAL: falha ao abrir log!\n");
    finalize(2);
  }

  stat(CACHE_FILE, &buf);
  if (buf.st_size > LOG_SIZE_MAX) {
    int lines = LOG_LINES_TO_DEL;
    string_t * line = NULL;
    while (lines) {
      int c = fgetc(cache);
      if (c == '\n') --lines;
      if (c == EOF) break;
    }

    if (!lines) {
      int m=1, i=0, arr_s=64;
      string_t ** arr = le_new(string_t *, arr_s);

      while (S(line = s_fline(s_new(0), cache))->size > 1) {
        arr[i++] = line;
        if (i >= arr_s) {
          arr_s = 64*(++m);
          arr = le_resize(arr, arr_s);
        }
      }
      arr[i] = NULL;
      fclose(cache);

      freopen(CACHE_FILE, "w", stdout);
      for (i=0; arr[i]; ++i)
        puts(arr[i]->arr);
      free(arr);
    }
    else {
      fclose(cache);
      freopen(CACHE_FILE, "w", stdout);
    }
  }
  else fclose(cache);

  S_tmp_free();
}


int main(int args, char * arg[]) {
  init();

  if (args > 1) {
    if (!strcmp(arg[1], "--no-logs"))
      SUPPRESS_LOGS=true;
    else {
      if (!strcmp(arg[1], "quit")) {
        FILE * f_exit = fopen(EXIT_FILE, "w");
        if (!f_exit) {
          fprintf(stderr, "ERR: falha ao criar arquivo de saída!\n");
          finalize(2);
        }
        fputs("0", f_exit);
        fclose(f_exit);
        send_toast("O serviço será encerrado");
        finalize(0);
      }
      fprintf(stderr, "ERR: não implementado!\n");
      finalize(3);
    }
  }

  if (!SUPPRESS_LOGS) freopen(CACHE_FILE, "a", stdout);
  else if (getenv("BTDROID")) freopen("/dev/null", "w", stdout);

  string_t * log_line = s_new(0);
  status_t status = { s_new(0), 0, 0, 0, 0 };
  int ign;
  int val;

  while (!feof(stdin)) {
    s_fline(log_line, stdin);
    puts(log_line->arr);
    if (!strcmp(log_line->arr, "=============================="))
      break;
  }

  while (1) {
    s_fline(log_line, stdin);
    if (feof(stdin) || !strcmp(log_line->arr, "==============================")) {
      puts(log_line->arr);
      break;
    }

    ign=0;
    if (log_line->arr[0] == '#') {
      ign=1;
      if (!strncmp(log_line->arr+1, "upd ", 4))
        send_toast(log_line->arr+5);
      else if (!strncmp(log_line->arr+1, "msg ", 4))
        send_message(log_line->arr+5);
      else ign=2;
    }

    if (!(ign%2) && sscanf(log_line->arr + ign, "%d", &val) > 0) {
      char * p = log_line->arr + ign + s_num(val)->size;

      while (!isgraph(*p) && *p) ++p;
      if (*p == '%') {
        char * q;
        // 45 % (Discharging) 500 mA 3815 mV 29 °C
        status.percent = val;
        do { ++p; } while (!isalnum(*p));
        q = p;

        s_setc(status.status, p);
        p = status.status->arr;
        while (*p != ')') ++p;
        *p = 0;
        p = q + (p - status.status->arr) + 1;
        while (!isalnum(*p) && *p != '-') ++p;
        s_refresh(status.status);

        sscanf(p, "%d", &status.current);
        p += s_num(status.current)->size + 3;
        while (!isalnum(*p) && *p != '-') ++p;

        sscanf(p, "%d", &status.voltage);
        p += s_num(status.voltage)->size + 3;
        while (!isalnum(*p) && *p != '-') ++p;

        sscanf(p, "%d", &status.temp);

        S_tmp_free();
        notify_status(status);
      }
    }

    if (!ign) {
      puts(log_line->arr);
      fflush(stdout);
      log_cleanup();
    }
  }

  while (!feof(stdin)) {
    s_fline(log_line, stdin);
    puts(log_line->arr);
  }

  s_free(log_line);
  s_free(status.status);
  if (TERMUX_API) system("termux-notification-remove batservice");
  finalize(0);
}


int get_charging_never_stop() {
  FILE * cfg = fopen(DATA_CONFIG, "r");
  if (!cfg) return 2;

  int r = 2;
  string_t * line = S(s_new(0));
  string_t * key = S(s_new(0));
  while (s_fline(line, cfg)->size > 1) {
    s_res(key, line->size-1);
    sscanf(line->arr, "%s", key->arr);
    s_refresh(key);
    if (!strcmp(key->arr, "charge-never-stop")) {
      string_t * value = S(s_new(line->size - key->size));
      sscanf(line->arr + key->size, "%s", value->arr);
      r = 0;
      if (!strcmp(value->arr, "true"))
        r = 1;
      break;
    }
  }
  return r;
}
