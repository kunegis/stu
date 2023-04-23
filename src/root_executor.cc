#include "root_executor.hh"

bool Root_Executor::finished() const
{
	return is_finished;
}

bool Root_Executor::finished(Flags flags) const
{
	(void) flags;
	return is_finished;
}

Root_Executor::Root_Executor(const vector <shared_ptr <const Dep> > &deps)
	:  is_finished(false)
{
	for (auto &d:  deps)
		push(d);
}

Proceed Root_Executor::execute(shared_ptr <const Dep> dep_this)
{
	/* This is an example of a "plain" execute() function,
	 * containing the minimal wrapper around execute_base_?()  */

	Proceed proceed= execute_base_A(dep_this);
	assert(proceed);
	if (proceed & (P_WAIT | P_PENDING)) {
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}
	if (proceed & P_FINISHED) {
		is_finished= true;
		return proceed;
	}

	proceed |= execute_base_B(dep_this);
	if (proceed & (P_WAIT | P_PENDING)) {
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}
	if (proceed & P_FINISHED) {
		is_finished= true;
	}

	return proceed;
}
