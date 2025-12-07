#include "executor.hh"

#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "cycle.hh"
#include "concat_executor.hh"
#include "dynamic_executor.hh"
#include "explain.hh"
#include "file_executor.hh"
#include "parser.hh"
#include "root_executor.hh"
#include "tokenizer.hh"
#include "trace.hh"
#include "trace_dep.hh"
#include "trace_executor.hh"
#include "transitive_executor.hh"

Rule_Set Executor::rule_set;

Timestamp Executor::timestamp_last;
/* Initialized to zero, i.e., older than the current date */

bool Executor::hide_out_message= false;
bool Executor::out_message_done= false;
std::unordered_map <Hash_Dep, Executor *> Executor::executors_by_hash_dep;

void Executor::read_dynamic(
	shared_ptr <const Plain_Dep> dep_target,
	std::vector <shared_ptr <const Dep> > &deps,
	shared_ptr <const Dep> dep,
	Executor *dynamic_executor)
{
	try {
		const Place_Target &place_target=
			to <Plain_Dep> (dep_target)->place_target;
		assert(place_target.place_name.get_n() == 0);
		const Hash_Dep hash_dep= place_target.unparametrized();
		assert(deps.empty());

		/* Check:  variable dependencies are not allowed in multiply dynamic
		 * dependencies. */
		if (dep_target->flags & F_VARIABLE) {
			dep_target->get_place() <<
				fmt("variable dependency %s must not appear",
				    ::show(dep_target));
			*this << fmt("within multiply-dynamic dependency %s",
				     ::show(dep));
			raise(ERROR_LOGICAL);
		}
		if (place_target.flags & F_TARGET_PHONY)
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

			Tokenizer::parse_tokens_file(
				tokens,
				Tokenizer::DYNAMIC, place_end, filename,
				place_target.place, -1, allow_enoent);

			Place_Name input; /* remains empty */
			Place place_input; /* remains empty */

			try {
				Parser::get_expression_list(
					deps, tokens,
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
				hash_dep_file.get_front_word_nondynamic()
					&= ~F_TARGET_PHONY;
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
				Parser::get_expression_list_delim(
					deps, filename.c_str(),
					place_target.place, c, c_printed,
					*dynamic_executor, allow_enoent);
			} catch (int e) {
				raise(e);
			}
		}

		/* Forbidden features in dynamic dependencies.  In keep-going mode (-k),
		 * we set the error, set the erroneous dependency to null, and at the end
		 * prune the null entries. */
		bool found_error= false;
		if (! delim)
			for (auto &j: deps)
				check_unparametrized(j, hash_dep, found_error);

		assert(! found_error || option_k);

		shared_ptr <const Dep> top_top= dep_target->top;
		shared_ptr <Dep> no_top= dep_target->clone();
		no_top->top= nullptr;
		shared_ptr <Dep> top= std::make_shared <Dynamic_Dep> (no_top);
		top->top= top_top;

		std::vector <shared_ptr <const Dep> > deps_new;
		for (auto &j: deps) {
			if (j) {
				shared_ptr <Dep> j_new= j->clone();
				j_new->top= top;
				deps_new.push_back(j_new);
			}
		}
		swap(deps, deps_new);
	} catch (int e) {
		dynamic_executor->raise(e);
	}
}

