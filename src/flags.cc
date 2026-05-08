#include "flags.hh"

const char *flags_placed_phrases[C_ALL]= {
	"persistent", "optional", "trivial",
	nullptr, nullptr, nullptr,
	"newline-separated", "nul-separated", "stu-syntax",
	"no-dereference",
	nullptr, nullptr, nullptr, nullptr,
};

static_assert(sizeof(flags_placed_phrases) / sizeof(flags_placed_phrases[0]) == C_ALL,
	"Keep in sync with Flags");

Index flag_get_index(char c)
{
	TRACE_FUNCTION();
	TRACE("c= '%s'", frmt("%c", c));
	char *r= strchr((char *)flags_chars, c);
	assert(r);
	return r - flags_chars;
}

bool is_placed_flag_char(char c)
{
	char *r= strchr((char *)flags_chars, c);
	if (!r) return false;
	return (1 << (r - flags_chars)) & F_PLACED;
}
