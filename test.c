#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>

#define SIZE 100

int message_format(char* start, const char *fmt, ...);

int main(int argc, char** argv){

  char* start = malloc(5);
  free(start);
  int a = 3;
  message_format(start, "asdf%d\n", a);

//  printf("%lf, %d",ceil((double)strlen(abc)/SIZE),  (int) ceil((double)strlen(abc)/SIZE));
//  char* str = malloc(40);
//  printf("sizeof: %lu, strlen: %lu, str:%s\n", sizeof(str), strlen(str), str);
  //for(int i=0;i<9;i++) {
  //  strcat(str, "abc");
  //  printf("sizeof: %lu, strlen: %lu, str:%s\n", sizeof(str), strlen(str), str);
  //}
//  printf("%lu", snprintf(NULL, 0, "%s", abc));
  return 0;
}

int message_format(char* start, const char *fmt, ...) {
  va_list argptr;
  char *vafmt = NULL;
  va_start(argptr,fmt);
  int source_len = vasprintf(&vafmt, fmt, argptr);
  va_end(argptr);
  printf("Source_len: %d", source_len);
  printf("source = %s", vafmt);
//  u32 multiplier = (u32) ceil((double) source_len/EXTEND_MESSAGE_SIZE);
  char* new_space = malloc(0 + source_len);


  if(!new_space) {exit(1);}
  else {*new_space = '\0';}

  if (start) {strcpy(new_space, start);}
  free(start);
//  message->content = new_space;
//  message->size += source_len;

//  va_list argptr;
  start = new_space;
  va_start(argptr,fmt);
  char* source = malloc(source_len);
  snprintf(source, source_len, fmt, argptr);
  va_end(argptr);
  strcat(start, source);
  return 0;
}