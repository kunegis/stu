#include "executor.hh"

#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "concat_executor.hh"
#include "dynamic_executor.hh"
#include "explain.hh"
#include "file_executor.hh"
#include "parser.hh"
#include "root_executor.hh"
#include "tokenizer.hh"
#include "transient_executor.hh"

Rule_Set Executor::rule_set;

Timestamp Executor::timestamp_last;
/* Initialized to zero, i.e., older than the current date */

bool Executor::hide_out_message= false;
bool Executor::out_message_done= false;
std::unordered_map <Hash_Dep, Executor *> Executor::executors_by_hash_dep;

void Executor::read_dynamic(shared_ptr <const Plain_Dep> dep_target,
			    std::vector <shared_ptr <const Dep> > &deps,
			    shared_ptr <const Dep> dep,
			    Executor *dynamic_executor)
{
	DEBUG_PRINT(fmt("read_dynamic %s", show(dep_target, S_DEBUG, R_SHOW_FLAGS)));

	try {
		const Place_Target &place_target=
			to <Plain_Dep> (dep_target)->place_target;
		assert(place_target.place_name.get_n() == 0);
		const Hash_Dep hash_dep= place_target.unparametrized();
		assert(deps.empty());

		/* Check:  variable dependencies are not allowed in multiply
		 * dynamic dependencies.  */
		if (dep_target->flags & F_VARIABLE) {
			dep_target->get_place() <<
				fmt("variable dependency %s must not appear",
				    ::show(dep_target));
			*this << fmt("within multiply-dynamic dependency %s",
				     ::show(dep));
			raise(ERROR_LOGICAL);
		}
		if (place_target.flags & F_TARGET_TRANSIENT)
			return;

		assert(hash_dep.is_file());
		string filename= hash_dep.get_name_nondynamic();

		bool delim= (dep_target->flags & (F_NEWLINE_SEPARATED | F_NUL_SEPARATED));
		/* Whether the dynamic dependency is delimiter-separated */

		bool allow_enoent= dep_target->flags & (F_OPTIONAL | F_TRIVIAL);

		if (! delim) {
			/* Dynamic dependency in full Stu syntax */
			std::vector <shared_ptr <Token> > tokens;
			Place place_end;

			Tokenizer::parse_tokens_file
				(tokens, Tokenizer::DYNAMIC, place_end, filename,
				 place_target.place, -1, allow_enoent);

			Place_Name input; /* remains empty */
			Place place_input; /* remains empty */

			try {
				Parser::get_expression_list(deps, tokens,
							    place_end, input, place_input);
			} catch (int e) {
				raise(e);
				goto end_normal;
			}

			/* Check that there are no input dependencies */
			if (! input.empty()) {
				Hash_Dep hash_dep_dynamic(0, hash_dep);
				place_input <<
					fmt("dynamic dependency %s must not contain input redirection %s",
					    show(hash_dep_dynamic),
					    show_prefix("<", input));
				Hash_Dep hash_dep_file= hash_dep;
				hash_dep_file.get_front_word_nondynamic() &= ~F_TARGET_TRANSIENT;
				(*dynamic_executor) << fmt("%s is declared here",
							   show(hash_dep_file));
				raise(ERROR_LOGICAL);
			}
		end_normal:;

		} else {
			/* Delimiter-separated dynamic dependency (-n/-0) */
			const char c= (dep_target->flags & F_NEWLINE_SEPARATED) ? '\n' : '\0';
			const char c_printed= (dep_target->flags & F_NEWLINE_SEPARATED) ? 'n' : '0';
			try {
				Parser::get_expression_list_delim
					(deps, filename.c_str(), c, c_printed,
					 *dynamic_executor, allow_enoent);
			} catch (int e) {
				raise(e);
			}
		}

		/* Forbidden features in dynamic dependencies.
		 * In keep-going mode (-k), we set the error, set the erroneous
		 * dependency to null, and at the end prune the null entries.  */
		bool found_error= false;
		if (! delim)  for (auto &j: deps) {
			/* Check that it is unparametrized */
			if (! j->is_unparametrized()) {
				shared_ptr <const Dep> depp= j;
				while (to <Dynamic_Dep> (depp)) {
					shared_ptr <const Dynamic_Dep> depp2=
						to <Dynamic_Dep> (depp);
					depp= depp2->dep;
				}
				to <Plain_Dep> (depp)
					->place_target.place_name.places[0] <<
					fmt("dynamic dependency %s must not contain parametrized dependencies",
					    show(Hash_Dep(0, hash_dep)));
				Hash_Dep hash_dep_base= hash_dep;
				hash_dep_base.get_front_word_nondynamic()
					&= ~F_TARGET_TRANSIENT;
				hash_dep_base.get_front_word_nondynamic()
					|= (hash_dep.get_front_word_nondynamic() & F_TARGET_TRANSIENT);
				*this << fmt("%s is declared here",
					     show(hash_dep_base));
				raise(ERROR_LOGICAL);
				j= nullptr;
				found_error= true;
				continue;
			}
		}

		assert(! found_error || option_k);

		shared_ptr <const Dep> top_top= dep_target->top;
		shared_ptr <Dep> no_top= Dep::clone(dep_target);
		no_top->top= nullptr;
		shared_ptr <Dep> top= std::make_shared <Dynamic_Dep> (no_top);
		top->top= top_top;

		std::vector <shared_ptr <const Dep> > deps_new;
		for (auto &j: deps) {
			if (j) {
				shared_ptr <Dep> j_new= Dep::clone(j);
				j_new->top= top;
				deps_new.push_back(j_new);
			}
		}
		swap(deps, deps_new);
	} catch (int e) {
		dynamic_executor->raise(e);
	}
}

