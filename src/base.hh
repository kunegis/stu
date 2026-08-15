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
	string get_base_dir() const { return base_dir; }
	string rebase(string filename) const;

private:
	std::vector <string> dirs;
	/* - Components are not ""
	 * - Components only end in slash if they consist only of slashes */

	string base_dir;

	void build_base_dir();
};

bool is_absolute_for_base(const Name &);

shared_ptr <const Dep> rebase(shared_ptr <const Dep> d, string base_dir);
//void rebase(Name &name, string base_dir);

#endif /* ! BASE_HH */
