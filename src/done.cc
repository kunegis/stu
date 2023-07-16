#include "done.hh"

string Done::format() const
{
	char ret[7]= "[0000]";
	for (int i= 0; i < 4; ++i)
		ret[1+i] |= 1 & bits << i;
	return ret;
}

Done Done::from_flags(Flags flags)
{
	return Done((~flags & (F_PERSISTENT | F_OPTIONAL))
		    * (1 | ((~flags & F_PHASE_A) != 0)));
}