bool Executor::find_cycle(Executor *parent,
			  Executor *child,
			  shared_ptr <const Dep> dep_link)
{
	std::vector <Executor *> path;
	path.push_back(parent);
	return find_cycle(path, child, dep_link);
}

bool Executor::find_cycle(std::vector <Executor *> &path,
			  Executor *child,
			  shared_ptr <const Dep> dep_link)
{
	if (same_rule(path.back(), child)) {
		cycle_print(path, dep_link);
		return true;
	}
	for (auto &i: path.back()->parents) {
		Executor *next= i.first;
		assert(next != nullptr);
		path.push_back(next);
		bool found= find_cycle(path, child, dep_link);
		if (found)
			return true;
		path.pop_back();
	}
	return false;
}

void Executor::cycle_print(const std::vector <Executor *> &path,
			   shared_ptr <const Dep> dep)
/*
 * Given PATH = [a, b, c, d, ..., x], we print:
 *
 *	x depends on ...      \
 *      ... depends on d      |
 *      d depends on c        | printed from PATH
 *      c depends on b        |
 *      b depends on a        /
 *      a depends on x        > printed from DEP
 *      x is needed by ...    \
 *      ...                   | printed by Backtrace
 *      ...                   /
 */
{
	assert(path.size() > 0);

	std::vector <string> names;
	/* Indexes are parallel to PATH */
	names.resize(path.size());

	for (size_t i= 0; i + 1 < path.size(); ++i)
		names[i]= ::show(path[i]->parents.at(path[i+1]));
	names.back()= ::show(path.back()->parents.begin()->second);

	for (ssize_t i= path.size() - 1; i >= 0; --i) {
		shared_ptr <const Dep> d= i == 0 ? dep :
			path[i - 1]->parents.at(const_cast <Executor *> (path[i]));

		/* Don't show a message for left-branch dynamic links */
		if (hide_link_from_message(d->flags))
			continue;

		d->get_place() << fmt(
			"%s%s depends on %s",
			i == (ssize_t)(path.size() - 1)
			? (path.size() == 1
				|| (path.size() == 2 && hide_link_from_message(dep->flags))
				? "target must not depend on itself: "
				: "cyclic dependency: ")
			: "",
			names[i],
			i == 0 ? ::show(dep) : names[i - 1]);
	}

	/* If the two targets are different (but have the same rule because they match the
	 * same pattern and/or because they are two different targets of a multitarget
	 * rule), then output a notice to that effect. */
	Hash_Dep t1= path.back()->parents.begin()->second->get_target();
	Hash_Dep t2= dep->get_target();
	const char *c1= t1.get_name_c_str_any();
	const char *c2= t2.get_name_c_str_any();
	if (strcmp(c1, c2)) {
		path.back()->get_place()
			<< fmt("both %s and %s match the same rule",
			       ::show(c1), ::show(c2));
	}

	/* Remove the offending (cycle-generating) link between the two.  The offending
	 * link is from path[0] as a parent to path[end] (as a child). */
	path.back()->parents.erase(path.at(0));
	path.at(0)->children.erase(path.back());

	*path.back() << "";
	explain_cycle();
}

