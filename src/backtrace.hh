#ifndef BACKTRACE_HH
#define BACKTRACE_HH

#include "place.hh"

/*
 * A place along with a message.  This class is only used when backtraces cannot be
 * printed immediately.  Otherwise, Place::operator<<() is called directly.
 */
class Backtrace
{
public:
	Place place;

	string message;
	/* May be "".  When the backtrace is printed, it must not be empty,
	 * and must not begin with an uppercase letter. */

	Backtrace(const Place &place_, std::string_view message_)
		: place(place_), message(message_) { }

	void print() const
	/* Print the backtrace to STDERR as part of an error message; see
	 * Place::operator<< for format information. */
	{
		place << message;
	}
};

#endif /* ! BACKTRACE_HH */