Executor *Executor::get_executor(shared_ptr <const Dep> dep)
{
	TRACE_FUNCTION(show_trace(*this));
	TRACE("dep= %s", show_trace(dep));

	/*
	 * Non-cached executors
	 */

	/* Concatenations */
	if (shared_ptr <const Concat_Dep> concat_dep= to <const Concat_Dep> (dep)) {
		TRACE("Creating Concat_Executor");
		Concat_Executor *executor= new Concat_Executor(concat_dep, this);
		assert(executor);
		return executor;
	}

	/* Dynamics that are not cached (with concatenations somewhere inside) */
	if (to <const Dynamic_Dep> (dep)
	    && ! to <const Plain_Dep> (dep->strip_dynamic())) {
		TRACE("Create Dynamic_Executor with concatenation inside");
		int error_additional= 0;
		Dynamic_Executor *executor= new Dynamic_Executor
			(to <const Dynamic_Dep> (dep), this, error_additional);
		assert(executor);
		if (error_additional) {
			should_not_happen();
			/* This cannot happen currently, as either (1) the dynamic would
			 * need to contain a single plain dependency, or (2) a cycle at
			 * rule-level would be found (but this type of Dynamic_Executor
			 * does not have a rule. */
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
		TRACE("Already exists");
		executor= it->second;
		if (executor->parents.count(this)) {
			TRACE("Already connected");
			/* Add necessary flags */
			Flags flags= dep->flags;
			if (flags & ~executor->parents.at(this)->flags) {
				TRACE("Has new flags");
				shared_ptr <Dep> dep_new=
					executor->parents.at(this)->clone();
				for (int i= 0; i < C_PLACED; ++i) {
					if ((flags & (1 << i))
						&& ! (dep_new->flags & (1 << i))) {
						dep_new->places[i]= dep->places[i];
					}
				}
				dep_new->flags |= flags;
				dep_new->check();
				dep= dep_new;
				/* No need to check for cycles here, because a link
				 * between the two already exists and therefore a cycle
				 * cannot be present. */
				executor->parents[this]= dep;
			}
		} else {
			TRACE("Not yet connected");
			if (Cycle::find(this, executor, dep)) {
				TRACE("File-level cycle found");
				raise(ERROR_LOGICAL);
				return nullptr;
			}
			/* The parent and child are not connected -- add the connection */
			executor->parents[this]= dep;
		}
		return executor;
	}

	/* Create a new Executor object */

	TRACE("Creating new executor");
	int error_additional= 0; /* Passed to the executor */
	if (! hash_dep.is_dynamic()) {
		/* Plain executor */
		shared_ptr <const Rule> rule_child, param_rule_child;
		std::map <string, string> mapping_parameter;
		bool use_file_executor= false;
		try {
			Hash_Dep hash_dep_without_flags= hash_dep;
			hash_dep_without_flags.get_front_word_nondynamic()
				&= F_TARGET_PHONY;
			rule_child= rule_set.get(
				hash_dep_without_flags,
				param_rule_child, mapping_parameter,
				dep->get_place());
		} catch (int e) {
			assert(e);
			error_additional= e;
		}
		assert((rule_child == nullptr) == (param_rule_child == nullptr));

		/* RULE_CHILD may be null here; this is handled in the constructors */
		/* We use a File_Executor if:  there is at least one file target in the
		 * rule OR there is a command in the rule.  When there is no rule, we
		 * consult the type of TARGET. */

		if (hash_dep.is_file()) {
			use_file_executor= true;
		} else if (rule_child == nullptr) {
			use_file_executor= false;
		} else if (rule_child->command) {
			use_file_executor= true;
		} else {
			for (auto &i: rule_child->place_targets) {
				if ((i->flags & F_TARGET_PHONY) == 0)
					use_file_executor= true;
			}
		}

		if (use_file_executor) {
			executor= new File_Executor
				(dep, this, rule_child,
				 param_rule_child, mapping_parameter,
				 error_additional);
		} else if (hash_dep.is_phony()) {
			executor= new Transitive_Executor
				(dep, this,
				 rule_child, param_rule_child, mapping_parameter,
				 error_additional);
		}
	} else {
		executor= new Dynamic_Executor(to <Dynamic_Dep> (dep), this, error_additional);
	}

	if (error_additional) {
		error |= error_additional;
		if (executor->want_delete()) {
			should_not_happen();
			/* At the moment, all executors for which want_delete() returns
			 * true don't set error_additional. */
			delete executor;
		}
		return nullptr;
	}
	assert(executor->parents.size() == 1);
	TRACE("Returning new executor");
	return executor;
}

bool Executor::same_rule(const Executor *executor_a, const Executor *executor_b)
/* This must also take into account that two executors could use the same rule but
 * parametrized differently, thus the two executors could have different targets, but the
 * same rule. */
{
	return
		executor_a->param_rule != nullptr &&
		executor_a->param_rule == executor_b->param_rule &&
		executor_a->get_depth() == executor_b->get_depth();
}

void Executor::operator<<(string text) const
/* The following traverses the executor graph backwards until it finds the root.  We
 * always take the first found parent, which is an arbitrary choice, but it doesn't matter
 * here which dependency path we point out as an error, so the first one it is. */
{
	/* If the error happens directly for the root executor, it was an error on the
	 * command line; don't output anything beyond the error message itself, which was
	 * already output. */
	if (dynamic_cast <const Root_Executor *> (this)) {
		should_not_happen();
		return;
	}

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

bool Executor::want_delete() const
{
	return true;
}

void Executor::notify_result( /* uncovered */
	shared_ptr <const Dep>, Executor *, Flags, shared_ptr <const Dep>)
{
	unreachable();
}

void Executor::notify_variable(const std::map <string, string> &)
{
	/* empty */
}

Proceed Executor::execute_children()
{
	TRACE_FUNCTION(show_trace(*this));
	assert(options_jobs > 0);

	/* Since disconnect() may change executor->children, we must first
	 * copy it over locally, and then iterate through it */
	std::vector <Executor *> executors_children_vector
		(children.begin(), children.end());
	Proceed proceed_all= 0;

	while (! executors_children_vector.empty()) {
		assert(options_jobs > 0);
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
		TRACE("dep_child= %s", show_trace(dep_child));

		Proceed proceed_child= child->execute(dep_child);
		TRACE("proceed_child= %s", show(proceed_child));
		TRACE("child->finished(dep_child->flags)= %s",
			frmt("%d", child->finished(dep_child->flags)));
		assert(is_valid(proceed_child));
		assert((proceed_child == P_NOTHING) !=
		       ((child->finished(dep_child->flags)) == 0));

		proceed_all |= proceed_child;
		/* The finished flag of the child only applies to the
		 * child, not to us. */

		if (proceed_child == P_NOTHING) {
			disconnect(child, dep_child);
		}
		if (proceed_all & P_WAIT && options_jobs == 0)
			return proceed_all;
	}

	if (error) {
		assert(option_k);
	}

	if (proceed_all == 0) {
		/* If there are still children, they must have returned P_WAIT or
		 * P_CALL_AGAIN. */
		assert(children.empty());
		if (error) {
			assert(option_k);
		}
		proceed_all= P_NOTHING;
	}

	TRACE("proceed_all= %s", show(proceed_all));
	return proceed_all;
}

void Executor::push(shared_ptr <const Dep> dep)
{
	TRACE_FUNCTION(show_trace(*this));
	TRACE("dep= %s", show_trace(dep));
	dep->check();

	std::vector <shared_ptr <const Dep> > deps;
	int e= 0;
	dep->Dep::normalize(deps, e);
	if (e) {
		if (parents.size()) {
			dep->get_place() <<
				fmt("%s is needed by %s",
					show(dep), show(parents.begin()->second));
			*this << "";
		}
		raise(e);
	}
	for (const auto &d: deps) {
		push_normalized(d);
	}
}

void Executor::push_normalized(shared_ptr <const Dep> dep)
{
	TRACE_FUNCTION(show_trace(*this));
	TRACE("dep= %s", show_trace(dep));
	assert(dep->is_normalized());
	shared_ptr <const Dep> untrivialized= dep->untrivialize();
	if (untrivialized) {
		TRACE("To buffer_B");
		TRACE("untrivialized= %s", show_trace(untrivialized));
		shared_ptr <Dep> dep_b= untrivialized->clone();
		dep_b->flags |= F_PHASE_B;
		buffer_B.push(dep_b);
	} else {
		TRACE("To buffer_A");
		buffer_A.push(dep);
	}
}

void Executor::raise(int e)
{
	assert(e >= 1 && e <= 3);
	error |= e;
	if (! option_k)
		throw error;
}

void Executor::disconnect(Executor *const child, shared_ptr <const Dep> dep_child)
{
	TRACE_FUNCTION(show_trace(*this));
	TRACE("dep_child= %s", show_trace(dep_child));
	assert(child);
	assert(child != this);
	assert(child->finished(dep_child->flags));
	assert(option_k || child->error == 0);
	dep_child->check();

	bool child_was_phase_B= dep_child->flags & F_PHASE_B;
	TRACE("child_was_phase_B= %s", frmt("%d", child_was_phase_B));
	bool child_finished_for_B= child->finished(dep_child->flags | F_PHASE_B);
	TRACE("child_finished_for_B= %s", frmt("%d", child_finished_for_B));

	if (dep_child->flags & F_RESULT
		&& dynamic_cast <File_Executor *> (child)
		&& (child_was_phase_B || child_finished_for_B)) {
		shared_ptr <Dep> d= dep_child->clone();
		d->flags &= ~F_RESULT;
		notify_result(d, child, dep_child->flags & F_RESULT, dep_child);
	}

	if (! child_was_phase_B && ! child_finished_for_B) {
		TRACE("Moving child to phase B");
		shared_ptr <Dep> d= dep_child->clone();
		d->flags |= F_PHASE_B;
		TRACE("d= %s", show_trace(d));
		buffer_B.push(d);
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

const Place &Executor::get_place() const
{
	if (param_rule == nullptr)
		return Place::place_empty;
	else
		return param_rule->place;
}

Proceed Executor::execute_phase_A(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION(show_trace(*this));
	assert(options_jobs > 0);
	assert(dep_link);
	if (finished(dep_link->flags)) {
		TRACE("Finished");
		return P_NOTHING;
	}

	if (optional_finished(dep_link)) {
		TRACE("Optional finished");
		return P_NOTHING;
	}

	Proceed proceed= 0;

	/* Continue the already-active child executors.  In DFS mode, first
	 * continue the already-open children, then open new children.  In
	 * random mode, start new children first and continue already-open
	 * children second. */
	if (order != Order::RANDOM) {
		Proceed proceed_children= execute_children();
		assert(is_valid(proceed_children));
		TRACE("proceed= %s, proceed_children= %s", show(proceed), show(proceed_children));
		proceed |= proceed_children;
		if (proceed & P_WAIT) {
			if (options_jobs == 0) {
				TRACE("Wait; no jobs left");
				return proceed;
			}
		}
	}

	assert(error == 0 || option_k);
	assert(options_jobs > 0);

	while (! buffer_A.empty()) {
		TRACE("A: Buffer A not empty");
		shared_ptr <const Dep> dep_child= buffer_A.pop();
		TRACE("Popped from buffer_A dep_child= %s", show_trace(dep_child));
		Proceed proceed_child= connect(dep_link, dep_child);
		assert(is_valid(proceed_child));
		TRACE("proceed_child= %s", show(proceed_child));
		proceed |= proceed_child;
		assert(options_jobs >= 0);
		if (proceed & P_WAIT && options_jobs == 0) {
			TRACE("No jobs left");
			return proceed;
		}
	}
	assert(buffer_A.empty());

	if (order == Order::RANDOM) {
		Proceed proceed_children= execute_children();
		assert(is_valid(proceed_children));
		proceed |= proceed_children;
		assert(is_valid(proceed));
		if (proceed & P_WAIT && options_jobs == 0) {
			TRACE("Wait");
			return proceed;
		}
	}

	TRACE("Some dependencies are still open");
	if (! children.empty()) {
		TRACE("Children not empty, proceed= %s", show(proceed));
		assert(is_valid(proceed));
		return proceed;
	}

	if (error) {
		assert(option_k);
		TRACE("Error");
		return P_NOTHING;
	}

	TRACE("proceed= %s", show(proceed));
	return proceed;
}

Proceed Executor::execute_phase_B(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION(show_trace(*this));
	assert(buffer_A.empty());
	assert(options_jobs > 0);
	Proceed proceed= 0;

	while (! (buffer_A.empty() && buffer_B.empty())) {
		TRACE("B: Buffers not empty");
		if (! buffer_A.empty()) {
			TRACE("B: Buffer A not empty");
			Proceed proceed_A= execute_phase_A(dep_link);
			if (proceed_A & P_WAIT && options_jobs == 0) {
				return proceed | proceed_A;
			}
			if (proceed_A == P_NOTHING && error) {
				proceed |= proceed_A;
				return proceed;
			}
			proceed |= proceed_A;
		}
		if (! buffer_B.empty()) {
			TRACE("B: Buffer B not empty");
			shared_ptr <const Dep> dep_child= buffer_B.pop();
			TRACE("Popped from buffer_B dep_child=%s", show_trace(dep_child));
			Proceed proceed_child= connect(dep_link, dep_child);
			proceed |= proceed_child;
			assert(options_jobs >= 0);
			if (proceed_child & P_WAIT && options_jobs == 0) {
				TRACE("No jobs left");
				return proceed;
			}
		}
	}
	assert(buffer_A.empty());
	assert(buffer_B.empty());
	proceed= P_NOTHING;
	TRACE("proceed= %s", show(proceed));
	return proceed;
}

void Executor::push_result(shared_ptr <const Dep> dd)
{
	TRACE_FUNCTION();
	TRACE("dd= %s", show_trace(dd));

	assert(! dynamic_cast <File_Executor *> (this));
	assert(! (dd->flags & F_RESULT_NOTIFY));
	dd->check();

	/* Add to own */
	result[trivial_index(dd)].push_back(dd);

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

	shared_ptr <Dep> ret= dep->clone();

	if (dep->top) {
		ret->top= append_top(dep->top, top);
	} else {
		ret->top= top;
	}

	return ret;
}

Proceed Executor::connect(
	shared_ptr <const Dep> dep_this,
	shared_ptr <const Dep> dep_child)
{
	TRACE_FUNCTION(show_trace(dep_this));
	TRACE("dep_child= %s", show_trace(dep_child));
	assert(dep_child->is_normalized());
	assert(! to <Root_Dep> (dep_child));
	shared_ptr <const Plain_Dep> plain_dep_this= to <Plain_Dep> (dep_this);

	/* '-p' and '-o' do not mix */
	if (dep_child->flags & F_PERSISTENT && dep_child->flags & F_OPTIONAL) {
		assert(! to <Root_Dep> (dep_this));
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
		return P_NOTHING;
	}

	/* '-o' does not mix with '$[' */
	if (dep_child->flags & F_VARIABLE && dep_child->flags & F_OPTIONAL) {
		shared_ptr <const Plain_Dep> plain_dep_child= to <Plain_Dep> (dep_child);
		assert(plain_dep_child);
		assert(!(dep_child->flags & F_TARGET_PHONY));
		const Place &place_variable= dep_child->get_place();
		const Place &place_flag= dep_child->get_place_flag(I_OPTIONAL);
		place_variable <<
			fmt("variable dependency %s must not be declared as optional dependency",
			    show_dynamic_variable
			    (plain_dep_child->place_target.place_name.unparametrized()));
		place_flag << fmt("using %s", show_prefix("-", "o"));
		*this << "";
		raise(ERROR_LOGICAL);
		return P_NOTHING;
	}

	Executor *child= get_executor(dep_child);
	if (!child) {
		return P_NOTHING;
	}
	children.insert(child);
	if (dep_child->flags & F_RESULT_NOTIFY) {
		for (const auto &dependency: child->result[(dep_child->flags & F_PHASE_B) != 0])
			this->notify_result(dependency, this, F_RESULT_NOTIFY, dep_child);
	}
	Proceed proceed_child= child->execute(dep_child);
	TRACE("proceed_child= %s", show(proceed_child));
	assert(is_valid(proceed_child));
	if (proceed_child & (P_WAIT | P_CALL_AGAIN))
		return proceed_child;
	bool child_finished= child->finished(dep_child->flags);
	TRACE("child_finished= %s", frmt("%d", child_finished));
	if (child_finished) {
		disconnect(child, dep_child);
	}
	return P_NOTHING;
}

bool Executor::same_dependency_for_print(
	shared_ptr <const Dep> d1,
	shared_ptr <const Dep> d2)
{
	shared_ptr <const Plain_Dep> p1= to <Plain_Dep> (d1);
	shared_ptr <const Plain_Dep> p2= to <Plain_Dep> (d2);
	if (!p1 && to <Dynamic_Dep> (d1))
		p1= to <Plain_Dep> (to <Dynamic_Dep> (d1)->strip_dynamic());
	if (!p2 && to <Dynamic_Dep> (d2))
		p2= to <Plain_Dep> (to <Dynamic_Dep> (d2)->strip_dynamic());
	if (! (p1 && p2))
		return false;
	return p1->place_target.unparametrized()
		== p2->place_target.unparametrized();
}

void Executor::check_unparametrized(
	shared_ptr <const Dep> &j,
	Hash_Dep hash_dep,
	bool &found_error)
{
	TRACE_FUNCTION();
	TRACE("j= %s", show_trace(j));
	TRACE("hash_dep= %s", show(hash_dep));
	string parameter_name;
	Place parameter_place;
	if (! j->find_parameter(parameter_name, parameter_place))
		return;

	parameter_place <<
		fmt("dynamic dependency %s must not contain parameter %s",
			show(Hash_Dep(0, hash_dep)),
			show_prefix("$", parameter_name));
	Hash_Dep hash_dep_base= hash_dep;
	hash_dep_base.get_front_word_nondynamic() &= ~F_TARGET_PHONY;
	hash_dep_base.get_front_word_nondynamic()
		|= (hash_dep.get_front_word_nondynamic() & F_TARGET_PHONY);
	*this << fmt("%s is declared here", show(hash_dep_base));
	explain_dynamic_no_param();
	raise(ERROR_LOGICAL);
	j= nullptr;
	found_error= true;
}

#ifndef NDEBUG
void render(const Executor &executor, Parts &parts, Rendering rendering)
{
	executor.render(parts, rendering);
}
#endif /* ! NDEBUG */
