#include "timestamp.hh"

#if USE_MTIM

const Timestamp Timestamp::UNDEFINED((time_t) -1);
Timestamp Timestamp::startup= Timestamp::now();

Timestamp Timestamp::now()
{
	Timestamp ret;
	int r= clock_gettime(CLOCK_REALTIME_COARSE, & ret.t);
	if (r != 0) {
		should_not_happen();
		print_errno("clock_gettime(CLOCK_REALTIME_COARSE, ...)");
		/* Do the next best thing:  use time(2).  This may lead to clock
		 * skew, as the nanoseconds are not set correctly. */
		ret.t.tv_sec= time(nullptr);
		ret.t.tv_nsec= 0;
	}
	return ret;
}

#else

const Timestamp Timestamp::UNDEFINED((time_t) -1, true);
Timestamp Timestamp::startup= Timestamp::now();

#endif
