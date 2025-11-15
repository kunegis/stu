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
	: is_finished(false)
{
	for (auto &d: deps) {
		push(d);
	}
}

Proceed Root_Executor::execute(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION();
	TRACE("is_finished= %s", frmt("%d", is_finished));
	Debug debug(this);

	Proceed proceed_A= execute_phase_A(dep_link);
	TRACE("proceed_A= %s", show(proceed_A));
	assert(is_valid(proceed_A));
	if (proceed_A & (P_WAIT | P_CALL_AGAIN)) {
		TRACE("Phase A wait / call again");
		return proceed_A;
	}
	assert(proceed_A == P_NOTHING);
	if (error) {
		TRACE("Phase A abort");
		is_finished= true;
		return P_NOTHING;
	}
	assert(get_buffer_A().empty());

	Proceed proceed_B= execute_phase_B(dep_link);
	TRACE("proceed_B= %s", show(proceed_B));
	assert(is_valid(proceed_B));
	if (proceed_B & (P_WAIT | P_CALL_AGAIN)) {
		TRACE("Phase B wait / call again");
		return proceed_B;
	}
	assert(proceed_B == P_NOTHING);
	is_finished= true;
	return P_NOTHING;
}
