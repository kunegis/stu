#include "cycle.hh"

#include "explain.hh"
#include "trace_executor.hh"

bool Cycle::find(
	Executor *parent,
	Executor *child,
	shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION();
	TRACE("parent= %s", show_trace(*parent));
	TRACE("child= %s", show_trace(*child));
	std::vector <Executor *> path;
	path.push_back(parent);
	return find(path, child, dep_link);
}

bool Cycle::find(
	std::vector <Executor *> &path,
	Executor *child,
	shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION();
	if (Executor::same_rule(path.back(), child)) {
		print(path, dep_link);
		return true;
	}
	for (auto &i: path.back()->get_parents()) {
		Executor *next= i.first;
		assert(next != nullptr);
		path.push_back(next);
		bool found= find(path, child, dep_link);
		if (found)
			return true;
		path.pop_back();
	}
	return false;
}

void Cycle::print(
	const std::vector <Executor *> &path,
	shared_ptr <const Dep> dep)
/*
 * Given PATH = [a, b, c, d, ..., x], we print:
 *
 *	x depends on ...      \
 *      ... depends on d      |
 *      d depends on c        | printed from PATH
 *      c depends on b        |
 *      b depends on a        /
 *      a depends on x        > printed from DEP
 *      x is needed by ...    \
 *      ...                   | printed by Backtrace
 *      ...                   /
 */
{
	TRACE_FUNCTION();
	assert(path.size() > 0);

	std::vector <string> names;
	/* Indexes are parallel to PATH */
	names.resize(path.size());

	for (size_t i= 0; i + 1 < path.size(); ++i)
		names[i]= ::show(path[i]->get_parents().at(path[i+1]));
	names.back()= ::show(path.back()->get_parents().begin()->second);

	for (ssize_t i= path.size() - 1; i >= 0; --i) {
		shared_ptr <const Dep> d= i == 0 ? dep :
			path[i - 1]->get_parents().at(const_cast <Executor *> (path[i]));

		/* Don't show a message for left-branch dynamic links */
		if (Executor::hide_link_from_message(d->flags))
			continue;

		d->get_place() << fmt(
			"%s%s depends on %s",
			i == (ssize_t)(path.size() - 1)
			? (path.size() == 1
				|| (path.size() == 2 && Executor::hide_link_from_message(dep->flags))
				? "target must not depend on itself: "
				: "cyclic dependency: ")
			: "",
			names[i],
			i == 0 ? ::show(dep) : names[i - 1]);
	}

	/* If the two targets are different (but have the same rule because they match the
	 * same pattern and/or because they are two different targets of a multitarget
	 * rule), then output a notice to that effect. */
	Hash_Dep t1= path.back()->get_parents().begin()->second->get_target();
	Hash_Dep t2= dep->get_target();
	const char *c1= t1.get_name_c_str_any();
	const char *c2= t2.get_name_c_str_any();
	if (strcmp(c1, c2)) {
		path.back()->get_place()
			<< fmt("both %s and %s match the same rule",
				::show(c1), ::show(c2));
	}

	/* Remove the offending (cycle-generating) link between the two.  The offending
	 * link is from path[0] as a parent to path[end] (as a child). */
	path.back()->get_parents().erase(path.at(0));
	path.at(0)->get_children().erase(path.back());

	*path.back() << "";
	explain_cycle();
}
