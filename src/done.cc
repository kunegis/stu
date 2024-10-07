#include "done.hh"

bool Done::is_done_from_flags(Flags flags) const
{
	Done d= from_flags(flags);
	return (~bits & d.bits) == 0;
}

Done Done::from_flags(Flags flags)
{
	return Done(
		(~flags & (F_PERSISTENT | F_OPTIONAL))
		* (1 | (flags & F_PHASE_B ? 4 : 0)));
}

#ifndef NDEBUG

string Done::show() const
{
	char ret[7]= "[0000]";
	for (int i= 0; i < 4; ++i)
		ret[1+i] |= 1 & bits >> i;
	return ret;
}

#endif /* ! NDEBUG */
