#ifndef EXECUTOR_HH
#define EXECUTOR_HH

/*
 * Code for executing the building process itself.  If there is ever a "libstu", this will
 * be its main entry point.
 *
 * OVERVIEW OF TYPES
 *
 * EXECUTOR CLASS     CACHING STRATEGY     WHEN USED
 * -----------------------------------------------------------------------------------------
 * Root_Executor      Singleton            The root of the dependency graph; uses the
 *                                         dummy Root_Dep
 * File_Executor      By Target (no flags) Non-dynamic targets with at least one file target
 *                                         in rule OR a command in rule OR files without a
 *                                         rule
 * Transient_Executor By Target (w/ flags) Transients without commands nor file targets in
 *                                         the same rule, i.e., transitive transient targets
 * "Plain executor"   By Target            Name for File_Executor or Transient_Executor
 * Dynamic_Ex.[nocat] By Target (w/ flags) Dynamic^+ targets of Plain_Dep w/o -* flag
 * Dynamic_Ex.[w/cat] Not cached           Dynamic^+ targets of Concat_Dep w/o -* flag
 * Concat_Executor    Not cached           Concatenated targets
 *
 * Caching with flags excludes flags that are not stored in Target objects,
 * i.e., F_RESULT_* flags.
 */

/*
 * Executor is the base class of all executors.  At runtime, executor objects are used to
 * manage the running of Stu itself.  All executor objects are linked to each other in an
 * acyclic directed graph rooted at a Root_Executor.
 *
 * Executors are allocated with new(), are used via ordinary pointers, and deleted (if
 * necessary, depending on caching policy), via delete().
 *
 * The set of active Executor objects forms a directed acyclic graph, rooted at the single
 * Root_Executor object.  Edges in this graph are represented by dependencies.  An edge is
 * said to go from a parent to a child.  Each Executor object corresponds to one or more
 * unique dependencies.  Two Executor objects are connected if there is a dependency
 * between them.  If there is an edge A ---> B, A is said to be the parent of B, and B the
 * child of A.  Also, we say that B is a dependency of A, even though properly speaking,
 * the edge connecting them is the dependency.
 */

#include "bits.hh"
#include "buffer.hh"
#include "debug.hh"
#include "job.hh"
#include "proceed.hh"
#include "rule.hh"
#include "timestamp.hh"

