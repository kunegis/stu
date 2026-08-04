#include "cd_stack.hh"

string CD_Stack::get_base_dir() const
{
	if (dirs.empty()) return "";
	size_t i= dirs.size() - 1;
	while (i && dirs[i][0] != '/') --i;
	string ret= dirs[i];
	for (size_t j= i + 1; j < dirs.size(); ++j) {
		ret += '/' + dirs[i];
	}
	return ret;
}

void CD_Stack::push(string dir)
{
	assert(dir != "");
	size_t i= dir.size() - 1;
	for (; i; --i) {
		if (dir[i] != '/') break;
	}
	if (i) {
		dir.resize(i+1);
	} else {
		if (dir[0] != '/') dir.resize(1);
	}
	assert(! dir.empty());
	dirs.push_back(dir);
}

void CD_Stack::pop()
{
	assert(! dirs.empty());
	dirs.pop_back();
}