Executor *Executor::get_executor(shared_ptr <const Dep> dep)
{
	TRACE_FUNCTION();
	TRACE("{%s} dep= %s", show(*this, S_DEBUG, R_SHOW_FLAGS),
	      show(dep, S_DEBUG, R_SHOW_FLAGS));
	
	/*
	 * Non-cached executors
	 */

	/* Concatenations */
	if (shared_ptr <const Concat_Dep> concat_dep= to <const Concat_Dep> (dep)) {
		int error_additional= 0;
		Concat_Executor *executor= new Concat_Executor
			(concat_dep, this, error_additional);
		assert(executor);
		if (error_additional) {
			error |= error_additional;
			assert(executor->want_delete());
			delete executor;
			return nullptr;
		}
		return executor;
	}

	/* Dynamics that are not cached (with concatenations somewhere inside) */
	if (to <const Dynamic_Dep> (dep)
	    && ! to <const Plain_Dep> (Dep::strip_dynamic(dep))) {
		int error_additional= 0;
		Dynamic_Executor *executor= new Dynamic_Executor
			(to <const Dynamic_Dep> (dep), this, error_additional);
		assert(executor);
		if (error_additional) {
			error |= error_additional;
			assert(executor->want_delete());
			delete executor;
			return nullptr;
		}
		return executor;
	}

	/*
	 * Cached executors
	 */

	const Hash_Dep hash_dep= dep->get_target();
	Executor *executor= nullptr;
	const Hash_Dep target_for_cache= get_target_for_cache(hash_dep);
	auto it= executors_by_hash_dep.find(target_for_cache);

	if (it != executors_by_hash_dep.end()) {
		/* An Executor object already exists for the target */
		TRACE("%s", "already exists");
		executor= it->second;
		if (executor->parents.count(this)) {
			/* THIS and CHILD are already connected -- add the necessary
			 * flags */
			Flags flags= dep->flags;
			if (flags & ~executor->parents.at(this)->flags) {
				shared_ptr <Dep> dep_new=
					Dep::clone(executor->parents.at(this));
				dep_new->flags |= flags;
				dep= dep_new;
				/* No need to check for cycles here, because a link
				 * between the two already exists and therefore a cycle
				 * cannot be present. */
				executor->parents[this]= dep;
			}
		} else {
			if (find_cycle(this, executor, dep)) {
				raise(ERROR_LOGICAL);
				return nullptr;
			}
			/* The parent and child are not connected -- add the connection */
			executor->parents[this]= dep;
		}
		TRACE("%s", "returning existing executor");
		return executor;
	}

	/* Create a new Executor object */

	int error_additional= 0; /* Passed to the executor */
	if (! hash_dep.is_dynamic()) {
		/* Plain executor */
		shared_ptr <const Rule> rule_child, param_rule_child;
		std::map <string, string> mapping_parameter;
		bool use_file_executor= false;
		try {
			Hash_Dep hash_dep_without_flags= hash_dep;
			hash_dep_without_flags.get_front_word_nondynamic()
				&= F_TARGET_TRANSIENT;
			rule_child= rule_set.get(hash_dep_without_flags,
						 param_rule_child, mapping_parameter,
						 dep->get_place());
		} catch (int e) {
			assert(e);
			error_additional= e;
		}
		assert((rule_child == nullptr) == (param_rule_child == nullptr));

		/* RULE_CHILD may be null here; this is handled in the constructors */
		/* We use a File_Executor if:  there is at least one file
		 * target in the rule OR there is a command in the rule.  When
		 * there is no rule, we consult the type of TARGET.  */

		if (hash_dep.is_file()) {
			use_file_executor= true;
		} else if (rule_child == nullptr) {
			use_file_executor= false;
		} else if (rule_child->command) {
			use_file_executor= true;
		} else {
			for (auto &i: rule_child->place_targets) {
				if ((i->flags & F_TARGET_TRANSIENT) == 0)
					use_file_executor= true;
			}
		}

		if (use_file_executor) {
			executor= new File_Executor
				(dep, this, rule_child,
				 param_rule_child, mapping_parameter,
				 error_additional);
		} else if (hash_dep.is_transient()) {
			executor= new Transient_Executor
				(dep, this,
				 rule_child, param_rule_child, mapping_parameter,
				 error_additional);
		}
	} else {
		shared_ptr <const Dynamic_Dep> dynamic_dep= to <Dynamic_Dep> (dep);
		executor= new Dynamic_Executor(dynamic_dep, this, error_additional);
	}

	if (error_additional) {
		error |= error_additional;
		if (executor->want_delete())
			delete executor;
		return nullptr;
	}
	assert(executor->parents.size() == 1);
	TRACE("%s", "returning new executor");
	return executor;
}

