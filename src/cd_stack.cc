#include "cd_stack.hh"

string CD_Stack::get_base_dir() const
{
	...;
}

void CD_Stack::push(string dir)
{
	assert(dir != "");
}

void CD_Stack::pop()
{
	assert(! dirs.empty());
}
