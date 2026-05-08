#include "show_dep.hh"

void render(Dynamic_Variable_View v, Parts &parts, Rendering)
{
	parts.append_marker("$[");
	parts.append_text(v.name);
	parts.append_marker("]");
}

#ifndef NDEBUG

string show_trace(const shared_ptr <const Dep> &dep)
{
	return show(dep, S_DEBUG, R_SHOW_FLAGS | R_SHOW_INDEX);
}

string show_trace(const shared_ptr <Dep> &dep)
{
	shared_ptr <const Dep> d= dep;
	return show_trace(d);
}

string show_trace(const shared_ptr <const Dynamic_Dep> &dep)
{
	shared_ptr <const Dep> d= dep;
	return show_trace(d);
}

string show_trace(const shared_ptr <Dynamic_Dep> &dep)
{
	shared_ptr <const Dep> d= dep;
	return show_trace(d);
}

string show_trace(const shared_ptr <Concat_Dep> &dep)
{
	shared_ptr <const Dep> d= dep;
	return show_trace(d);
}

#endif /* ! NDEBUG */