bool Executor::same_rule(const Executor *executor_a,
			 const Executor *executor_b)
/* This must also take into account that two executors could use the
 * same rule but parametrized differently, thus the two executors could
 * have different targets, but the same rule. */
{
	return
		executor_a->param_rule != nullptr &&
		executor_b->param_rule != nullptr &&
		executor_a->get_depth() == executor_b->get_depth() &&
		executor_a->param_rule == executor_b->param_rule;
}

void Executor::operator<<(string text) const
/* The following traverses the executor graph backwards until it finds the root.  We
 * always take the first found parent, which is an arbitrary choice, but it doesn't matter
 * here which dependency path we point out as an error, so the first one it is. */
{
	/* If the error happens directly for the root executor, it was
	 * an error on the command line; don't output anything beyond
	 * the error message itself, which was already output.  */
	if (dynamic_cast <const Root_Executor *> (this))
		return;

	bool first= true;

	/* If there is a rule for this target, show the message with the
	 * rule's trace, otherwise show the message with the first
	 * dependency trace */
	if (this->get_place().type != Place::Type::EMPTY && ! text.empty()) {
		this->get_place() << text;
		first= false;
	}

	const Executor *executor= this->parents.begin()->first;
	shared_ptr <const Dep> depp= this->parents.begin()->second;
	string text_parent= show(depp);

	while (true) {
		if (dynamic_cast <const Root_Executor *> (executor)) {
			/* We are in a child of the root executor */
			assert(! depp->top);
			if (first && ! text.empty()) {
				/* No text was printed yet, but there was a TEXT passed:
				 * Print it with the place available. */
				/* This is a top-level target, i.e., passed on the command
				 * line via an argument or an option */
				depp->get_place() << text;
			}
			break;
		}

		/* Increment */
		shared_ptr <const Dep> depp_old= depp;
		if (! depp->top) {
			/* Assign DEPP first, because we change EXECUTOR */
			depp= executor->parents.begin()->second;
			executor= executor->parents.begin()->first;
		} else {
			depp= depp->top;
		}

		/* New text */
		string text_child= text_parent;
		text_parent= show(depp);

		/* Don't show left-branch edges of dynamic executors */
		if (hide_link_from_message(depp_old->flags)) {
			continue;
		}

		if (same_dependency_for_print(depp, depp_old)) {
			continue;
		}

		/* Print */
		string msg;
		if (first && ! text.empty()) {
			msg= fmt("%s, needed by %s", text, text_parent);
			first= false;
		} else {
			msg= fmt("%s is needed by %s",
				 text_child, text_parent);
		}
		depp_old->get_place() << msg;
	}
}

