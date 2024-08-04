#include "trace_executor.hh"

string show_trace(const Executor &executor)
{
	return show(executor, S_DEBUG, R_SHOW_FLAGS);
}
