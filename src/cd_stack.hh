#ifndef CD_STACK_HH
#define CD_STACK_HH

#include <vector>

class CD_Stack
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
};

#endif /* ! CD_STACK_HH */
