#include "dynamic_executor.hh"

Dynamic_Executor::Dynamic_Executor(
	shared_ptr <const Dynamic_Dep> dep_,
	Executor *parent,
	int &error_additional)
	: dep(dep_)
{
	TRACE_FUNCTION();
	assert(dep_);
	assert(dep_->is_normalized());
	assert(parent);
	assert(error_additional == 0);
	dep->check();

	/* Set the rule here, so cycles in the dependency graph can be detected.  Note
	 * however that the rule of dynamic executors is otherwise not used. */

	parents[parent]= dep;

	/* Find the rule of the inner dependency */
	shared_ptr <const Dep> inner_dep= dep->strip_dynamic();
	if (auto inner_plain_dep= to <const Plain_Dep> (inner_dep)) {
		Hash_Dep hash_dep_base(inner_plain_dep->place_target.flags,
			inner_plain_dep->place_target.place_name.unparametrized());
		Hash_Dep hash_dep= dep->get_target();
		TRACE("hash_dep= %s", show(hash_dep));
		try {
			std::map <string, string> mapping_parameter;
			shared_ptr <const Plain_Dep> target_plain_dep;
			shared_ptr <const Rule> rule=
				rule_set.get(hash_dep_base, param_rule, mapping_parameter,
					dep->get_place(), target_plain_dep);
		} catch (int e) {
			assert(e);
			*this << "";
			error_additional |= e;
			raise(e);
			return;
		}
		executors_by_hash_dep[hash_dep]= this;
	}

	parents.erase(parent);
	if (Cycle::find(parent, this, dep)) {
		TRACE("Found rule-level but not file-level cycle");
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}
	parents[parent]= dep;

	/* Push the single initial dependency */
	shared_ptr <Dep> dep_child= dep->dep->clone();
	dep_child->flags |= F_RESULT_NOTIFY;
	push(dep_child);

	if (dep->flags & F_PHASE_B) bits |= B_NEED_BUILD;
}

Proceed Dynamic_Executor::execute(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION(show_trace(dep_link));
	TRACE("done= %s; bits= %s", done.show(), show_bits(bits));
	bool need_build;

	Proceed proceed_A= execute_phase_A(dep_link);
	TRACE("proceed_A= %s", show(proceed_A));
	assert(is_valid(proceed_A));
	if (proceed_A) {
		return proceed_A;
	}
	assert(proceed_A == P_NOTHING);
	if (error) {
		done |= Done::from_flags(dep_link->flags);
		return P_NOTHING;
	}

	assert(get_buffer_A().empty());
	if (finished(dep_link->flags)) {
		return P_NOTHING;
	}

	need_build= (bits & B_NEED_BUILD) != 0 || dep_link->flags & F_PHASE_B;
	TRACE("need_build= %s", frmt("%d", need_build));
	if (! need_build) {
		done |= Done::from_flags(dep_link->flags);
		return P_NOTHING;
	}

	Proceed proceed_B= execute_phase_B(dep_link);
	TRACE("proceed_B= %s", show(proceed_B));
	assert(is_valid(proceed_B));
	if (proceed_B) {
		return proceed_B;
	}
	assert(proceed_B == P_NOTHING);
	done |= Done::from_flags(dep_link->flags);
	return P_NOTHING;
}

bool Dynamic_Executor::finished(Flags flags) const
{
	TRACE_FUNCTION();
	TRACE("flags= %s; done= %s", show_flags(flags, S_DEBUG), done.show());
	bool ret= done.is_done_from_flags(flags);
	TRACE("ret= %s", frmt("%d", ret));
	return ret;
}

bool Dynamic_Executor::want_delete() const
{
	return to <Plain_Dep> (dep->strip_dynamic()) == nullptr;
}

#ifndef NDEBUG
void Dynamic_Executor::render(Parts &parts, Rendering rendering) const
{
	dep->render(parts, rendering);
}
#endif /* ! NDEBUG */

void Dynamic_Executor::notify_variable(
	const std::map <string, string> &result_variable_child)
{
	result_variable.insert(result_variable_child.begin(),
		result_variable_child.end());
}

void Dynamic_Executor::notify_result(
	shared_ptr <const Dep> dep_result,
	Executor *source,
	Flags flags,
	shared_ptr <const Dep> dep_source)
{
	TRACE_FUNCTION(show_trace(dep));
	TRACE("dep_result= %s; flags= %s; dep_source= %s",
		show_trace(dep_result), show_flags(flags), show_trace(dep_source));
	assert(!(flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY)));
	assert((flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY))
	       != (F_RESULT_NOTIFY | F_RESULT_COPY));
	assert(dep_source);

	if (flags & F_RESULT_NOTIFY) {
		std::vector <shared_ptr <const Dep> > deps;
		source->read_dynamic(to <const Plain_Dep> (dep_result), deps, dep, this);
		for (auto &j: deps) {
			shared_ptr <Dep> j_new= j->clone();
			/* Add -% flag */
			j_new->flags |= F_RESULT_COPY;
			/* Add flags from self */
			j_new->flags |= dep->flags & (F_WORD & ~F_TARGET_DYNAMIC);
			for (unsigned i= 0; i < C_PLACED; ++i) {
				if (j_new->get_place_flag(i).empty() &&
				    ! dep->get_place_flag(i).empty())
					j_new->set_place_flag(i, dep->get_place_flag(i));
			}
			j= j_new;
			push(j);
		}
	} else if (flags & F_RESULT_COPY) {
		push_result(dep_result);
	} else {
		unreachable();
	}
}
