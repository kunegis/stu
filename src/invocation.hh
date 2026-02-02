#ifndef INVOCATION_HH
#define INVOCATION_HH

#include "dep.hh"
#include "place.hh"

class Invocation
{
public:
	Invocation(int argc, char **argv, int &error);
	void main_loop();

private:
	std::vector <string> filenames;
	/* Filenames passed using the -f option.  Entries are unique and sorted as they
	 * were given, except for duplicates. */

	std::vector <shared_ptr <const Dep> > deps;

	shared_ptr <const Plain_Dep> target_first;
	/* Set to the first target of the first rule when there is one */

	Place place_first; /* Place of first file when no rule is contained */

	bool had_option_target= false;
	/* Whether any target(s) was passed through one of the options -c, -C, -o, -p, -n,
	 * -0.  Also set when zero targets are passed through one of these, e.g., when -n
	 * is used on an empty file. */

	bool had_option_f= false; /* Both -f and -F */
};

#endif /* ! INVOCATION_HH */
