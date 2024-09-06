#include "proceed.hh"

constexpr const char *const proceed_names[]= {
	"P_WAIT", "P_CALL_AGAIN", "P_FINISHED", "P_ABORT"
};
static_assert(sizeof(proceed_names)/sizeof(proceed_names[0]) == P_COUNT);

string show_proceed(Proceed proceed)
{
	string ret;
	for (int i= 0; i < P_COUNT; ++i) {
		if (!(proceed & (1 << i))) continue;
		if (ret.size()) ret += "|";
		ret += proceed_names[i];
	}
	if (ret.size() == 0) ret += "0";
	return ret;
}
