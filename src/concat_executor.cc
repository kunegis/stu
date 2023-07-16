#include "concat_executor.hh"

Concat_Executor::Concat_Executor(shared_ptr <const Concat_Dep> dep_,
				 Executor *parent,
				 int &error_additional)
	:  dep(dep_),
	   stage(ST_DYNAMIC)
{
	assert(dep_);
	assert(dep_->is_normalized());
	assert(dep->is_normalized());
	assert(parent);
	dep->check();

	parents[parent]= dep;
	if (error_additional) {
		*this << "";
		stage= ST_FINISHED;
		parents.erase(parent);
		raise(error_additional);
		return;
	}

	parents.erase(parent);
	if (find_cycle(parent, this, dep)) {
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}
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
			shared_ptr <Dep> dep_child= Dep::clone(dynamic_d->dep);
			dep_child->flags |= F_RESULT_NOTIFY;
			dep_child->index= i;
			push(dep_child);
		} else {
			/* Everything else would mean that DEP
			 * was not normalized  */
			unreachable();
		}
		++i;
	}
}

Proceed Concat_Executor::execute(shared_ptr <const Dep> dep_this)
{
 again:
	assert(stage <= ST_FINISHED);
	if (stage == ST_FINISHED)
		return P_FINISHED;
	Proceed proceed= execute_base_A(dep_this);
	assert(proceed);
	if (proceed & (P_WAIT | P_PENDING)) {
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}
	if (!(proceed & P_FINISHED)) {
		proceed |= execute_base_B(dep_this);
		if (proceed & (P_WAIT | P_PENDING)) {
			assert((proceed & P_FINISHED) == 0);
			return proceed;
		}
	}
	if (proceed & P_FINISHED) {
		++stage;
		assert(stage <= ST_FINISHED);
		if (stage == ST_FINISHED) {
			return proceed;
		} else {
			assert(stage == ST_NORMAL);
			launch_stage_1();
			goto again;
		}
	}

	return proceed;
}

bool Concat_Executor::finished() const
{
	assert(stage <= ST_FINISHED);
	return stage == ST_FINISHED;
}

bool Concat_Executor::finished(Flags) const
/* Since Concat_Executor objects are used just once, by a single parent, this
 * always returns the same as finished() itself.  */
{
	return finished();
}

void Concat_Executor::launch_stage_1()
{
	shared_ptr <Concat_Dep> c= std::make_shared <Concat_Dep> ();
	c->deps.resize(collected.size());
	for (size_t i= 0; i < collected.size(); ++i) {
		c->deps.at(i)= move(collected.at(i));
	}
	std::vector <shared_ptr <const Dep> > deps;
	int e= 0;
	Dep::normalize(c, deps, e);
	if (e) {
		*this << "";
		raise(e);
	}

	for (auto f: deps) {
		shared_ptr <Dep> f2= Dep::clone(f);
		/* Add -% flag */
		f2->flags |= F_RESULT_COPY;
		/* Add flags from self */
		f2->flags |= dep->flags & (F_TARGET_BYTE & ~F_TARGET_DYNAMIC);
		for (unsigned i= 0; i < C_PLACED; ++i) {
			if (f2->get_place_flag(i).empty() && ! dep->get_place_flag(i).empty())
				f2->set_place_flag(i, dep->get_place_flag(i));
		}
		push(f2);
	}
}

void Concat_Executor::notify_result(shared_ptr <const Dep> d,
				    Executor *source,
				    Flags flags,
				    shared_ptr <const Dep> dep_source)
{
	assert(!(flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY)));
	assert((flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY))
	       != (F_RESULT_NOTIFY | F_RESULT_COPY));
	assert(dep_source);
	DEBUG_PRINT(fmt("notify_result(flags = %s, d = %s)",
			show_flags(flags, S_DEBUG),
			::show(d, S_DEBUG, R_SHOW_FLAGS)));

	if (flags & F_RESULT_NOTIFY) {
		std::vector <shared_ptr <const Dep> > deps;
		source->read_dynamic(to <const Plain_Dep> (d), deps, dep, this);
		for (auto &j: deps) {
			size_t i= dep_source->index;
			collected.at(i)->deps.push_back(j);
		}
	} else {
		// TODOhave else if instead and unreachable().
		assert(flags & F_RESULT_COPY);
		push_result(d);
	}
}
