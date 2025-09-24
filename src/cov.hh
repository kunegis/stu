#ifndef COV_HH
#define COV_HH

#ifdef STU_COV
extern "C" {
#include <gcov.h>
}
#else
#define __gcov_dump()
#endif

#endif /* ! COV_HH */
