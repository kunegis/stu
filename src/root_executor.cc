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

Root_Executor::Root_Executor(const std::vector <shared_ptr <const Dep> > &deps)
	:  is_finished(false)
{
	for (auto &d: deps)
		push(d);
}

Proceed Root_Executor::execute(shared_ptr <const Dep> dep_link)
/* This is an example of a "plain" execute() function, containing the minimal wrapper
 * around execute_phase_{A,B}() */
{
	TRACE_FUNCTION();
	Debug debug(this);
	Proceed proceed= execute_phase_A(dep_link);
	TRACE("proceed= %s", show(proceed));
	assert(is_valid(proceed));
	if (proceed & P_ABORT) {
		is_finished= true;
		return proceed;
	}
	if (proceed & (P_WAIT | P_CALL_AGAIN)) {
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}
	assert(proceed & P_FINISHED);
	assert(get_buffer_A().empty());

	Proceed proceed_B= execute_phase_B(dep_link);
	TRACE("proceed_B= %s", show(proceed));
	if (proceed_B & (P_WAIT | P_CALL_AGAIN)) {
		assert((proceed_B & P_FINISHED) == 0);
		return proceed;
	}
	if (proceed_B & P_FINISHED) {
		is_finished= true;
	}

	return proceed;
}
