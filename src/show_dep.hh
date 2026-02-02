#ifndef SHOW_DEP_HH
#define SHOW_DEP_HH

#ifndef NDEBUG

string show_trace(const shared_ptr <const Dep> &d);
string show_trace(const shared_ptr <Dep> &d);
string show_trace(const shared_ptr <const Dynamic_Dep> &d);
string show_trace(const shared_ptr <Dynamic_Dep> &d);
string show_trace(const shared_ptr <Concat_Dep> &d);

#endif /* ! NDEBUG */

#endif /* ! SHOW_DEP_HH */
