#include "flags.hh"

Flag_Info flag_info[C_ALL]= {
	{"persistent",     "persistent"},
	{"optional",       "optional"},
	{"trivial",        "trivial"},
	{nullptr,          nullptr},
	{nullptr,          nullptr},
	{nullptr,          nullptr},
	{"newline",        "newline-separated"},
	{"null",           "nul-separated"},
	{"full-syntax",    "full-syntax"},
	{"no-dereference", "no-dereference"},
	{nullptr,          nullptr},
	{nullptr,          nullptr},
	{nullptr,          nullptr},
	{nullptr,          nullptr},
};

static_assert(sizeof(flag_info) / sizeof(flag_info[0]) == C_ALL,
	"Keep in sync with Flags");

Index flag_get_index(char c)
{
	TRACE_FUNCTION();
	TRACE("c= '%s'", frmt("%c", c));
	char *r= strchr((char *)flag_chars, c);
	assert(r);
	return r - flag_chars;
}

Index index_from_name(string name)
{
	static std::map <string, Index> indexes;
	if (indexes.size() == 0) {
		for (Index i= 0; i < C_ALL; ++i) {
			if (!flag_info[i].name) continue;
			indexes[flag_info[i].name]= i;
		}
	}
	auto i= indexes.find(name);
	if (i == indexes.end()) return I_ERR;
	return i->second;
}

bool is_placed_flag_char(char c)
{
	char *r= strchr((char *)flag_chars, c);
	if (!r) return false;
	return (1 << (r - flag_chars)) & F_PLACED;
}

bool is_flag_name_char(char c)
{
	return islower(c) || c == '-';
}
