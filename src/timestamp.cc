#include "timestamp.hh"

#if USE_MTIM

const Timestamp Timestamp::UNDEFINED((time_t) -1);
Timestamp Timestamp::startup= Timestamp::now();

#else

const Timestamp Timestamp::UNDEFINED((time_t) -1, true);
Timestamp Timestamp::startup= Timestamp::now();

#endif