Proceed Executor::execute_children()
{
	TRACE_FUNCTION();
	TRACE("{%s}", show(*this, S_DEBUG, R_SHOW_FLAGS));

	/* Since disconnect() may change executor->children, we must first
	 * copy it over locally, and then iterate through it */
	std::vector <Executor *> executors_children_vector
		(children.begin(), children.end());
	Proceed proceed_all= 0;

	while (! executors_children_vector.empty()) {
		assert(options_jobs >= 0);
		if (order_vec) {
			/* Exchange a random position with last position */
			size_t p_last= executors_children_vector.size() - 1;
			size_t p_random= random_number(executors_children_vector.size());
			if (p_last != p_random) {
				std::swap(executors_children_vector[p_last],
					  executors_children_vector[p_random]);
			}
		}

		Executor *child= executors_children_vector.at
			(executors_children_vector.size() - 1);
		executors_children_vector.resize(executors_children_vector.size() - 1);
		assert(child != nullptr);

		shared_ptr <const Dep> dep_child= child->parents.at(this);

		Proceed proceed_child= child->execute(dep_child);
		assert(proceed_child);

		proceed_all |= (proceed_child & ~(P_FINISHED | P_ABORT));
		/* The finished and abort flags of the child only apply to the
		 * child, not to us. */

		assert(((proceed_child & P_FINISHED) == 0) ==
		       ((child->finished(dep_child->flags)) == 0));

		if (proceed_child & P_FINISHED) {
			disconnect(child, dep_child);
		} else {
			assert((proceed_child & ~P_FINISHED) != 0);
			/* If the child executor is not finished, it must have returned
			 * either the P_WAIT or P_PENDING bit. */
		}
	}

	if (error) {
		assert(option_k);
	}

	if (proceed_all == 0) {
		/* If there are still children, they must have returned WAIT or PENDING. */
		assert(children.empty());
		if (error) {
			assert(option_k);
		}
	}

	TRACE("proceed_all= %s", show_proceed(proceed_all));
	return proceed_all;
}

void Executor::push(shared_ptr <const Dep> dep)
{
	TRACE_FUNCTION();
	TRACE("{%s} dep= %s", show(*this, S_DEBUG, R_SHOW_FLAGS),
	      show(dep, S_DEBUG, R_SHOW_FLAGS));
	dep->check();
	DEBUG_PRINT(fmt("push %s", show(dep, S_DEBUG, R_SHOW_FLAGS)));

	std::vector <shared_ptr <const Dep> > deps;
	int e= 0;
	Dep::normalize(dep, deps, e);
	if (e) {
		dep->get_place() <<
			fmt("%s is needed by %s", show(dep), show(parents.begin()->second));
		*this << "";
		raise(e);
	}
	for (const auto &d: deps) {
		TRACE("d= %s", show(d, S_DEBUG, R_SHOW_FLAGS));
		d->check();
		assert(d->is_normalized());
		shared_ptr <const Dep> untrivialized= Dep::untrivialize(d);
		TRACE("untrivialized= %s", untrivialized ?
			show(untrivialized, S_DEBUG, R_SHOW_FLAGS) : "<null>");
		if (untrivialized) {
			shared_ptr <Dep> d2= Dep::clone(untrivialized);
			d2->flags |= F_PHASE_B;
			TRACE("buffer_B push %s", show(d2, S_DEBUG, R_SHOW_FLAGS));
			buffer_B.push(d2);
		} else {
			TRACE("buffer_A push %s", show(d, S_DEBUG, R_SHOW_FLAGS));
			buffer_A.push(d);
		}
	}
}

void Executor::raise(int e)
{
	assert(e >= 1 && e <= 3);
	error |= e;
	if (! option_k)
		throw error;
}

