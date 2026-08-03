#ifndef CD_STACK_HH
#define CD_STACK_HH

#include <vector>

class CD_Stack
{
public:
	string get_base_dir() const;
	void push(string dir);
	void pop();
	
private:
	std::vector <string> dirs;
};

#endif /* ! CD_STACK_HH */
