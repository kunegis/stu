#ifndef BASE_HH
#define BASE_HH

#include <vector>

#include "dep.hh"

class Base_Stack
{
public:
	bool empty() const { return dirs.empty(); }
	void push(string dir);
	void pop();

	string get_base_dir() const;
	/* Empty string if no 'cd' needed */

	string rebase(string filename) const;

private:
	std::vector <string> dirs;
	/* - Components are not ""
	 * - Components only end in slash if they consist only of slashes */

	mutable string base_dir;

};

bool is_absolute_for_base(const Name &);

shared_ptr <const Dep> rebase(shared_ptr <const Dep> d, string base_dir);

#endif /* ! BASE_HH */