void Executor::disconnect(
	Executor *const child,
	shared_ptr <const Dep> dep_child)
{
	TRACE_FUNCTION();
	TRACE("{%s} dep_child= %s", show(*this, S_DEBUG, R_SHOW_FLAGS),
		show(dep_child, S_DEBUG, R_SHOW_FLAGS));
	DEBUG_PRINT(fmt("disconnect %s", show(dep_child, S_DEBUG, R_SHOW_FLAGS)));
	assert(child);
	assert(child != this);
	assert(child->finished(dep_child->flags));
	assert(option_k || child->error == 0);
	dep_child->check();

	if (dep_child->flags & F_RESULT_NOTIFY && dynamic_cast <File_Executor *> (child)) {
		shared_ptr <Dep> d= Dep::clone(dep_child);
		d->flags &= ~F_RESULT_NOTIFY;
		notify_result(d, child, F_RESULT_NOTIFY, dep_child);
	}

	if (dep_child->flags & F_RESULT_COPY && dynamic_cast <File_Executor *> (child)) {
		shared_ptr <Dep> d= Dep::clone(dep_child);
		d->flags &= ~F_RESULT_COPY;
		notify_result(d, child, F_RESULT_COPY, dep_child);
	}

	/* Child is done for phase A, but not for phase B */
	bool child_was_phase_B= dep_child->flags & F_PHASE_B;
	TRACE("child_was_phase_B= %s", frmt("%d", child_was_phase_B));
	if (! child_was_phase_B) {
		bool child_finished_for_B= child->finished(dep_child->flags | F_PHASE_B);
		TRACE("child_finished_for_B= %s", frmt("%d", child_finished_for_B));
		if (! child_finished_for_B) {
			shared_ptr <Dep> d= Dep::clone(dep_child);
			d->flags |= F_PHASE_B;
			TRACE("d= %s", show(d, S_DEBUG, R_SHOW_FLAGS));
			DEBUG_PRINT(fmt("disconnect A_to_B %s",
					show(d, S_DEBUG, R_SHOW_FLAGS)));
			buffer_B.push(d);
		}
	}
	
	/* Propagate timestamp */
	/* Don't propagate the timestamp of the dynamic dependency itself */
	if (! (dep_child->flags & F_PERSISTENT) &&
	    ! (dep_child->flags & F_RESULT_NOTIFY)) {
		if (child->timestamp.defined()) {
			if (! timestamp.defined()) {
				timestamp= child->timestamp;
			} else if (timestamp < child->timestamp) {
				timestamp= child->timestamp;
			}
		}
	}

	/* Propagate variables */
	if ((dep_child->flags & F_VARIABLE)) {
		assert(dynamic_cast <File_Executor *> (child));
		dynamic_cast <File_Executor *> (child)->read_variable(dep_child);
	}
	if (! child->result_variable.empty()) {
		notify_variable(child->result_variable);
	}

	/*
	 * Propagate attributes
	 */

	/* Propagate the flags after propagating other things, since flags can be
	 * changed by the propagations done before. */

	error |= child->error;

	/* Don't propagate the NEED_BUILD flag via F_RESULT_NOTIFY links: It just means the
	 * list of depenencies have changed, not the dependencies themselves. */
	if (child->bits & B_NEED_BUILD && ! (dep_child->flags & F_RESULT_NOTIFY)) {
		bits |= B_NEED_BUILD;
	}

	/* Remove the links between them */
	assert(children.count(child) == 1);
	assert(child->parents.count(this) == 1);
	children.erase(child);
	child->parents.erase(this);

	if (child->want_delete()) delete child;
}