class Executor
	: private Printer, protected Debuggable
{
public:
	void raise(int error);
	/* Set the error code, and throw an error except with the keep-going
	 * option.  Does not print an error message. */

	int get_error() const { return error; }

	void read_dynamic(shared_ptr <const Plain_Dep> dep_target,
			  std::vector <shared_ptr <const Dep> > &deps,
			  shared_ptr <const Dep> dep, Executor *dynamic_executor);
	/* Read dynamic dependencies.  The only reason this is not static is
	 * that errors can be raised and printed correctly.  Dependencies that
	 * are read are written into DEPS, which is empty on calling. */

	void operator<<(string text) const override;
	/* Print full trace for the executor.  First the message is printed,
	 * then all traces for it starting at this executor, up to the root
	 * executor.  TEXT may be "" to not print the first message. */

	const std::map <Executor *, shared_ptr <const Dep> > &get_parents() const {
		return parents;
	}

	virtual bool want_delete() const= 0;

	virtual Proceed execute(shared_ptr <const Dep> dep_link)= 0;
	/* Start the next job(s).  This will also terminate jobs when they don't need to
	 * be run anymore, and thus it can be called when K = 0 just to terminate jobs
	 * that need to be terminated.  Can only return LATER in random mode.  When
	 * returning LATER, not all possible child jobs where started.  Child
	 * implementations call this implementation.  Never returns P_CONTINUE: When
	 * everything is finished, the FINISHED bit is set.  In DONE, set those bits that
	 * have been done.  When the call is over, clear the PENDING bit.  DEPENDENCY_LINK
	 * is only null when called on the root executor, because it is the only executor
	 * that is not linked from another executor. */

	virtual bool finished() const= 0;
	/* Whether the executor is completely finished */

	virtual bool finished(Flags flags) const= 0;
	/* Whether the executor is finished working for the given tasks */

	virtual void notify_result(shared_ptr <const Dep> dep_result,
				   Executor *source, Flags flags,
				   shared_ptr <const Dep> dep_source)
	/* The child executor SOURCE notifies THIS about a new result.  Only
	 * called when the dependency linking the two had one of the F_RESULT_*
	 * flag.  The given flag contains only one of the two F_RESULT_* flags.
	 * DEP_SOURCE is the dependency leading from THIS to SOURCE (for
	 * F_RESULT_COPY). */
	{
		unreachable();
		(void) dep_result; (void) source; (void) flags; (void) dep_source;
	}

	virtual void notify_variable
	(const std::map <string, string> &result_variable_child) {
		(void) result_variable_child;
	}

	virtual void render(Parts &, Rendering= 0) const override= 0;

	static bool hide_out_message;
	/* Whether to show a STDOUT message at the end */

	static bool out_message_done;
	/* Whether the STDOUT message is not "Targets are up to date" */

	static Rule_Set rule_set;
	/* Set before calling main_loop() */

	static Hash_Dep get_target_for_cache(Hash_Dep hash_dep);
	/* Get the target value used for caching.  I.e, return TARGET with certain flags
	 * removed. */

protected:
	Bits bits= 0;
	int error; /* Propagated using '|' to the parent */

	std::map <Executor *, shared_ptr <const Dep> > parents;
	/* This is a map rather than an unsorted_map because typically, the number of
	 * elements is always very small, i.e., mostly one, and a map is better suited in
	 * this case.  The map is sorted, but by the executor pointer, i.e., the sorting
	 * is arbitrary as far as Stu is concerned. */

	std::set <Executor *> children;

	Timestamp timestamp;
	/* Latest timestamp of a (direct or indirect) dependency that was not rebuilt.
	 * Files that were rebuilt are not considered, since they make the target be
	 * rebuilt anyway.  Implementations also change this to consider the file itself,
	 * if any.  This final timestamp is then carried over to the parent executors. */

	std::vector <shared_ptr <const Dep> > result[2];
	/* The final list of dependencies represented by the target.  This does not
	 * include any dynamic dependencies, i.e., all dependencies are flattened to
	 * Plain_Dep's.  Not used for executors that have file targets, neither for
	 * executors that have multiple targets.  This is not used for file dependencies,
	 * as a file dependency's result can be each of its files, depending on the parent
	 * -- for file dependencies, parents are notified directly, bypassing
	 * push_result().
	 * The index is the trivial bit, i.e. result[1] are results only as a trivial
	 * target, and result[0]+result[1] are the results for non-trivial targets.
	 */

	std::map <string, string> result_variable;
	/* Same semantics as RESULT, but for variable values, stored as KEY-VALUE pairs. */

	shared_ptr <const Rule> param_rule;
	/* The (possibly parametrized) rule from which this executor was derived.  This is
	 * only used to detect cycles.  To manage the dependencies, the instantiated
	 * general rule is used.  Null by default, and set by individual implementations
	 * in their constructor if necessary. */

	explicit Executor(shared_ptr <const Rule> param_rule_= nullptr)
		: error(0), timestamp(Timestamp::UNDEFINED),
		  param_rule(param_rule_) { }

	Proceed execute_children();
	/* Execute already-active children */

	Proceed execute_phase_A(shared_ptr <const Dep> dep_link);
	/* DEP_LINK must not be null.  In the return value, at least one bit is set.  The
	 * P_FINISHED bit indicates only that tasks related to this function are done, not
	 * the whole Executor. */

	Proceed execute_phase_B(shared_ptr <const Dep> dep_link);
	/* Second pass (trivial dependencies).  Called once we are sure that the target
	 * must be built.  Arguments and return value have the same semantics as
	 * execute_base_B(). */

	Executor *get_executor(shared_ptr <const Dep> dep);
	/* Get an existing Executor or create a new one for the given
	 * DEPENDENCY.  Return null on errors. */

	void check_waited() const {
		assert(buffer_A.empty());
		assert(buffer_B.empty());
		assert(children.empty());
	}

	const Buffer &get_buffer_A() const { return buffer_A; }
	const Buffer &get_buffer_B() const { return buffer_B; }

	void push(shared_ptr <const Dep> dep);
	/* Push a dependency to the default buffer, breaking down non-normalized
	 * dependencies while doing so.  DEP does not have to be normalized. */

	void push_result(shared_ptr <const Dep> dd);
	Proceed connect(shared_ptr <const Dep> dep_this,
			shared_ptr <const Dep> dep_child);
	void disconnect(Executor *const child,
			shared_ptr <const Dep> dep_child);

	const Place &get_place() const
	/* The place for the executor; e.g. the rule; empty if there is no place */
	{
		if (param_rule == nullptr)
			return Place::place_empty;
		else
			return param_rule->place;
	}

	shared_ptr <const Dep> append_top(shared_ptr <const Dep> dep,
					  shared_ptr <const Dep> top);
	shared_ptr <const Dep> set_top(shared_ptr <const Dep> dep,
				       shared_ptr <const Dep> top);

	virtual ~Executor()= default;

	virtual int get_depth() const= 0;
	/* -1 when undefined as in concatenated executors and the root
	 * executor, in which case PARAM_RULE is always null.  Only used to
	 * check for cycles on the rule level. */

	virtual bool optional_finished(shared_ptr <const Dep> dep_link)= 0;
	/* Whether the executor would be finished if this was an optional
	 * dependency.  Check whether this is an optional dependency and if it
	 * is, return TRUE when the file does not exist.  Return FALSE when
	 * children should be started.  Return FALSE in executor types that are
	 * not affected. */

	static Timestamp timestamp_last;
	/* The timepoint of the last time wait() returned.  No file in the
	 * file system should be newer than this. */

	static std::unordered_map <Hash_Dep, Executor *> executors_by_hash_dep;
	/* All cached Executor objects by each of their Target.  Such
	 * Executor objects are never deleted. */

	static bool find_cycle(Executor *parent, Executor *child,
			       shared_ptr <const Dep> dep_link);
	/* Find a cycle.  Assuming that the edge parent-->child will be added, find a
	 * directed cycle that would be created.  Start at PARENT and perform a
	 * depth-first search upwards in the hierarchy to find CHILD.  DEPENDENCY_LINK is
	 * the link that would be added between child and parent, and would create a
	 * cycle. */

	static bool find_cycle(std::vector <Executor *> &path,
			       Executor *child,
			       shared_ptr <const Dep> dep_link);
	/* Helper function.  PATH is the currently explored path.  PATH[0] is the original
	 * PARENT; PATH[end] is the oldest grandparent found yet. */

	static void cycle_print(const std::vector <Executor *> &path,
				shared_ptr <const Dep> dep);
	/* Print the error message of a cycle on rule level.
	 * Given PATH = [a, b, c, d, ..., x], the found cycle is
	 * [x <- a <- b <- c <- d <- ... <- x], where A <- B denotes
	 * that A is a dependency of B.  For each edge in this cycle,
	 * output one line.  DEPENDENCY is the link (x <- a), which is not yet
	 * created in the executor objects.  All other link
	 * dependencies are read from the executor objects. */

	static bool same_rule(const Executor *executor_a,
			      const Executor *executor_b);
	/* Whether both executors have the same parametrized rule.
	 * Only used for finding cycles. */

	static int trivial_index(shared_ptr <const Dep> d) {
		return d->flags & F_TRIVIAL ? 1 : 0;
	}

private:
	Buffer buffer_A;
	/* Dependencies that have not yet begun to be built.  Initialized with all
	 * dependencies, and emptied over time when things are built, and filled over time
	 * when dynamic dependencies are worked on.  Entries are not necessarily unique.
	 * Dependencies are normalized. */

	Buffer buffer_B;
	/* The buffer for dependencies in the second pass.  They are only started if,
	 * after (potentially) starting all non-trivial dependencies, the target must be
	 * rebuilt anyway.  Dependencies are normalized. */

	static bool hide_link_from_message(Flags flags) {
		return flags & F_RESULT_NOTIFY;
	}
	static bool same_dependency_for_print(shared_ptr <const Dep> d1,
					      shared_ptr <const Dep> d2);
};

void render(const Executor &, Parts &, Rendering= 0);

#endif /* ! EXECUTOR_HH */
