#ifndef BASE_HH
#define BASE_HH

#include <vector>

#include "dep.hh"

class Base_Stack
{
public:
	string get_base_dir() const;
	/* Returns empty string if no 'cd' needed */

	bool empty() const { return dirs.empty(); }
	void push(string dir);
	void pop();

private:
	std::vector <string> dirs;
	/* - Components are not ""
	 * - Components only end in slash if they consist only of slashes */

	mutable string base_dir;
};

shared_ptr <const Dep> rebase(shared_ptr <const Dep> d, string base_dir);

bool is_absolute_for_base(const Name &);

#endif /* ! BASE_HH */
