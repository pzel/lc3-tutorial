/* Binary literals for 16-bit integers */

/* word_from_string produces a 16-bit unsigned integer
   from an arbitrary zero-terminated string, as long as
   the string contains *exactly 16* characters from
   the set: {'0', '1'};

   This gets converted to a unit16_t, like so:

       uint16_t i = word_from_string("0000 0000 0000 1001");
       assert(i == 0x9);


   Arbitrary comments are allowed:

       uint16_t commented = word_from_string("0000:some zeroes; "
                                             "0000(more zeores); "
                                             "0000 ^^; "
                                             "1001 finally! some ones");
       assert(commented == 0x9);

*/

#include <stdint.h>
uint16_t word_from_string(char*);