Proceed Executor::execute_phase_A(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION();
	TRACE("{%s}", show(*this, S_DEBUG, R_SHOW_FLAGS));
	assert(options_jobs >= 0);
	assert(dep_link);
	DEBUG_PRINT("phase_A");
	Proceed proceed= 0;

	if (finished(dep_link->flags)) {
		DEBUG_PRINT("finished");
		proceed |= P_FINISHED;
		goto ret;
	}

	if (optional_finished(dep_link)) {
		DEBUG_PRINT("optional finished");
		proceed |= P_FINISHED;
		goto ret;
	}

	/* Continue the already-active child executors.  In DFS mode, first
	 * continue the already-open children, then open new children.  In
	 * random mode, start new children first and continue already-open
	 * children second. */
	if (order != Order::RANDOM) {
		Proceed proceed_2= execute_children();
		proceed |= proceed_2;
		if (proceed & P_WAIT) {
			if (options_jobs == 0) {
				goto ret;
			}
		} else if (finished(dep_link->flags) && ! option_k) {
			DEBUG_PRINT("finished");
			proceed |= P_FINISHED;
			goto ret;
		}
	}

	assert(error == 0 || option_k);

	if (options_jobs == 0) {
		proceed |= P_WAIT;
		goto ret;
	}

	while (! buffer_A.empty()) {
		shared_ptr <const Dep> dep_child= buffer_A.pop();
		Proceed proceed_2= connect(dep_link, dep_child);
		proceed |= proceed_2;
		if (options_jobs == 0) {
			proceed |= P_WAIT;
			goto ret;
		}
	}
	assert(buffer_A.empty());

	if (order == Order::RANDOM) {
		Proceed proceed_2= execute_children();
		proceed |= proceed_2;
		if (proceed & P_WAIT) {
			goto ret;
		}
	}

	/* Some dependencies are still running */
	if (! children.empty()) {
		assert(proceed != 0);
		goto ret;
	}

	if (error) {
		assert(option_k);
		proceed |= P_ABORT | P_FINISHED;
		goto ret;
	}

	if (proceed)
		goto ret;

	proceed |= P_FINISHED;

 ret:
	TRACE("proceed= %s", show_proceed(proceed));
	return proceed;
}

Proceed Executor::execute_phase_B(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION();
	TRACE("{%s}", show(*this, S_DEBUG, R_SHOW_FLAGS));
	DEBUG_PRINT("phase_B");
	assert(buffer_A.empty());
	Proceed proceed= 0;
	while (! (buffer_A.empty() && buffer_B.empty())) {
//	while (! buffer_B.empty()) {

		if (! buffer_A.empty()) {
			Proceed proceed_2= execute_phase_A(dep_link);
			if (proceed_2 & (P_WAIT | P_ABORT)) {
				proceed |= proceed_2;
				assert(!(proceed & P_FINISHED));
				goto ret;
			}
			proceed |= proceed_2;
			proceed &= ~P_FINISHED;
		}
		
		if (! buffer_B.empty()) {
			shared_ptr <const Dep> dep_child= buffer_B.pop();
			Proceed proceed_2= connect(dep_link, dep_child);
			proceed |= proceed_2;
			assert(options_jobs >= 0);
			if (options_jobs == 0) {
				proceed |= P_WAIT;
				goto ret;
			}
		}
	}
	assert(buffer_A.empty());
	assert(buffer_B.empty());
	proceed |= P_FINISHED;
 ret:
	TRACE("proceed= %s", show_proceed(proceed));
	return proceed;
}

void Executor::push_result(shared_ptr <const Dep> dd)
{
	DEBUG_PRINT(fmt("push_result %s", show(dd, S_DEBUG, R_SHOW_FLAGS)));

	assert(! dynamic_cast <File_Executor *> (this));
	assert(! (dd->flags & F_RESULT_NOTIFY));
	dd->check();

	/* Add to own */
	result.push_back(dd);

	/* Notify parents */
	for (auto &i: parents) {
		Flags flags= i.second->flags & (F_RESULT_NOTIFY | F_RESULT_COPY);
		if (flags) {
			i.first->notify_result(dd, this, flags, i.second);
		}
	}
}

Hash_Dep Executor::get_target_for_cache(Hash_Dep hash_dep)
{
	if (hash_dep.is_file()) {
		/* For file targets, we don't use flags for hashing.
		 * Zero is the word for file targets. */
		hash_dep.get_front_word_nondynamic()= (word_t)0;
	}

	return hash_dep;
}

shared_ptr <const Dep> Executor::append_top(
	shared_ptr <const Dep> dep,
	shared_ptr <const Dep> top)
{
	assert(dep);
	assert(top);
	assert(dep != top);

	shared_ptr <Dep> ret= Dep::clone(dep);

	if (dep->top) {
		ret->top= append_top(dep->top, top);
	} else {
		ret->top= top;
	}

	return ret;
}

