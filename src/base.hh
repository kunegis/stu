#ifndef BASE_HH
#define BASE_HH

/*
 * The base directory can be empty, in which case no 'cd' is needed.
 */

#include <vector>

#include "dep.hh"

class Base_Stack
{
public:
	bool empty() const { return dirs.empty(); }
	void push(string dir);
	void pop();
	string get_base() const { return base; }
	string rebase(string filename) const;

private:
	std::vector <string> dirs;
	/* - Components are not ""
	 * - Components only end in slash if they consist only of slashes */

	string base;

	void build_base();
};

bool is_absolute_for_base(const Name &);

shared_ptr <const Dep> rebase(shared_ptr <const Dep> d, string base);
shared_ptr <const Dep> rebase_inner(shared_ptr <const Dep> d, string base);

#endif /* ! BASE_HH */
