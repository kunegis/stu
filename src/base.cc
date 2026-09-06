#include "base.hh"

#include "canonicalize.hh"
#include "format.hh"
#include "show_dep.hh"
#include "trace.hh"

void Base_Stack::build_base_dir()
// TODO it may be unnecessary to call canonicalize_string(), if the directories are
// already canonicalized.  In that case, it's enough to ignore the '.' entries.
{
	TRACE_FUNCTION();
	TRACE("dirs.size()= %s", frmt("%zu", dirs.size()));

	if (dirs.empty()) {
		base_dir= "";
		TRACE("base_dir= '%s'", base_dir);
		return;
	}

	size_t i= dirs.size() - 1;
	while (i && dirs[i][0] != '/') --i;
	base_dir= dirs[i];
	for (size_t j= i + 1; j < dirs.size(); ++j) {
		base_dir += '/' + dirs[j];
	}

	char *end= canonicalize_string(A_BEGIN | A_END, base_dir.data());
	base_dir.resize(end - base_dir.data());

	if (base_dir == ".") base_dir= "";

	TRACE("base_dir= '%s'", base_dir);
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
	TRACE("filename= '%s'", filename);
	TRACE("base_dir='%s'", base_dir);
	if (base_dir.empty()) return filename;

	if (is_absolute_for_base(filename)) return filename;
	bool end_in_slash= base_dir[base_dir.size()-1] == '/';
	string sep= end_in_slash ? "" : "/";
	return base_dir + sep + filename;
}

bool is_absolute_for_base(const Name &name)
/* Starts with '/' text, or with param followed by '/' text */
{
	return name.get_texts()[0].size() != 0 && name.get_texts()[0][0] == '/';
}

shared_ptr <const Dep> rebase(shared_ptr <const Dep> d, string base_dir)
{
	TRACE_FUNCTION();
	TRACE("d= %s", show_trace(d));
	TRACE("base_dir='%s'", base_dir);
	assert(d);
	if (base_dir.empty()) {
		TRACE("return %s", show_trace(d));
		return d;
	}

	if (shared_ptr <const Plain_Dep> e= to <const Plain_Dep> (d)) {
		if (is_absolute_for_base(e->object.name)) {
			TRACE("Is absolute for base");
			TRACE("return %s", show_trace(d));
			return d;
		}
		bool end_in_slash= base_dir[base_dir.size()-1] == '/';
		string sep= end_in_slash ? "" : "/";
		shared_ptr <Plain_Dep> f= to <Plain_Dep> (e->clone());
		if (f->variable_name.empty() && f->flags.get_flags() & F_VARIABLE) {
			/* Variable dependencies are always unparametrized */
			assert(f->object.name.get_n() == 0);
			f->variable_name= f->object.name.unparametrized();
		}
		f->object.name.prepend_text(base_dir + sep);
		f->object.name.canonicalize();
		TRACE("return %s", show_trace((shared_ptr <const Dep>)f));
		return f;
	} else if (shared_ptr <const Dynamic_Dep> e2= to <const Dynamic_Dep> (d)) {
		shared_ptr <Dynamic_Dep> f= to <Dynamic_Dep> (e2->clone());
		f->dep= rebase(f->dep, base_dir);
		TRACE("return %s", show_trace(f));
		return f;
	} else if (shared_ptr <const Concat_Dep> e3= to <const Concat_Dep> (d)) {
		shared_ptr <Concat_Dep> f= to <Concat_Dep> (e3->clone());
		if (f->deps.size() != 0)
			f->deps[0]= rebase(f->deps[0], base_dir);
		for (size_t i= 1; i < f->deps.size(); ++i)
			f->deps[i]= rebase_inner(f->deps[i], base_dir);
		TRACE("return %s", show_trace(f));
		return f;
	} else if (shared_ptr <const Compound_Dep> e4= to <const Compound_Dep> (d)) {
		shared_ptr <Compound_Dep> f= to <Compound_Dep> (e4->clone());
		for (size_t i= 0; i < f->deps.size(); ++i)
			f->deps[i]= rebase(f->deps[i], base_dir);
		TRACE("return %s", show_trace((shared_ptr <const Dep>)f));
		return f;
	} else {
		unreachable();
	}
}

shared_ptr <const Dep> rebase_inner(shared_ptr <const Dep> d, string base_dir)
{
	TRACE_FUNCTION();
	TRACE("d= %s", show_trace(d));
	TRACE("base_dir='%s'", base_dir);
	assert(d);
	assert(! base_dir.empty());

	if (to <const Plain_Dep> (d)) {
		return d;
	} else if (shared_ptr <const Dynamic_Dep> e2= to <const Dynamic_Dep> (d)) {
		shared_ptr <Dynamic_Dep> f= to <Dynamic_Dep> (e2->clone());
		f->dep= rebase(f->dep, base_dir);
		return f;
	} else if (shared_ptr <const Concat_Dep> e3= to <const Concat_Dep> (d)) {
		shared_ptr <Concat_Dep> f= to <Concat_Dep> (e3->clone());
		if (f->deps.size() != 0)
			f->deps[0]= rebase(f->deps[0], base_dir);
		for (size_t i= 1; i < f->deps.size(); ++i)
			f->deps[i]= rebase_inner(f->deps[i], base_dir);
		return f;
	} else if (shared_ptr <const Compound_Dep> e4= to <const Compound_Dep> (d)) {
		shared_ptr <Compound_Dep> f= to <Compound_Dep> (e4->clone());
		for (size_t i= 0; i < f->deps.size(); ++i)
			f->deps[i]= rebase_inner(f->deps[i], base_dir);
		return f;
	} else {
		unreachable();
	}
}
