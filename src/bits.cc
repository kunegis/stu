#include "bits.hh"

const char *bits_text[]= {
	"NEED_BUILD", "CHECKED", "EXISTING", "MISSING"
};
static_assert(B_COUNT == sizeof(bits_text)/sizeof(bits_text[0]));

#ifndef NDEBUG

string show_bits(Bits bits)
{
	string ret;
	for (int i= 0; i < B_COUNT; ++i) {
		if (bits & (1 << i)) {
			if (ret.size()) ret += "|";
			ret += bits_text[i];
		}
	}
	if (!bits) ret= "0";
	return ret;
}

#endif /* ! NDEBUG */
