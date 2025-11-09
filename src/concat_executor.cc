#include "concat_executor.hh"

#include "trace.hh"
#include "trace_dep.hh"

Concat_Executor::Concat_Executor(
	shared_ptr <const Concat_Dep> dep_,
	Executor *parent)
	: dep(dep_), stage(ST_DYNAMIC)
{
	TRACE_FUNCTION();
	TRACE("dep_= %s", show_trace(dep_));
	assert(dep);
	assert(dep->is_normalized());
	assert(parent);
	dep->check();

	parents[parent]= dep;

	/* Initialize COLLECTED */
	size_t k= dep_->deps.size();
	collected.resize(k);
	for (size_t i= 0; i < k; ++i) {
		collected.at(i)= std::make_shared <Compound_Dep> (Place::place_empty);
	}

	/* Push initial dependencies */
	size_t i= 0;
	for (auto d: dep->deps) {
		if (auto plain_d= to <const Plain_Dep> (d)) {
			collected.at(i)->deps.push_back(d);
		} else if (auto dynamic_d= to <const Dynamic_Dep> (d)) {
			/* This is the only place where Dependency::index is set to
			 * another value than -1. */
			shared_ptr <Dep> dep_child= dynamic_d->dep->clone();
			dep_child->flags |= F_RESULT_NOTIFY;
			dep_child->index= i;
			push_normalized(dep_child);
		} else {
			/* Everything else would mean that DEP was not normalized */
			unreachable();
		}
		++i;
	}
}

Proceed Concat_Executor::execute(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION(show_trace(dep_link));
	Debug debug(this);
 again:
	TRACE("stage= %s", frmt("%u", stage));

	/* STAGE cannot be ST_FINISHED because concat executors are not cached. */
	assert(stage < ST_FINISHED);

	Proceed proceed_A= execute_phase_A(dep_link);
	assert(is_valid(proceed_A));
	if (proceed_A & (P_WAIT | P_CALL_AGAIN)) {
		TRACE("phase A wait/call again");
		return proceed_A;
	}
	assert(proceed_A == P_FINISHED);
	if (error) {
		TRACE("phase A aborted");
		stage= ST_FINISHED;
		return P_FINISHED;
	}

	Proceed proceed_B= execute_phase_B(dep_link);
	assert(is_valid(proceed_B));
	if (proceed_B & (P_WAIT | P_CALL_AGAIN)) {
		TRACE("phase B wait/call again");
		return proceed_B;
	}
	assert(proceed_B == P_FINISHED);
	if (error) {
		TRACE("phase B aborted");
		stage= ST_FINISHED;
		return P_FINISHED;
	}

	assert(stage < ST_FINISHED);
	if (stage == ST_NORMAL) {
		++stage;
		TRACE("finished");
		return proceed_B;
	} else if (stage == ST_DYNAMIC) {
		launch_stage_normal();
		goto again;
	}
	unreachable();
}

bool Concat_Executor::finished() const
{
	assert(stage <= ST_FINISHED);
	return stage == ST_FINISHED;
}

bool Concat_Executor::finished(Flags) const
/* Since Concat_Executor objects are used just once, by a single parent, this
 * always returns the same as finished() itself. */
{
	return finished();
}

void Concat_Executor::notify_variable(
	const std::map <string, string> &result_variable_child)
{
	result_variable.insert(result_variable_child.begin(),
		result_variable_child.end());
}

void Concat_Executor::launch_stage_normal()
{
	TRACE_FUNCTION();
	assert(stage == ST_DYNAMIC);
	++stage;
	assert(stage == ST_NORMAL);
	shared_ptr <Concat_Dep> c= std::make_shared <Concat_Dep> ();
	TRACE("collected.size()= %s", frmt("%zu", collected.size()));
	c->deps.resize(collected.size());
	for (size_t i= 0; i < collected.size(); ++i) {
		TRACE("collected[%s]= %s", frmt("%zu", i), show(collected[i]));
		c->deps.at(i)= move(collected[i]);
	}
	TRACE("c= %s", show(c, S_DEBUG, R_SHOW_FLAGS));
	std::vector <shared_ptr <const Dep> > deps;
	int e= 0;
	c->normalize(deps, e);
	if (e) {
		*this << "";
		raise(e);
	}
	TRACE("deps.size()= %s", frmt("%zu", deps.size()));

	for (auto f: deps) {
		shared_ptr <Dep> f2= f->clone();
		/* Add -% flag */
		f2->flags |= F_RESULT_COPY;
		/* Add flags from self */
		f2->flags |= dep->flags & (F_WORD & ~F_TARGET_DYNAMIC);
		for (unsigned i= 0; i < C_PLACED; ++i) {
			if (f2->get_place_flag(i).empty()
				&& ! dep->get_place_flag(i).empty())
			{
				f2->set_place_flag(i, dep->get_place_flag(i));
			}
		}
		push(f2);
	}
}

void Concat_Executor::notify_result(
	shared_ptr <const Dep> dep_result,
	Executor *source,
	Flags flags,
	shared_ptr <const Dep> dep_source)
{
	assert(source);
	assert(!(flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY)));
	assert((flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY))
		!= (F_RESULT_NOTIFY | F_RESULT_COPY));
	assert(dep_source);
	TRACE_FUNCTION(show_trace(dep));
	TRACE("dep_result= %s", show_trace(dep_result));
	TRACE("source= %s", show(*source));
	TRACE("flags= %s", show_flags(flags));
	TRACE("i= %s", frmt("%zd", dep_source->index));
	DEBUG_PRINT(fmt("notify_result(flags = %s, d = %s)",
			show_flags(flags, S_DEBUG),
			::show(dep_result, S_DEBUG, R_SHOW_FLAGS)));

	if (flags & F_RESULT_NOTIFY) {
		std::vector <shared_ptr <const Dep> > deps;
		source->read_dynamic(to <const Plain_Dep> (dep_result), deps, dep, this);
		for (auto &j: deps) {
			size_t i= dep_source->index;
			collected.at(i)->deps.push_back(j);
		}
	} else if (flags & F_RESULT_COPY) {
		push_result(dep_result);
	} else {
		unreachable();
	}
}
