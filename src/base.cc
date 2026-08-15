#include "base.hh"

#include "show_dep.hh"
#include "trace.hh"

void Base_Stack::build_base_dir()
{
	if (dirs.empty()) {
		base_dir= "";
		return;
	}

	size_t i= dirs.size() - 1;
	while (i && dirs[i][0] != '/') --i;
	base_dir= dirs[i];
	for (size_t j= i + 1; j < dirs.size(); ++j) {
		base_dir += '/' + dirs[j];
	}
}

void Base_Stack::push(string dir)
{
	TRACE_FUNCTION();
	TRACE("dir= '%s'", dir);
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
	TRACE("After canonicalization: dir= '%s'", dir);
	assert(! dir.empty());
	dirs.push_back(dir);
	build_base_dir();
}

void Base_Stack::pop()
{
	TRACE_FUNCTION();
	assert(! dirs.empty());
	base_dir= "";
	dirs.pop_back();
	build_base_dir();
}

string Base_Stack::rebase(string filename) const
{
	TRACE_FUNCTION();
	TRACE("base_dir='%s'", base_dir);
	if (base_dir.empty()) return filename;

	if (is_absolute_for_base(filename)) return filename;
	bool end_in_slash= base_dir[base_dir.size()-1] == '/';
	string sep= end_in_slash ? "" : "/";
	return base_dir + sep + filename;
}

shared_ptr <const Dep> rebase(shared_ptr <const Dep> d, string base_dir)
{
	TRACE_FUNCTION();
	TRACE("d= %s", show_trace(d));
	TRACE("base_dir='%s'", base_dir);
	assert(d);
	if (base_dir.empty()) return d;

	if (shared_ptr <const Plain_Dep> e= to <const Plain_Dep> (d)) {
		if (is_absolute_for_base(e->object.name)) return d;
		bool end_in_slash= base_dir[base_dir.size()-1] == '/';
		string sep= end_in_slash ? "" : "/";
		shared_ptr <Plain_Dep> f= to <Plain_Dep> (e->clone());
		f->object.name.prepend_text(base_dir + sep);
		f->object.name.canonicalize();
		return f;
	} else if (shared_ptr <const Dynamic_Dep> e2= to <const Dynamic_Dep> (d)) {
		shared_ptr <Dynamic_Dep> f= to <Dynamic_Dep> (e2->clone());
		f->dep= rebase(f->dep, base_dir);
		return f;
	} else if (shared_ptr <const Concat_Dep> e3= to <const Concat_Dep> (d)) {
		shared_ptr <Concat_Dep> f= to <Concat_Dep> (e3->clone());
		if (f->deps.size() != 0)
			f->deps[0]= rebase(f->deps[0], base_dir);
		return f;
	} else if (shared_ptr <const Compound_Dep> e4= to <const Compound_Dep> (d)) {
		shared_ptr <Compound_Dep> f= to <Compound_Dep> (e4->clone());
		for (size_t i= 0; i < f->deps.size(); ++i)
			f->deps[i]= rebase(f->deps[i], base_dir);
		return f;
	} else {
		unreachable();
	}
}

//void rebase(Name &name, string base_dir)
//{
//	TRACE_FUNCTION();
//	TRACE("base_dir= '%s'", base_dir);
//	if (is_absolute_for_base(name)) return;
//	bool end_in_slash= base_dir[base_dir.size()-1] == '/';
//	string sep= end_in_slash ? "" : "/";
//	name.prepend_text(base_dir + sep);
//	name.canonicalize();
//}

bool is_absolute_for_base(const Name &name)
/* Starts with '/' text, or with param followed by '/' text */
{
	return
		(name.get_texts()[0].size() != 0 && name.get_texts()[0][0] == '/') ||
		(name.get_texts()[0].empty() &&
			name.get_n() != 0 &&
			name.get_texts()[1].size() &&
			name.get_texts()[1][0] == '/');
}
