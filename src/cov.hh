#ifndef COV_HH
#define COV_HH

#ifdef STU_COV
extern "C" {
#include <gcov.h>
}
#else
#define __gcov_dump()
#endif

#define uncovered_due_to_bug_in_gcov()
/* https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83616
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83434 */

#endif /* ! COV_HH */
