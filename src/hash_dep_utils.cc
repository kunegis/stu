#include "hash_dep_utils.hh"

string string_from_word(Flags flags)
{
	assert(flags < 1 << C_WORD);
	char ret[sizeof(word_t) + 1];
	ret[sizeof(word_t)]= '\0';
	word_t w= (word_t)flags;
	memcpy(ret, &w, sizeof(word_t)); /* Assigning it directly is undefined behavior */
	return string(ret, sizeof(word_t));
}

string string_from_size(size_t size)
{
	string ret(sizeof(word_size_t), 0);
	*(word_size_t *)ret.data()= size;
	return ret;
}
