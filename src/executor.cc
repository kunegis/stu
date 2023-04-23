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
unordered_map <Target, Executor *> Executor::executors_by_target;

void Executor::read_dynamic(shared_ptr <const Plain_Dep> dep_target,
			    vector <shared_ptr <const Dep> > &deps,
			    shared_ptr <const Dep> dep,
			    Executor *dynamic_executor)
{
	try {
		const Place_Param_Target &place_param_target=
			to <Plain_Dep> (dep_target)->place_param_target;
		assert(place_param_target.place_name.get_n() == 0);
		const Target target= place_param_target.unparametrized();
		assert(deps.empty());

		/* Check:  variable dependencies are not allowed in multiply
		 * dynamic dependencies.  */
		if (dep_target->flags & F_VARIABLE) {
			dep_target->get_place() <<
				fmt("variable dependency %s must not appear",
				    dep_target->format_err());
			*this << fmt("within multiply-dynamic dependency %s",
				     dep->format_err());
			raise(ERROR_LOGICAL);
		}
		if (place_param_target.flags & F_TARGET_TRANSIENT)
			return;

		assert(target.is_file());
		string filename= target.get_name_nondynamic();

		bool delim= (dep_target->flags & (F_NEWLINE_SEPARATED | F_NUL_SEPARATED));
		/* Whether the dynamic dependency is delimiter-separated */

		if (! delim) {
			/* Dynamic dependency in full Stu syntax */
			vector <shared_ptr <Token> > tokens;
			Place place_end;

			Tokenizer::parse_tokens_file
				(tokens, Tokenizer::DYNAMIC,
				 place_end, filename,
				 place_param_target.place, -1,
				 dep_target->flags & F_OPTIONAL);

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
				Target target_dynamic(0, target);
				place_input <<
					fmt("dynamic dependency %s must not contain input redirection %s",
					    target_dynamic.format_err(),
					    prefix_format_err(input.raw(), "<"));
				Target target_file= target;
				target_file.get_front_word_nondynamic() &= ~F_TARGET_TRANSIENT;
				(*dynamic_executor) << fmt("%s is declared here",
							   target_file.format_err());
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
					 *dynamic_executor);
			} catch (int e) {
				raise(e);
			}
		}

		/* Perform checks on forbidden features in dynamic dependencies.
		 * In keep-going mode (-k), we set the error, set the erroneous
		 * dependency to null, and at the end prune the null entries.  */
		bool found_error= false;
		if (! delim)  for (auto &j:  deps) {
			/* Check that it is unparametrized */
			if (! j->is_unparametrized()) {
				shared_ptr <const Dep> depp= j;
				while (to <Dynamic_Dep> (depp)) {
					shared_ptr <const Dynamic_Dep> depp2=
						to <Dynamic_Dep> (depp);
					depp= depp2->dep;
				}
				to <Plain_Dep> (depp)
					->place_param_target.place_name.places[0] <<
					fmt("dynamic dependency %s must not contain parametrized dependencies",
					    Target(0, target).format_err());
				Target target_base= target;
				target_base.get_front_word_nondynamic() &= ~F_TARGET_TRANSIENT;
				target_base.get_front_word_nondynamic() |= (target.get_front_word_nondynamic() & F_TARGET_TRANSIENT);
				*this << fmt("%s is declared here",
					     target_base.format_err());
				raise(ERROR_LOGICAL);
				j= nullptr;
				found_error= true;
				continue;
			}
		}

		assert(! found_error || option_k);
		vector <shared_ptr <const Dep> > deps_new;

		shared_ptr <const Dep> top_top= dep_target->top;
		shared_ptr <Dep> no_top= Dep::clone(dep_target);
		no_top->top= nullptr;
		shared_ptr <Dep> top= make_shared <Dynamic_Dep> (no_top);
		top->top= top_top;

		for (auto &j:  deps) {
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
	vector <Executor *> path;
	path.push_back(parent);
	return find_cycle(path, child, dep_link);
}