shared_ptr <const Dep> Executor::set_top(
	shared_ptr <const Dep> dep,
	shared_ptr <const Dep> top)
{
	assert(dep);
	assert(dep != top);

	if (dep->top == nullptr && top == nullptr)
		return dep;

	shared_ptr <Dep> ret= Dep::clone(dep);
	ret->top= top;
	return ret;
}

Proceed Executor::connect(
	shared_ptr <const Dep> dep_this,
	shared_ptr <const Dep> dep_child)
{
	TRACE_FUNCTION();
	TRACE("{%s} dep_child= %s", show(dep_this, S_DEBUG, R_SHOW_FLAGS),
	      show(dep_child, S_DEBUG, R_SHOW_FLAGS));
	DEBUG_PRINT(fmt("connect %s",  show(dep_child, S_DEBUG, R_SHOW_FLAGS)));
	assert(dep_child->is_normalized());
	assert(! to <Root_Dep> (dep_child));
	shared_ptr <const Plain_Dep> plain_dep_this= to <Plain_Dep> (dep_this);

	/* '-p' and '-o' do not mix */
	if (dep_child->flags & F_PERSISTENT && dep_child->flags & F_OPTIONAL) {
		const Place &place_persistent= dep_child->get_place_flag(I_PERSISTENT);
		const Place &place_optional= dep_child->get_place_flag(I_OPTIONAL);
		place_persistent <<
			fmt("declaration of persistent dependency using %s",
			    show_prefix("-", "p"));
		place_optional <<
			fmt("clashes with declaration of optional dependency using %s",
			    show_prefix("-", "o"));
		dep_child->get_place() <<
			fmt("in declaration of %s, needed by %s",
			    show(dep_child), show(dep_this->get_target()));
		*this << "";
		explain_clash();
		raise(ERROR_LOGICAL);
		return 0;
	}

	/* '-o' does not mix with '$[' */
	if (dep_child->flags & F_VARIABLE && dep_child->flags & F_OPTIONAL) {
		shared_ptr <const Plain_Dep> plain_dep_child= to <Plain_Dep> (dep_child);
		assert(plain_dep_child);
		assert(!(dep_child->flags & F_TARGET_TRANSIENT));
		const Place &place_variable= dep_child->get_place();
		const Place &place_flag= dep_child->get_place_flag(I_OPTIONAL);
		place_variable <<
			fmt("variable dependency %s must not be declared as optional dependency",
			    show_dynamic_variable
			    (plain_dep_child->place_target.place_name.unparametrized()));
		place_flag << fmt("using %s", show_prefix("-", "o"));
		*this << "";
		raise(ERROR_LOGICAL);
		return 0;
	}

	Executor *child= get_executor(dep_child);
	if (!child)
		return 0;
	children.insert(child);
	if (dep_child->flags & F_RESULT_NOTIFY)
		for (const auto &dependency: child->result)
			this->notify_result(dependency, this, F_RESULT_NOTIFY, dep_child);
	Proceed proceed_child= child->execute(dep_child);
	assert(proceed_child);
	if (proceed_child & (P_WAIT | P_CALL_AGAIN))
		return proceed_child;
	bool f = child->finished(dep_child->flags);
	TRACE("f= %s", frmt("%d", f));
	if (f) {
		disconnect(child, dep_child);
	}
	return 0;
}

bool Executor::same_dependency_for_print(shared_ptr <const Dep> d1,
					 shared_ptr <const Dep> d2)
{
	shared_ptr <const Plain_Dep> p1= to <Plain_Dep> (d1);
	shared_ptr <const Plain_Dep> p2= to <Plain_Dep> (d2);
	if (!p1 && to <Dynamic_Dep> (d1))
		p1= to <Plain_Dep> (Dynamic_Dep::strip_dynamic(to <Dynamic_Dep> (d1)));
	if (!p2 && to <Dynamic_Dep> (d2))
		p2= to <Plain_Dep> (Dynamic_Dep::strip_dynamic(to <Dynamic_Dep> (d2)));
	if (! (p1 && p2))
		return false;
	return p1->place_target.unparametrized()
		== p2->place_target.unparametrized();
}

void render(const Executor &executor, Parts &parts, Rendering rendering)
{
	executor.render(parts, rendering);
}
