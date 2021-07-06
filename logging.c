//
// Created by Dongge Liu on 18/12/20.
/*
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
//
#define _GNU_SOURCE
#include "logging.h"

#define MAX_CALLBACKS 32

typedef struct {
    log_LogFn fn;
    void *udata;
    int level;
} Callback;

static struct {
    void *udata;
    log_LockFn lock;
    int level;
    bool quiet;
    Callback callbacks[MAX_CALLBACKS];
} L;


static const char *level_strings[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "ASSERT"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m", "\x1b[34m"
};
#endif

int IGNORE_ASSERT = 0;

static void stdout_callback(log_Event *ev) {
  char buf[16];
  buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
  fprintf(
    ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    buf, level_colors[ev->level], level_strings[ev->level],
    ev->file, ev->line);
#else
  fprintf(
          ev->udata, "%s %-5s %s:%d: ",
          buf, level_strings[ev->level], ev->file, ev->line);
#endif
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void file_callback(log_Event *ev) {
  char buf[64];
  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
  fprintf(
          ev->udata, "%s %-5s %s:%d: ",
          buf, level_strings[ev->level], ev->file, ev->line);
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void lock(void)   {
  if (L.lock) { L.lock(true, L.udata); }
}


static void unlock(void) {
  if (L.lock) { L.lock(false, L.udata); }
}


const char* log_level_string(int level) {
  return level_strings[level];
}


void log_set_lock(log_LockFn fn, void *udata) {
  L.lock = fn;
  L.udata = udata;
}


void log_set_level(int level) {
  L.level = level;
}


void log_set_quiet(bool enable) {
  L.quiet = enable;
}


int log_add_callback(log_LogFn fn, void *udata, int level) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (!L.callbacks[i].fn) {
      L.callbacks[i] = (Callback) { fn, udata, level };
      return 0;
    }
  }
  return -1;
}


int log_add_fp(FILE *fp, int level) {
  return log_add_callback(file_callback, fp, level);
}


static void init_event(log_Event *ev, void *udata) {
  if (!ev->time) {
    time_t t = time(NULL);
    ev->time = localtime(&t);
  }
  ev->udata = udata;
}


void log_log(int level, const char *file, int line, const char *fmt, ...) {
  log_Event ev = {
          .fmt   = fmt,
          .file  = file,
          .line  = line,
          .level = level,
  };

  lock();

  if (!L.quiet && level >= L.level) {
    init_event(&ev, stderr);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }

  for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
    Callback *cb = &L.callbacks[i];
    if (level >= cb->level) {
      init_event(&ev, cb->udata);
      va_start(ev.ap, fmt);
      cb->fn(&ev);
      va_end(ev.ap);
    }
  }

  unlock();
}

char* u32_array_to_str(u32* a, u32 a_len)
{
  // Compute the len of str
  u32 str_len = 0;
  // number of digits
  for (u32 i = 0; i < a_len; i++) {
    if (a[i]) {
      // each number should be within the range
      //assert(0 < a[i] && a[i] <= UINT32_MAX);
      log_assert(0 < a[i] && a[i] <= UINT32_MAX, "[u32_array_to_str] element %u in array is not within the range of UINT: %u", i, a[i]);
      str_len += (u32)(floor(log10(a[i]))) + 1;
    } else {
      str_len += 1;
    }
//    log_debug("code: %d, len %d:\n"
//              "\tlog10(code): %lf\n"
//              "\tfloor(log10(a[i])): %lf\n"
//              "\t(u32)(floor(log10(a[i]))): %d\n"
//              "\t(u32)(floor(log10(a[i]))) + 1: %d",
//              a[i], (u32)(ceil(log10(a[i]))) + 1,
//              log10(a[i]),
//              floor(log10(a[i])),
//              (u32)(floor(log10(a[i]))),
//              (u32)(floor(log10(a[i]))) + 1
//              );
  }
  // number of delimiters (", ")
  str_len += 2 * (a_len - 1);
//  log_debug("str_len: %d", str_len);
  // The size of str = len*char_size + 1
  char* str = (char *) malloc(str_len * sizeof (char) + 1);

  // Write to str, the terminating null character is automatically appended after the content written
  u32 n = 0, m = 0;
  for (u32 i = 0; i < a_len; i++) {
    // the last delimiter (", ") will be omitted as it won't fit in the size of str
    m = sprintf(str + n, "%d", a[i]);
//    m = snprintf(str + n, str_len-n, "%d", a[i]);
    if (m<0) {
      log_fatal("Failure in snprintf writing with str_len %d", str_len);
      exit(1);
    }
    n += m;

    if (strlen(str) < str_len){
      m = sprintf(str + n, ", ");
//      m = snprintf(str + n, str_len-n, ", ");
      if (m<0) {
        log_fatal("Failure in snprintf writing delimiters");
        exit(1);
      }
      n += m;
    }

  }
  return str;
}
/* TOASK: Why does not this work? */
//int message_array(char** message, u32* a, u32 a_len) {
//  assert(a_len);
//  int a_str_len = - 1;
//  char* new_message = NULL;
//  if (*message) {a_str_len = message_append(&new_message, "%s%u", *message, a[0]);}
//  else  {a_str_len = message_append(&new_message, "%u", a[0]);}
//  *message = malloc(a_str_len);
//  strncpy(*message, new_message, a_str_len);
//  free(new_message);
//
//  for (int i = 1; i < a_len; ++i) {
//    if (a_str_len == -1)  {exit(1);}
//    new_message = NULL;
//    a_str_len = message_append(&new_message, "%s, %u", message, a[i]);
//    /* TOASK: How to free message? */
////    free(*message);
//    *message = malloc(a_str_len);
//    strncpy(*message, new_message, a_str_len);
//    free(new_message);
////    *message = new_message;
//  }
//  return a_str_len;
//}

int message_append(char** message, const char *fmt, ...) {
  va_list argptr;
  char *fmt_str = NULL;
  va_start(argptr,fmt);
  int fmt_str_len = vasprintf(&fmt_str, fmt, argptr); /* vasprintf returns the str length including the `\0` */
  va_end(argptr);
//  log_debug("Source_len: %d", fmt_str_len);
//  log_debug("Source: %s", fmt_str);

  if (fmt_str_len == -1) {exit(1);}

  if (*message) {
    char* new_message = NULL;
    fmt_str_len = message_append(&new_message, "%s%s", *message, fmt_str);
    /* TOASK: How to free message? */
//    free(*message);
    *message = new_message;
  } else {
    /* TOASK: How to free message? */
//    free(*message);
    *message = fmt_str;
  }
  return fmt_str_len;
}


void set_ignore_assertion(int ignore_assertion) {
  IGNORE_ASSERT = ignore_assertion;
}

void log_assertion(int condition, int level, const char *file, int line, const char *fmt, ...) {

  if (!IGNORE_ASSERT) {assert(condition);}

  if (condition) {return;}

  log_Event ev = {
          .fmt   = fmt,
          .file  = file,
          .line  = line,
          .level = level,
  };

  lock();

  if (!L.quiet && level >= L.level) {
    init_event(&ev, stderr);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }

  for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
    Callback *cb = &L.callbacks[i];
    if (level >= cb->level) {
      init_event(&ev, cb->udata);
      va_start(ev.ap, fmt);
      cb->fn(&ev);
      va_end(ev.ap);
    }
  }

  unlock();
}