bool Executor::find_cycle(vector <Executor *> &path,
			  Executor *child,
			  shared_ptr <const Dep> dep_link)
{
	if (same_rule(path.back(), child)) {
		cycle_print(path, dep_link);
		return true;
	}
	for (auto &i:  path.back()->parents) {
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

void Executor::cycle_print(const vector <Executor *> &path,
			   shared_ptr <const Dep> dep)
/*
 * Given PATH = [a, b, c, d, ..., x], we print:
 *
 * 	x depends on ...      \
 *      ... depends on d      |
 *      d depends on c        | printed from PATH
 *      c depends on b        |
 *      b depends on a        /
 *      a depends on x        > printed from DEP
 *      x is needed by ...    \
 *      ...                   | printed by print_traces()
 *      ...                   /
 */
{
	assert(path.size() > 0);

	vector <string> names;
	/* Indexes are parallel to PATH */
	names.resize(path.size());

	for (size_t i= 0;  i + 1 < path.size();  ++i)
		names[i]= path[i]->parents.at(path[i+1])->format_err();
	names.back()= path.back()->parents.begin()->second->format_err();

	for (ssize_t i= path.size() - 1;  i >= 0;  --i) {
		shared_ptr <const Dep> d= i == 0
			? dep
			: path[i - 1]->parents.at(const_cast <Executor *> (path[i]));

		/* Don't show a message for left-branch dynamic links */
		if (hide_link_from_message(d->flags))
			continue;

		d->get_place()
			<< fmt("%s%s depends on %s",
			       i == (ssize_t)(path.size() - 1)
			       ? (path.size() == 1
				  || (path.size() == 2 && hide_link_from_message(dep->flags))
				  ? "target must not depend on itself: "
				  : "cyclic dependency: ")
			       : "",
			       names[i],
			       i == 0 ? dep->format_err() : names[i - 1]);
	}

	/* If the two targets are different (but have the same rule
	 * because they match the same pattern and/or because they are
	 * two different targets of a multitarget rule), then output a
	 * notice to that effect */
	Target t1= path.back()->parents.begin()->second->get_target();
	Target t2= dep->get_target();
	const char *c1= t1.get_name_c_str_any();
	const char *c2= t2.get_name_c_str_any();
	if (strcmp(c1, c2)) {
		path.back()->get_place()
			<<
			fmt("both %s and %s match the same rule",
			    name_format_err(c1), name_format_err(c2));
	}

	/* Remove the offending (cycle-generating) link between the
	 * two.  The offending link is from path[0] as a parent to
	 * path[end] (as a child).  */
	path.back()->parents.erase(path.at(0));
	path.at(0)->children.erase(path.back());

	*path.back() << "";
	explain_cycle();
}

Executor *Executor::get_executor(shared_ptr <const Dep> dep)
{
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
	if (to <const Dynamic_Dep> (dep) && ! to <const Plain_Dep> (Dep::strip_dynamic(dep))) {
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

	const Target target= dep->get_target();

	/* Set to the returned Executor object when one is found or created */
	Executor *executor= nullptr;

	const Target target_for_cache= get_target_for_cache(target);
	auto it= executors_by_target.find(target_for_cache);

	if (it != executors_by_target.end()) {
		/* An Executor object already exists for the target */
		executor= it->second;
		if (executor->parents.count(this)) {
			/* THIS and CHILD are already connected -- add the
			 * necessary flags  */
			Flags flags= dep->flags;
			if (flags & ~executor->parents.at(this)->flags) {
				shared_ptr <Dep> dep_new= Dep::clone(executor->parents.at(this));
				dep_new->flags |= flags;
				dep= dep_new;
				/* No need to check for cycles here,
				 * because a link between the two
				 * already exists and therefore a cycle
				 * cannot be present.  */
				executor->parents[this]= dep;
			}
		} else {
			if (find_cycle(this, executor, dep)) {
				raise(ERROR_LOGICAL);
				return nullptr;
			}
			/* The parent and child are not connected -- add the
			 * connection */
			executor->parents[this]= dep;
		}
		return executor;
	}

	/* Create a new Executor object */

	int error_additional= 0; /* Passed to the executor */

	if (! target.is_dynamic()) {
		/* Plain executor */
		shared_ptr <const Rule> rule_child, param_rule_child;
		map <string, string> mapping_parameter;
		bool use_file_executor= false;
		try {
			Target target_without_flags= target;
			target_without_flags.get_front_word_nondynamic() &= F_TARGET_TRANSIENT;
			rule_child= rule_set.get(target_without_flags,
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

		if (target.is_file()) {
			use_file_executor= true;
		} else if (rule_child == nullptr) {
			use_file_executor= false;
 		} else if (rule_child->command) {
			use_file_executor= true;
		} else {
			for (auto &i:  rule_child->place_param_targets) {
				if ((i->flags & F_TARGET_TRANSIENT) == 0)
					use_file_executor= true;
			}
		}

		if (use_file_executor) {
			executor= new File_Executor
				(dep, this, rule_child,
				 param_rule_child, mapping_parameter,
				 error_additional);
		} else if (target.is_transient()) {
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
	return executor;
}

bool Executor::same_rule(const Executor *executor_a,
			 const Executor *executor_b)
/* This must also take into account that two executors could use the
 * same rule but parametrized differently, thus the two executors could
 * have different targets, but the same rule.  */
{
	return
		executor_a->param_rule != nullptr &&
		executor_b->param_rule != nullptr &&
		executor_a->get_depth() == executor_b->get_depth() &&
		executor_a->param_rule == executor_b->param_rule;
}

void Executor::operator<<(string text) const
/* The following traverses the executor graph backwards until it finds
 * the root.  We always take the first found parent, which is an
 * arbitrary choice, but it doesn't matter here which dependency path
 * we point out as an error, so the first one it is.  */
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

	string text_parent= depp->format_err();

	while (true) {
		if (dynamic_cast <const Root_Executor *> (executor)) {
			/* We are in a child of the root executor */
			assert(! depp->top);
			if (first && ! text.empty()) {
				/* No text was printed yet, but there
				 * was a TEXT passed:  Print it with the
				 * place available.  */
				/* This is a top-level target, i.e.,
				 * passed on the command line via an
 				 * argument or an option  */
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
		text_parent= depp->format_err();

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
	/* Since disconnect() may change executor->children, we must first
	 * copy it over locally, and then iterate through it */

	vector <Executor *> executors_children_vector
		(children.begin(), children.end());
	Proceed proceed_all= 0;

	while (! executors_children_vector.empty()) {
		assert(options_jobs >= 0);
		if (order_vec) {
			/* Exchange a random position with last position */
			size_t p_last= executors_children_vector.size() - 1;
			size_t p_random= random_number(executors_children_vector.size());
			if (p_last != p_random) {
				swap(executors_children_vector[p_last],
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
		 * child, not to us  */

		assert(((proceed_child & P_FINISHED) == 0) ==
		       ((child->finished(dep_child->flags)) == 0));

		if (proceed_child & P_FINISHED) {
			disconnect(child, dep_child);
		} else {
			assert((proceed_child & ~P_FINISHED) != 0);
			/* If the child executor is not finished, it
			 * must have returned either the P_WAIT or
			 * P_PENDING bit.  */
		}
	}

	if (error) {
		assert(option_k);
	}

	if (proceed_all == 0) {
		/* If there are still children, they must have returned
		 * WAIT or PENDING */
		assert(children.empty());
		if (error) {
			assert(option_k);
		}
	}

	return proceed_all;
}

void Executor::push(shared_ptr <const Dep> dep)
{
	assert(dep);
	dep->check();

	vector <shared_ptr <const Dep> > deps;
	int e= 0;
	Dep::normalize(dep, deps, e);
	if (e) {
		dep->get_place() << fmt("%s is needed by %s",
					dep->format_err(),
					parents.begin()->second->format_err());
		*this << "";
		raise(e);
	}
	for (const auto &d:  deps) {
		d->check();
		assert(d->is_normalized());
		buffer_A.push(d);
	}
}

Proceed Executor::execute_base_A(shared_ptr <const Dep> dep_this)
{
	Debug debug(this);
	assert(options_jobs >= 0);
	assert(dep_this);
	Proceed proceed= 0;

	if (finished(dep_this->flags)) {
		Debug::print(this, "finished");
		return proceed |= P_FINISHED;
	}

	if (optional_finished(dep_this)) {
		Debug::print(this, "optional finished");
		return proceed |= P_FINISHED;
	}

	/* In DFS mode, first continue the already-open children, then
	 * open new children.  In random mode, start new children first
	 * and continue already-open children second */
	/* Continue the already-active child executors */
	if (order != Order::RANDOM) {
		Proceed proceed_2= execute_children();
		proceed |= proceed_2;
		if (proceed & P_WAIT) {
			if (options_jobs == 0)
				return proceed;
		} else if (finished(dep_this->flags) && ! option_k) {
			Debug::print(this, "finished");
			return proceed |= P_FINISHED;
		}
	}

	/* Is this a trivial run?  Then skip the dependency. */
	if (dep_this->flags & F_TRIVIAL) {
		return proceed |= P_ABORT | P_FINISHED;
	}

	assert(error == 0 || option_k);

	/*
	 * Deploy dependencies (first pass), with the F_NOTRIVIAL flag
	 */

	if (options_jobs == 0) {
		return proceed |= P_WAIT;
	}

	while (! buffer_A.empty()) {
		shared_ptr <const Dep> dep_child= buffer_A.next();
		if ((dep_child->flags & (F_RESULT_NOTIFY | F_TRIVIAL)) == F_TRIVIAL) {
			shared_ptr <Dep> dep_child_2=
				Dep::clone(dep_child);
			dep_child_2->flags &= ~F_TRIVIAL;
			dep_child_2->get_place_flag(I_TRIVIAL)= Place::place_empty;
			buffer_B.push(dep_child_2);
		}
		Proceed proceed_2= connect(dep_this, dep_child);
		proceed |= proceed_2;
		if (options_jobs == 0) {
			return proceed |= P_WAIT;
		}
	}
	assert(buffer_A.empty());

	if (order == Order::RANDOM) {
		Proceed proceed_2= execute_children();
		proceed |= proceed_2;
		if (proceed & P_WAIT)
			return proceed;
	}

	/* Some dependencies are still running */
	if (! children.empty()) {
		assert(proceed != 0);
		return proceed;
	}

	/* There was an error in a child */
	if (error) {
		assert(option_k);
		return proceed |= P_ABORT | P_FINISHED;
	}

	if (proceed)
		return proceed;

	return proceed |= P_FINISHED;
}

void Executor::raise(int error_)
{
	assert(error_ >= 1 && error_ <= 3);
	error |= error_;
	if (! option_k)
		throw error;
}

void Executor::disconnect(Executor *const child,
			  shared_ptr <const Dep> dep_child)
{
	Debug::print(this, fmt("disconnect %s", dep_child->format_src()));

	assert(child != nullptr);
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

	/* Note: propagate the flags after propagating other things,
	 * since flags can be changed by the propagations done
	 * before.  */

	error |= child->error;

	/* Don't propagate the NEED_BUILD flag via DYNAMIC_LEFT links:
	 * It just means the list of depenencies have changed, not the
	 * dependencies themselves.  */
	if (child->bits & B_NEED_BUILD
	    && ! (dep_child->flags & F_RESULT_NOTIFY)) {
		bits |= B_NEED_BUILD;
	}

	/* Remove the links between them */
	assert(children.count(child) == 1);
	assert(child->parents.count(this) == 1);
	children.erase(child);
	child->parents.erase(this);

	/* Delete the Executor object */
	if (child->want_delete())
		delete child;
}

Proceed Executor::execute_base_B(shared_ptr <const Dep> dep_link)
{
	Proceed proceed= 0;
	while (! buffer_B.empty()) {
		shared_ptr <const Dep> dep_child= buffer_B.next();
		Proceed proceed_2= connect(dep_link, dep_child);
		proceed |= proceed_2;
		assert(options_jobs >= 0);
		if (options_jobs == 0) {
			return proceed |= P_WAIT;
		}
	}
	assert(buffer_B.empty());

	return proceed;
}

void Executor::copy_result(Executor *parent, Executor *child)
{
	/* Check that the child is not of a type for which RESULT is not
	 * used */
	if (dynamic_cast <File_Executor *> (child)) {
		File_Executor *file_child= dynamic_cast <File_Executor *> (child);
		assert(file_child->targets.size() == 1 &&
		       file_child->targets.at(0).is_transient());
	}

	for (auto &i:  child->result) {
		parent->result.push_back(i);
	}
}

void Executor::push_result(shared_ptr <const Dep> dd)
{
	Debug::print(this, fmt("push_result %s", dd->format_src()));

	assert(! dynamic_cast <File_Executor *> (this));
	assert(! (dd->flags & F_RESULT_NOTIFY));
	dd->check();

	/* Add to own */
	result.push_back(dd);

	/* Notify parents */
	for (auto &i:  parents) {
		Flags flags= i.second->flags & (F_RESULT_NOTIFY | F_RESULT_COPY);
		if (flags) {
			i.first->notify_result(dd, this, flags, i.second);
		}
	}
}

Target Executor::get_target_for_cache(Target target)
{
	if (target.is_file()) {
		/* For file targets, we don't use flags for hashing.
		 * Zero is the word for file targets.  */
		target.get_front_word_nondynamic()= (word_t)0;
	}

	return target;
}

shared_ptr <const Dep> Executor::append_top(shared_ptr <const Dep> dep,
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

shared_ptr <const Dep> Executor::set_top(shared_ptr <const Dep> dep,
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

Proceed Executor::connect(shared_ptr <const Dep> dep_this,
			  shared_ptr <const Dep> dep_child)
{
	Debug::print(this, fmt("connect %s",  dep_child->format_src()));

	assert(dep_child->is_normalized());
	assert(! to <Root_Dep> (dep_child));

	shared_ptr <const Plain_Dep> plain_dep_this=
		to <Plain_Dep> (dep_this);

	/*
	 * Check for various invalid types of connections
	 */

	/* '-p' and '-o' do not mix */
	if (dep_child->flags & F_PERSISTENT && dep_child->flags & F_OPTIONAL) {

		/* '-p' and '-o' encountered for the same target */
		const Place &place_persistent=
			dep_child->get_place_flag(I_PERSISTENT);
		const Place &place_optional=
			dep_child->get_place_flag(I_OPTIONAL);
		place_persistent <<
			fmt("declaration of persistent dependency using %s",
			    multichar_format_err("-p"));
		place_optional <<
			fmt("clashes with declaration of optional dependency using %s",
			    multichar_format_err("-o"));
		dep_child->get_place() <<
			fmt("in declaration of %s, needed by %s",
			    dep_child->format_err(),
			    dep_this->get_target().
			    format_err());
		*this << "";
		explain_clash();
		raise(ERROR_LOGICAL);
		return 0;
	}

	/* '-o' does not mix with '$[' */
	if (dep_child->flags & F_VARIABLE && dep_child->flags & F_OPTIONAL) {
		shared_ptr <const Plain_Dep> plain_dep_child=
			to <Plain_Dep> (dep_child);
		assert(plain_dep_child);
		assert(!(dep_child->flags & F_TARGET_TRANSIENT));
		const Place &place_variable= dep_child->get_place();
		const Place &place_flag= dep_child->get_place_flag(I_OPTIONAL);
		place_variable <<
			fmt("variable dependency %s must not be declared "
			    "as optional dependency",
			    dynamic_variable_format_err
			    (plain_dep_child->place_param_target.place_name.unparametrized()));
		place_flag << fmt("using %s",
				  multichar_format_err("-o"));
		*this << "";
		raise(ERROR_LOGICAL);
		return 0;
	}

	/*
	 * Actually do the connection
	 */

	Executor *child= get_executor(dep_child);
	if (child == nullptr) {
		/* Strong cycle was found */
		return 0;
	}

	children.insert(child);

	if (dep_child->flags & F_RESULT_NOTIFY) {
		for (const auto &dependency:  child->result) {
			this->notify_result(dependency, this, F_RESULT_NOTIFY, dep_child);
		}
	}

	Proceed proceed_child= child->execute(dep_child);
	assert(proceed_child);
	if (proceed_child & (P_WAIT | P_PENDING))
		return proceed_child;
	if (child->finished(dep_child->flags)) {
		disconnect(child, dep_child);
	}
	return 0;
}

bool Executor::same_dependency_for_print(shared_ptr <const Dep> d1,
					 shared_ptr <const Dep> d2)
{
	shared_ptr <const Plain_Dep> p1=
		to <Plain_Dep> (d1);
	shared_ptr <const Plain_Dep> p2=
		to <Plain_Dep> (d2);
	if (!p1 && to <Dynamic_Dep> (d1))
		p1= to <Plain_Dep>
			(Dynamic_Dep::strip_dynamic(to <Dynamic_Dep> (d1)));
	if (!p2 && to <Dynamic_Dep> (d2))
		p2= to <Plain_Dep>
			(Dynamic_Dep::strip_dynamic(to <Dynamic_Dep> (d2)));
	if (! (p1 && p2))
		return false;
	return p1->place_param_target.unparametrized()
		== p2->place_param_target.unparametrized();
}
