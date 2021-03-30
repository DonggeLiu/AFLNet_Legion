//
// Created by Dongge Liu on 30/3/21.
//
#include "../khash.h"
#include "../types.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

KHASH_INIT(hmn, khint32_t, u32, 1, kh_int_hash_func, kh_int_hash_equal)
khash_t(hmn) *khmn_nodes;

int main() {
  int ret, is_missing;
  khiter_t k;
  khmn_nodes = kh_init_hmn();

  for (k = kh_begin(khmn_nodes); k != kh_end(khmn_nodes); ++k) {
    printf("State %u selected %u times\n", kh_key(khmn_nodes, k), kh_value(khmn_nodes, k));
//    assert(kh_exist(khmn_nodes, k));
  }

  k = kh_put_hmn(khmn_nodes, 5, &ret);
  printf("RET: %d\n", ret);
  printf("Exists: %d\n", kh_exist(khmn_nodes, k));
  kh_value(khmn_nodes, k) = 10;
  printf("Exists: %d\n", kh_exist(khmn_nodes, k));

  for (k = kh_begin(khmn_nodes); k != kh_end(khmn_nodes); ++k) {
    if (kh_exist(khmn_nodes, k)) printf("State %u selected %u times\n", kh_key(khmn_nodes, k), kh_value(khmn_nodes, k));
  }

  k = kh_get_hmn(khmn_nodes, 10);
  is_missing = (k == kh_end(khmn_nodes));
  printf("Is missing: %d\n", is_missing);

  k = kh_get_hmn(khmn_nodes, 5);
  kh_value(khmn_nodes, k) += 1;
  printf("State %u selected %u times\n", kh_key(khmn_nodes, k), kh_value(khmn_nodes, k));
  kh_del_hmn(khmn_nodes, k);

  for (k = kh_begin(khmn_nodes); k != kh_end(khmn_nodes); ++k) {
    if (kh_exist(khmn_nodes, k)) printf("State %u selected %u times\n", kh_key(khmn_nodes, k), kh_value(khmn_nodes, k));
  }
  kh_destroy_hmn(khmn_nodes);
  return 0;
}