#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "binlit.h"

uint16_t word_from_string(char* source) {
  char clean[16];
  char c;
  int n_chars = 0;
  uint16_t result = 0;

  while((c = *source++) != 0) {
    /* Only account for '0' and '1' chars, ignore others */
    if(c == '1') {
      clean[n_chars++] = 1;
    } else if (c == '0') {
      clean[n_chars++] = 0;
    }
  }
  /* Make sure there were exactly 16 zeroes or ones in source */
  assert(n_chars == 16);

  for (int p=16; p>0; p--) {
    if(clean[16-p]) { result += (1 << (p-1)); }
  }
  return result;
}
