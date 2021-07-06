#include "logging.h"

int main(){
  char log_file[100];

  snprintf(log_file, sizeof(log_file), "%s", getenv("FUZZER_LOG"));
  log_add_fp(fopen(log_file, "w+"), 0);
  log_set_quiet(1);
  set_ignore_assertion(1);
  log_info("[INITIALISATION] LOG PATH: %s", log_file);
  log_info("[INITIALISATION] Log level: %u", 0);
  log_info("[INITIALISATION] Ignore assertions: %s", 1?"True":"False");

  log_assert(0 < 5, "0 < 5");
  log_assert(0 > 5, "0 > 5");

  log_info("End");

  return  0;
}
