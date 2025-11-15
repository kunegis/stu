#include "proceed.hh"

#ifndef NDEBUG

bool is_valid(Proceed proceed)
{
	return
		proceed == (P_WAIT) ||
		proceed == (P_CALL_AGAIN) ||
		proceed == (P_WAIT | P_CALL_AGAIN) ||
		proceed == P_NOTHING;
}

string show(Proceed proceed)
{
	static constexpr const char *const proceed_names[]= {
		"P_WAIT", "P_CALL_AGAIN"
	};
	static_assert(sizeof(proceed_names)/sizeof(proceed_names[0]) == P_COUNT);

	string ret;
	for (int i= 0; i < P_COUNT; ++i) {
		if (!(proceed & (1 << i))) continue;
		if (ret.size()) ret += "|";
		ret += proceed_names[i];
	}
	if (ret.size() == 0) ret += "P_NOTHING";
	return ret;
}

#endif /* ! NDEBUG */
