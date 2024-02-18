#ifndef MAIN_LOOP_HH
#define MAIN_LOOP_HH

#include "dep.hh"

void main_loop(const std::vector <shared_ptr <const Dep> > &deps);
/* Throws ERROR_BUILD and ERROR_LOGICAL */

#endif /* ! MAIN_LOOP_HH */
