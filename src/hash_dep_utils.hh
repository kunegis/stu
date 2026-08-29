#ifndef HASH_DEP_UTILS_HH
#define HASH_DEP_UTILS_HH

// TODO join the three 'hash_dep' source files.

#include <stdint.h>

#include "show.hh"

typedef uint16_t word_t;
// TODO find a way to automate the choice of uint<N>_t based on C_WORD.  (Will make the
// static_assert unnecessary)
typedef size_t word_size_t; /* Index in the string */

static_assert(sizeof(word_t) == sizeof(uint8_t) && C_WORD <= 8
	|| sizeof(word_t) == sizeof(uint16_t) && C_WORD > 8 && C_WORD <= 16);

static string string_from_word(Flags flags);
/* Return a string of length sizeof(word_t) containing the given flags */
static string string_from_size(size_t size);

#endif /* ! HASH_DEP_UTILS_HH */
