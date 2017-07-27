#ifndef EXECUTION_HH
#define EXECUTION_HH

/* 
 * Code for executing the building process itself.  
 *
 * If there is ever a "libstu", this will be its main entry point. 
 * 
 * OVERVIEW OF TYPES
 *
 * EXECUTION CLASS	CACHED?				WHEN USED
 * ---------------------------------------------------------------------------------------------------
 * Root_Execution	not cached (single object) 	The root of the dependency graph; 
 *       						uses the dummy Root_Dependency 
 * File_Execution	cached by Target (no flags)	Non-dynamic targets with at least one
 *							file target in rule OR a command in rule OR
 *							files without a rule
 * Transient_Execution	cached by Target (w/ flags)	Transients without commands nor file targets in
 * 							the same rule, i.e., transitive transient targets
 * "Plain execution"	cached by Target		Name for File_Execution or Transient_Execution
 * Dynamic_Ex.[nocat]	cached by Target (w/ flags)	Dynamic^+ targets of Plain_Dependency w/o -* flag
 * Dynamic_Ex.[w/cat]	not cached 			Dynamic^+ targets of Concatenated_Dependency w/o -* flag
 * Concatenated_Ex.	not cached			Concatenated targets
 *
 * Caching with flags excludes flags that are not stored in Target
 * objects, i.e., F_RESULT_* flags.   
 */

#include <sys/stat.h>

#include "buffer.hh"
#include "parser.hh"
#include "job.hh"
#include "tokenizer.hh"
#include "rule.hh"
#include "timestamp.hh"

typedef unsigned Proceed;
/* This is used as the return value of the functions execute*() Defined
 * as typedef to make arithmetic with it.  */
enum {
	P_WAIT =     1 << 0,
	/* There's more to do, but it can only be started after having
	 * waited for other jobs to finish.  I.e., the wait function
	 * will have to be called.  */

	P_PENDING =  1 << 1,
	/* The function execute() should be called again for this
	 * execution (without waiting) at least, for various reasons,
	 * mostly for randomization of exdecution order.  */
		
	P_FINISHED = 1 << 2,
	/* This Execution is finished */ 
		
	P_ABORT    = 1 << 3,
	/* This Execution should be finished immediately.  When set,
	 * P_FINISHED is also set.  This does not imply that there was
	 * an error -- for instance, the trivial flag -t may mean that
	 * nothing more should be done.  */
};

class Execution
/*
 * Base class of all executions.
 *
 * Executions are allocated with new(), are used via ordinary pointers,
 * and deleted (if nercessary), via delete().  
 *
 * The set of active Execution objects forms a directed acyclic graph,
 * rooted at a single Root_Execution object.  Edges in this graph are
 * represented by dependencies.  An edge is said to go from a parent to
 * a child.  Each Execution object corresponds to one or more unique
 * dependencies.  Two Execution objects are connected if there is a
 * dependency between them.  If there is an edge A ---> B, A is said to
 * be the parent of B, and B the child of A.  Also, B is a dependency of
 * A.  
 */
{
public: 
	typedef unsigned Bits;
	/* These are bits set for individual execution objects.  The
	 * semantics of each is chosen such that in a new execution
	 * object, the value is zero.  The semantics of the different
	 * bits are distinct and could just as well be realized as
	 * individual "bool" variables.  */ 
	enum {
		B_NEED_BUILD	= 1 << 0,
		/* Whether this target needs to be built.  When a target is
		 * finished, this value is propagated to the parent executions,
		 * except when the F_PERSISTENT flag is set.  */ 

		B_CHECKED  	= 1 << 1,
		/* Whether a certain check has been performed.  Only
		 * used by File_Execution.  */

		B_EXISTING	= 1 << 2, 
		/* All file targets are known to exist (only in
		 * File_Execution; may be set when there are no file
		 * targets).  */

		B_MISSING	= 1 << 3,
		/* At least one file target is known not to exist (only
		 * possible if there is at least one file target in
		 * File_Execution).  */
	};

	void raise(int error_);
	/* All errors by Execution objects call this function.  Set the
	 * error code, and throw an error except with the keep-going
	 * option.  */

	Proceed execute_base_A(shared_ptr <const Dependency> dependency_link);
	/* DEPENDENCY_LINK must not be null.  In the return value, at
	 * least one bit is set.  The P_FINISHED bit indicates only that
	 * tasks related to this function are done, not the whole
	 * Execution.  */

	int get_error() const {  return error;  }

	void read_dynamic(Flags flags_this,
			  const Place_Param_Target &place_param_target,
			  vector <shared_ptr <const Dependency> > &dependencies);
	/* Read dynamic dependencies from the content of
	 * PLACE_PARAM_TARGET.  The only reason this is not static is
	 * that errors can be raised and printed correctly.
	 * Dependencies that are read are written into DEPENDENCIES,
	 * which is empty on calling.  FLAGS_THIS determines
	 * whether the -n/-0/etc. flag was used, and may also contain
	 * the -o flag to ignore a non-existing file.  */

	void print_traces(string text= "") const;
	/* Print full trace for the execution.  First the message is
	 * Printed, then all traces for it starting at this execution,
	 * up to the root execution. 
	 * TEXT may be "" to not print the first message.  */ 

	const map <Execution *, shared_ptr <const Dependency> > &get_parents() const {  return parents;  }
	
	virtual Proceed execute(shared_ptr <const Dependency> dependency_this)= 0;
	/* 
	 * Start the next job(s).  This will also terminate jobs when
	 * they don't need to be run anymore, and thus it can be called
	 * when K = 0 just to terminate jobs that need to be terminated.
	 * Can only return LATER in random mode. 
	 * When returning LATER, not all possible child jobs where started.  
	 * Child implementations call this implementation.  
	 * Never returns P_CONTINUE:  When everything is finished, the
	 * FINISHED bit is set.  
	 * In DONE, set those bits that have been done. 
	 * When the call is over, clear the PENDING bit. 
	 * DEPENDENCY_LINK is only null when called on the root
	 * execution, because it is the only execution that is not
	 * linked from another execution.
	 */

	virtual bool finished() const= 0;
	/* Whether the execution is completely finished */ 

	virtual bool finished(Flags flags) const= 0; 
	/* Whether the execution is finished working for the given tasks */ 

	virtual string debug_done_text() const
	/* Extra string for the "done" information; may be empty.  */
	{ 
		return ""; 
	}

	virtual string format_src() const= 0;

	virtual void notify_result(shared_ptr <const Dependency> dependency,
				   Execution *source,
				   Flags flags)
	/* The child execution SOURCE notifies THIS about a new result.
	 * Only called when the dependency linking the two had one of the
	 * F_RESULT_* flag.  The given flag contains only one of the two
	 * F_RESULT_* flags.  */ 
	{
		/* Execution classes that use F_RESULT_* override this function */
		(void) dependency;
		(void) source; 
		(void) flags;
		assert(false);
	}

	virtual void notify_variable(const map <string, string> &result_variable_child) {  
		(void) result_variable_child; 
	}

	static long jobs;
	/* Number of free slots for jobs.  This is a long because
	 * strtol() gives a long.  Set before calling main() from the -j
	 * option, and then changed internally by this class.  Always
	 * nonnegative.  */ 

	static Rule_Set rule_set; 
	/* Set once before calling Execution::main().  Unchanging during
	 * the whole call to Execution::main().  */ 

	static void main(const vector <shared_ptr <const Dependency> > &dependencies);
	/* Main execution loop.  This throws ERROR_BUILD and
	 * ERROR_LOGICAL.  */

	static Target get_target_for_cache(Target target); 
	/* Get the target value used for caching.  I.e, return TARGET
	 * with certain flags removed.  */

protected: 

	Bits bits;

	int error;
	/* Error value of this execution.  The value is propagated
	 * (using '|') to the parent.  Values correspond to constants
	 * defined in error.hh; zero denotes the absence of an
	 * error.  */ 

	map <Execution *, shared_ptr <const Dependency> > parents; 
	/* The parent executions.  This is a map rather than an
	 * unsorted_map because typically, the number of elements is
	 * always very small, i.e., mostly one, and a map is better
	 * suited in this case.  */ 

	set <Execution *> children;
	/* Currently running executions.  Allocated with operator new()
	 * and never deleted.  */ 

	Timestamp timestamp; 
	/* Latest timestamp of a (direct or indirect) dependency
	 * that was not rebuilt.  Files that were rebuilt are not
	 * considered, since they make the target be rebuilt anyway.
	 * Implementations also changes this to consider the file
	 * itself, if any.  This final timestamp is then carried over to the
	 * parent executions.  */

	vector <shared_ptr <const Dependency> > result; 
	/* The final list of dependencies represented by the target.
	 * This does not include any dynamic dependencies, i.e., all
	 * dependencies are flattened to Plain_Dependency's.  Not used
	 * for executions that have file targets, neither for
	 * executions that have multiple targets.  This is not used for
	 * file dependencies, as a file dependency's result can be each
	 * of its files, depending in the parent -- for file
	 * dependencies, parents are notified directly, bypassing
	 * push_result().  */ 

	map <string, string > result_variable; 
	/* Same semantics as RESULT, but for variable values, stored as
	 * KEY-VALUE pairs.  */

	shared_ptr <Rule> param_rule;
	/* The (possibly parametrized) rule from which this execution
	 * was derived.  This is only used to detect strong cycles.  To
	 * manage the dependencies, the instantiated general rule is
	 * used.  Null by default, and set by individual implementations
	 * in their constructor if necessary.  */ 

	Execution(shared_ptr <Rule> param_rule_= nullptr)
		:  bits(0),
		   error(0),
		   timestamp(Timestamp::UNDEFINED),
		   param_rule(param_rule_)
	{  }

	Proceed execute_children();
	/* Execute already-active children */

	Proceed execute_base_B(shared_ptr <const Dependency> dependency_link); 
	/* Second pass (trivial dependencies).  Called once we are sure
	 * that the target must be built.  Arguments and return value
	 * have the same semantics as execute_base_B().  */

	void check_waited() const {
		assert(buffer_A.empty()); 
		assert(buffer_B.empty()); 
		assert(children.empty()); 
	}

	const Buffer &get_buffer_A() const {  return buffer_A;  }
	const Buffer &get_buffer_B() const {  return buffer_B;  }

	void push(shared_ptr <const Dependency> );
	/* Push a dependency to the default buffer, breaking down non-normalized
	 * dependencies while doing so.  */

	void push_result(shared_ptr <const Dependency> dd); 
	void disconnect(Execution *const child,
			shared_ptr <const Dependency> dependency_child);
	/* Remove an edge from the dependency graph.  Propagate
	 * information from CHILD to THIS, and then delete CHILD if
	 * necessary.  */

	const Place &get_place() const 
	/* The place for the execution; e.g. the rule; empty if there is no place */
	{
		if (param_rule == nullptr)
			return Place::place_empty;
		else
			return param_rule->place; 
	}

	virtual ~Execution(); 

	virtual int get_depth() const= 0;
	/* The dynamic depth, or -1 when undefined as in concatenated
	 * executions and the root execution, in which case PARAM_RULE
	 * is always null.  Only used to check for cycles on the rule
	 * level.  */ 

	virtual bool optional_finished(shared_ptr <const Dependency> dependency_link)= 0;
	/* Whether the execution would be finished if this was an
	 * optional dependency.  Check whether this is an optional
	 * dependency and if it is, return TRUE when the file does not
	 * exist.  Return FALSE when children should be started.  Return
	 * FALSE in execution types that are not affected.  */

	virtual bool want_delete() const= 0; 

	static Timestamp timestamp_last; 
	/* The timepoint of the last time wait() returned.  No file in the
	 * file system should be newer than this.  */ 

	static bool hide_out_message;
	/* Whether to show a STDOUT message at the end */

	static bool out_message_done;
	/* Whether the STDOUT message is not "Targets are up to date" */

	static unordered_map <Target, Execution *> executions_by_target;
	/* All cached Execution objects by each of their Target.  Such
	 * Execution objects are never deleted.  */

	static bool find_cycle(Execution *parent,
			       Execution *child,
			       shared_ptr <const Dependency> dependency_link);
	/* Find a cycle.  Assuming that the edge parent->child will be
	 * added, find a directed cycle that would be created.  Start at
	 * PARENT and perform a depth-first search upwards in the
	 * hierarchy to find CHILD.  DEPENDENCY_LINK is the link that
	 * would be added between child and parent, and would create a
	 * cycle.  */

	static bool find_cycle(vector <Execution *> &path,
			       Execution *child,
			       shared_ptr <const Dependency> dependency_link); 
	/* Helper function.  PATH is the currently explored path.
	 * PATH[0] is the original PARENT; PATH[end] is the oldest
	 * grandparent found yet.  */ 

	static void cycle_print(const vector <Execution *> &path,
				shared_ptr <const Dependency> dependency);
	/* Print the error message of a cycle on rule level.
	 * Given PATH = [a, b, c, d, ..., x], the found cycle is
	 * [x <- a <- b <- c <- d <- ... <- x], where A <- B denotes
	 * that A is a dependency of B.  For each edge in this cycle,
	 * output one line.  DEPENDENCY is the link (x <- a), which is not yet
	 * created in the execution objects.  All other link
	 * dependencies are read from the execution objects.  */ 

	static bool same_rule(const Execution *execution_a,
			      const Execution *execution_b);
	/* Whether both executions have the same parametrized rule.
	 * Only used for finding cycle.  */ 

private: 

	Buffer buffer_A;
	/* Dependencies that have not yet begun to be built.
	 * Initialized with all dependencies, and emptied over time when
	 * things are built, and filled over time when dynamic
	 * dependencies are worked on.  Entries are not necessarily
	 * unique.  Does not contain compound dependencies, except under
	 * concatenating ones.  */  

	Buffer buffer_B; 
	/* The buffer for dependencies in the second pass.  They are
	 * only started if, after (potentially) starting all non-trivial
	 * dependencies, the target must be rebuilt anyway.  Does not
	 * contain compound dependencies.  */

	Proceed connect(shared_ptr <const Dependency> dependency_link_parent,
			shared_ptr <const Dependency> dependency_child);
	/* Add an edge to the dependency graph.  Deploy a new child
	 * execution.  LINK is the link from the THIS's parent to THIS.
	 * Note: the top-level flags of LINK.DEPENDENCY may be modified.
	 * DEPENDENCY_CHILD must be normalized.  */

	Execution *get_execution(shared_ptr <const Dependency> dependency);
	/* Get an existing Execution or create a new one for the
	 * given DEPENDENCY.  Return null when a strong cycle was found;
	 * return the execution otherwise.  PLACE is the place of where
	 * the dependency was declared.  */

	static void copy_result(Execution *parent, Execution *child); 
	/* Copy the result list from CHILD to PARENT */

	static bool hide_link_from_message(Flags flags) {
		return flags & F_RESULT_NOTIFY; 
	}
};

class File_Execution
/*
 * Each non-dynamic file target is represented at run time by one
 * File_Execution object.  Each File_Execution object may correspond to
 * multiple files or transients, when a rule has multiple targets.
 * Transients are only represented by a File_Execution when they appear
 * as targets of rules that have at least one file target, or when the
 * rule has a command.  Otherwise, Transient_Execution is used for them.
 *
 * This is the only Execution subclass that actually starts jobs -- all
 * other Execution subclasses only delegate their tasks to child
 * executions. 
 */
	:  public Execution 
{
public:

	File_Execution(shared_ptr <const Dependency> dependency_link,
		       Execution *parent,
		       shared_ptr <Rule> rule,
		       shared_ptr <Rule> param_rule,
		       map <string, string> &mapping_parameter_,
		       int &error_additional);
	/* ERROR_ADDITIONAL indicates whether an error will be thrown
	 * after the call.  (Because an error can only be thrown after
	 * the execution has been connected to a parent, which is not
	 * done in the constructor.  The parent is connected to this iff
	 * ERROR_ADDITIONAL is zero after the call.  */

	void read_variable(shared_ptr <const Dependency> dependency); 
	/* Read the content of the file into a string as the
	 * variable value.  THIS is the variable execution.  Write the
	 * result into THIS's RESULT_VARIABLE.  */

	shared_ptr <const Rule> get_rule() const { return rule; }

	const map <string, string> &get_mapping_variable() const {
		return mapping_variable; 
	}

	virtual string debug_done_text() const {
		return flags_format(flags_finished);
	}

	virtual Proceed execute(shared_ptr <const Dependency> dependency_this);
	virtual bool finished() const;
	virtual bool finished(Flags flags) const; 
	virtual string format_src() const {
		assert(targets.size()); 
		return targets.front().format_src(); 
	}
	virtual void notify_variable(const map <string, string> &result_variable_child) {  
		mapping_variable.insert(result_variable_child.begin(), result_variable_child.end()); 
	}

	static unordered_map <pid_t, File_Execution *> executions_by_pid;
	/*
	 * The currently running executions by process IDs.  Write
	 * access to this is enclosed in a Signal_Blocker.
	 */ 

	static void wait();
	/* Wait for next job to finish and finish it.  Do not start anything
	 * new.  */ 

protected:

	virtual bool optional_finished(shared_ptr <const Dependency> dependency_link);
	virtual bool want_delete() const {  return false;  }
	virtual int get_depth() const {  return 0;  }

private:

	friend class Execution; 
	friend void job_terminate_all(); 
	friend void job_print_jobs(); 

	vector <Target> targets; 
	/* The targets to which this execution object corresponds.
	 * Never empty.  
	 * All targets are non-dynamic, i.e., only plain files and
	 * transients are included.   
	 * Does not include flags (except F_TRANSIENT).  */

	shared_ptr <Rule> rule;
	/* The instantiated file rule for this execution.  Null when
	 * there is no rule for this file (this happens for instance
	 * when a source code file is given as a dependency).
	 * Individual dynamic dependencies do have rules, in order for
	 * cycles to be detected.  Null if and only if PARAM_RULE is
	 * null.  */ 

	Job job;
	/* The job used to execute this rule's command */ 

	vector <Timestamp> timestamps_old; 
	/* Timestamp of each file target, before the command is
	 * executed.  Only valid once the job was started.  The indexes
	 * correspond to those in TARGETS.  Non-file indexes are
	 * uninitialized.  Used for checking whether a file was rebuild
	 * to decide whether to remove it after a command failed or was
	 * interrupted.  This is UNDEFINED when the file did not exist,
	 * or no target is a file.  */ 

	map <string, string> mapping_parameter; 
	/* Variable assignments from parameters for when the command is run */

	map <string, string> mapping_variable; 
	/* Variable assignments from variables dependencies */

	Flags flags_finished; 
	/* What parts of this target have been done.  Each bit that is
	 * set represents one aspect that was done.  When an execution
	 * is invoked with a certain set of flags, all flags *not*
	 * passed will be set when the execution is finished.  Only the
	 * first C_FINISHABLE flags are used.  */

	~File_Execution(); 

	bool remove_if_existing(bool output); 
	/* Remove all file targets of this execution object if they
	 * exist.  If OUTPUT is true, output a corresponding message.
	 * Return whether the file was removed.  If OUTPUT is false,
	 * only do async signal-safe things.  */  

	void waited(pid_t pid, int status); 
	/* Called after the job was waited for.  The PID is only passed
	 * for checking that it is correct.  */

	void warn_future_file(struct stat *buf, 
			      const char *filename,
			      const Place &place,
			      const char *message_extra= nullptr);
	/* Warn when the file has a modification time in the future.
	 * MESSAGE_EXTRA may be null to not show an extra message.  */ 

	void print_command() const; 
	/* Print the command and its associated variable assignments,
	 * according to the selected verbosity level.  */

	void print_as_job() const;
	/* Print a line to stdout for a running job, as output of SIGUSR1.
	 * Is currently running.  */ 

	void write_content(const char *filename, const Command &command); 
	/* Create the file FILENAME with content from COMMAND */

	static unordered_map <string, Timestamp> transients;
	/* The timestamps for transient targets.  This container plays
	 * the role of the file system for transient targets, holding
	 * their timestamps, and remembering whether they have been
	 * executed.  Note that if a rule has both file targets and
	 * transient targets, and all file targets are up to date and
	 * the transient targets have all their dependencies up to date,
	 * then the command is not executed, even though it was never
	 * executed in the current invocation of Stu. In that case, the
	 * transient targets are never inserted in this map.  */
};

class Transient_Execution
/* 
 * Used for non-dynamic transients that appear in rules that have only
 * transients as targets, and have no command.  If at least one file
 * target or a command is present in the rule, File_Execution is used.
 */
	:  public Execution 
{
public:

	Transient_Execution(shared_ptr <const Dependency> dependency_link,
			    Execution *parent,
			    shared_ptr <Rule> rule,
			    shared_ptr <Rule> param_rule,
			    map <string, string> &mapping_parameter,
			    int &error_additional);

	shared_ptr <const Rule> get_rule() const { return rule; }

	const map <string, string> &get_mapping_variable() const {
		return mapping_variable; 
	}

	virtual Proceed execute(shared_ptr <const Dependency> dependency_this);
	virtual bool finished() const;
	virtual bool finished(Flags flags) const; 
	virtual string format_src() const;
	virtual void notify_result(shared_ptr <const Dependency> dependency, Execution *, Flags flags);
	virtual void notify_variable(const map <string, string> &result_variable_child) {  
		result_variable.insert(result_variable_child.begin(), result_variable_child.end()); 
	}

protected:

	virtual bool want_delete() const {  return false;  }
	virtual int get_depth() const {  return 0;  }
	virtual bool optional_finished(shared_ptr <const Dependency> dependency_link) {  
		(void) dependency_link; 
		return false;  
	}

private:

	vector <Target> targets; 
	/* The targets to which this execution object corresponds.  All
	 * are transients.  Contains at least one element.  */

	shared_ptr <Rule> rule;
	/* The instantiated file rule for this execution.  Never null. */ 

	Timestamp timestamp_old;

	bool is_finished; 

	map <string, string> mapping_parameter; 
	/* Contains the parameters; is not used */

	map <string, string> mapping_variable; 
	/* Variable assignments from variables dependencies.  This is in
	 * Transient_Execution because it may be percolated up to the
	 * parent execution.  */

	~Transient_Execution();
};

class Root_Execution
	:  public Execution
{
public:

	Root_Execution(const vector <shared_ptr <const Dependency> > &dependencies); 

	virtual Proceed execute(shared_ptr <const Dependency> dependency_this);
	virtual bool finished() const; 
	virtual bool finished(Flags flags) const;
	virtual string format_src() const { return "ROOT"; }

protected:

	virtual int get_depth() const {  return -1;  }
	virtual bool optional_finished(shared_ptr <const Dependency> ) {  return false;  }
	virtual bool want_delete() const {  return true;  }

private:

	bool is_finished; 
};

class Concatenated_Execution
/* 
 * An execution representating a concatenation.  Its dependency is
 * always a compound dependency containing normalized dependencies, whose
 * results are concatenated as new targets added to the parent.
 *
 * Dynamic concatenations are handled by Dynamic_Execution, not by this class. 
 *
 * Concatenated executions always have exactly one parent.  They are not
 * cached, and they are deleted when done.  Thus, they also don't need
 * the 'done' field.  (But the parent class has it.)
 */
	:  public Execution
{
public:

	Concatenated_Execution(shared_ptr <const Concatenated_Dependency> dependency_,
			       shared_ptr <const Dependency> dependency_link,
			       Execution *parent,
			       bool &found_cycle);
	/* The given dependency must be normalized, and contain at least
	 * one Concatenated_Dependency.  */

	~Concatenated_Execution(); 

	void add_part(shared_ptr <Plain_Dependency> dependency, 
		      int concatenation_index);
	/* Add a single part -- exclude the outer layer */

	void assemble_parts(); 

	virtual int get_depth() const { return -1; }
	virtual Proceed execute(shared_ptr <const Dependency> dependency_this);
	virtual bool finished() const;
	virtual bool finished(Flags flags) const; 
	virtual string format_src() const {  return "CONCAT";  }

protected:

	virtual bool optional_finished(shared_ptr <const Dependency> ) {  return false;  }

private:

	shared_ptr <const Concatenated_Dependency> dependency;
	/* Contains the concatenation. 
	 * This is a Concatenated_Dependency,
	 * itself containing each a Compound_Dependency^{0,1} of
	 * Dynamic_Dependency^* of a plain dependency. 
	 * Is normalized.  */

	int stage;
	/* 0:  Nothing done yet. 
	 *  --> put dependencies into the queue
	 * 1:  We're building the normal dependencies.
	 *  --> read out the dependencies and construct the list of actual dependencies
	 * 2:  Building actual dependencies.
	 * 3:  Finished.  */

	vector <vector <shared_ptr <const Plain_Dependency> > > parts; 
	/* The individual parts, inserted here during stage 1 by
	 * disconnect().  Excludes the outer layer.  */

	void add_stage0_dependency(shared_ptr <const Dependency> d, unsigned concatenate_index);
	/* Add a dependency during Stage 0.  The given dependency can be
	 * non-normalized, because it comes from within a concatenated
	 * dependency.  */

	virtual bool want_delete() const {  return true;  }

	static shared_ptr <const Dependency> concatenate_dependency_one(shared_ptr <const Plain_Dependency> dependency_1,
									shared_ptr <const Plain_Dependency> dependency_2,
									Flags dependency_flags);
	/* Concatenate to two given dependencies, additionally
	 * adding the given flags.  */
};

class Dynamic_Execution
/*
 * This is used for all dynamic targets, regardless of whether they are
 * files, transients, or concatenations. 
 *
 * If it corresponds to a (possibly multiply) dynamic transient or file,
 * it used for caching and is not deleted.  If it corresponds to a
 * concatenation, it is not cached, and is deleted when not used anymore.
 *
 * Each dynamic execution corresponds to an exact dynamic dependency,
 * taking into account all flags.  This is as opposed to file
 * executions, where multiple file dependencies share a single execution
 * object. 
 *
 * The implementation:  we have exactly one Result_Execution as a child,
 * which generates the list of dependencies that we are then adding as
 * children to ourselves. 
 */
	:  public Execution 
{
public:

	Dynamic_Execution(shared_ptr <const Dynamic_Dependency> dependency_,
			  Execution *parent,
			  int &error_additional); 

	shared_ptr <const Dynamic_Dependency> get_dependency() const {  return dependency;  }

	virtual Proceed execute(shared_ptr <const Dependency> dependency_this);
	virtual bool finished() const;
	virtual bool finished(Flags flags) const; 
	virtual int get_depth() const {  return dependency->get_depth();  }
	virtual bool optional_finished(shared_ptr <const Dependency> ) {  return false;  }
	virtual string format_src() const;
	virtual void notify_variable(const map <string, string> &result_variable_child) {  
		result_variable.insert(result_variable_child.begin(), result_variable_child.end()); 
	}
	virtual void notify_result(shared_ptr <const Dependency> dependency, Execution *source, Flags flags);

protected:

	virtual bool want_delete() const;

private: 

	const shared_ptr <const Dynamic_Dependency> dependency; 
	/* A dynamic of anything */

	bool is_finished; 
};

class Debug
/* 
 * Padding for debug output (option -d).  During the lifetime of an
 * object, padding is increased by one step.  
 */
{
public:
	Debug(Execution *e) 
	{
		padding_current += "   ";
		executions.push_back(e); 
	}

	~Debug() 
	{
		padding_current.resize(padding_current.size() - 3);
		executions.pop_back(); 
	}

	static const char *padding() {
		return padding_current.c_str(); 
	}

	static void print(Execution *, string text);
	/* Print a line for debug mode.  The given TEXT starts with the
	 * lower-case name of the operation being performed, followed by
	 * parameters, and not ending in a newline or period.  */

private:
	static string padding_current;
	static vector <Execution *> executions; 

	static void print(string text_target, string text);
};

long Execution::jobs= 1;
Rule_Set Execution::rule_set; 
Timestamp Execution::timestamp_last;
bool Execution::hide_out_message= false;
bool Execution::out_message_done= false;
unordered_map <Target, Execution *> Execution::executions_by_target;

unordered_map <pid_t, File_Execution *> File_Execution::executions_by_pid;
unordered_map <string, Timestamp> File_Execution::transients;

string Debug::padding_current= "";
vector <Execution *> Debug::executions; 

Execution::~Execution()
{
	/* Nop */
}

void Execution::main(const vector <shared_ptr <const Dependency> > &dependencies)
{
	assert(jobs >= 0);
	timestamp_last= Timestamp::now(); 
	Root_Execution *root_execution= new Root_Execution(dependencies); 
	int error= 0; 
	shared_ptr <const Root_Dependency> dependency_root= make_shared <Root_Dependency> (); 

	try {
		while (! root_execution->finished()) {
			Proceed proceed;
			do {
				Debug::print(nullptr, "loop"); 
				proceed= root_execution->execute(dependency_root);
				assert(proceed); 
			} while (proceed & P_PENDING); 

			if (proceed & P_WAIT) {
				File_Execution::wait();
			}
		}

		assert(root_execution->finished()); 
		assert(File_Execution::executions_by_pid.size() == 0);

		bool success= (root_execution->error == 0);
		assert(option_keep_going || success); 

		error= root_execution->error; 
		assert(error >= 0 && error <= 3); 

		if (success) {
			if (! hide_out_message) {
				if (out_message_done)
					print_out("Build successful");
				else 
					print_out("Targets are up to date");
			}
		} else {
			if (option_keep_going) {
				print_error_reminder("Targets not up to date because of errors");
			}
		}
	} 

	/* A build error is only thrown when option_keep_going is
	 * not set */ 
	catch (int e) {
		assert(! option_keep_going); 
		assert(e >= 1 && e <= 4); 
		/* Terminate all jobs */ 
		if (File_Execution::executions_by_pid.size()) {
			print_error_reminder("Terminating all jobs"); 
			job_terminate_all();
		}
		assert(e > 0 && e < ERROR_FATAL); 
		error= e; 
	}

	if (error)
		throw error; 
}

void Execution::read_dynamic(Flags flags_this, 
			     const Place_Param_Target &place_param_target,
			     vector <shared_ptr <const Dependency> > &dependencies)
{
	assert(place_param_target.place_name.get_n() == 0); 
	assert((place_param_target.flags & F_TARGET_TRANSIENT) == 0); 
	const Target target= place_param_target.unparametrized(); 
	assert(target.is_file()); 
	assert(dependencies.empty()); 

	string filename= target.get_name_nondynamic();

	if (! (flags_this & (F_NEWLINE_SEPARATED | F_NUL_SEPARATED))) {

		/* Parse dynamic dependency in full Stu syntax */ 

		vector <shared_ptr <Token> > tokens;
		Place place_end; 

		Tokenizer::parse_tokens_file
			(tokens, 
			 Tokenizer::DYNAMIC,
			 place_end, 
			 filename, 
			 place_param_target.place,
			 -1,
			 flags_this & F_OPTIONAL); 

		Place_Name input; /* remains empty */ 
		Place place_input; /* remains empty */ 

		try {
			Parser::get_expression_list(dependencies, tokens, 
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
				    target_dynamic.format_word(),
				    prefix_format_word(input.raw(), "<")); 
			Target target_file= target;
			target_file.get_front_byte_nondynamic() &= ~F_TARGET_TRANSIENT; 
			print_traces(fmt("%s is declared here",
					 target_file.format_word())); 
			raise(ERROR_LOGICAL);
		}
	end_normal:;

	} else {
		/* Delimiter-separated */

		/* We use getdelim() for parsing.  A more optimized way
		 * would be via mmap()+strchr(), but why the
		 * complexity?  */ 
			
		const char c= (flags_this & F_NEWLINE_SEPARATED) ? '\n' : '\0';
		/* The delimiter */ 

		const char c_printed= (flags_this & F_NEWLINE_SEPARATED) ? 'n' : '0';
		/* The character to print as the delimiter */
		
		char *lineptr= nullptr;
		size_t n= 0;
		ssize_t len;
		int line= 0; 
			
		FILE *file= fopen(filename.c_str(), "r"); 
		if (file == nullptr) {
			print_error_system(filename); 
			raise(ERROR_BUILD); 
			goto end;
		}

		while ((len= getdelim(&lineptr, &n, c, file)) >= 0) {
				
			Place place(Place::Type::INPUT_FILE, filename, ++line, 0); 

			assert(lineptr[len] == '\0'); 

			if (len == 0) {
				/* Should not happen by the specification
				 * of getdelim(), so abort parse.  */ 
				assert(false); 
				break;
			} 

			/* There may or may not be a terminating \n or \0.
			 * getdelim(3) will include it if it is present,
			 * but the file may not have one for the last entry.  */ 

			if (lineptr[len - 1] == c) {
				--len; 
			}

			/* An empty line: This corresponds to an empty
			 * filename, and thus we treat is as a syntax
			 * error, because filenames can never be
			 * empty.  */ 
			if (len == 0) {
				free(lineptr); 
				fclose(file); 
				place << "filename must not be empty"; 
				print_traces(fmt("in %s-separated dynamic dependency "
						 "declared with flag %s",
						 c == '\0' ? "zero" : "newline",
						 multichar_format_word
						 (frmt("-%c", c_printed))));
				throw ERROR_LOGICAL; 
			}
				
			string filename_dependency= string(lineptr, len); 

			dependencies.push_back
				(make_shared <Plain_Dependency>
				 (0,
				  Place_Param_Target
				  (0, 
				   Place_Name(filename_dependency, place)))); 
		}
		free(lineptr); 
		if (fclose(file)) {
			print_error_system(filename); 
			raise(ERROR_BUILD);
		}
	end:;
	}

	
	/* 
	 * Perform checks on forbidden features in dynamic dependencies.
	 * In keep-going mode (-k), we set the error, set the erroneous
	 * dependency to null, and at the end prune the null entries. 
	 */
	bool found_error= false; 
	for (auto &j:  dependencies) {

		/* Check that it is unparametrized */ 
		if (! j->is_unparametrized()) {
			shared_ptr <const Dependency> dep= j;
			while (dynamic_pointer_cast <const Dynamic_Dependency> (dep)) {
				shared_ptr <const Dynamic_Dependency> dep2= 
					dynamic_pointer_cast <const Dynamic_Dependency> (dep);
				dep= dep2->dependency; 
			}
			dynamic_pointer_cast <const Plain_Dependency> (dep)
				->place_param_target.place_name.places[0] <<
				fmt("dynamic dependency %s must not contain parametrized dependencies",
				    Target(0, target).format_word());
			Target target_base= target;
			target_base.get_front_byte_nondynamic() &= ~F_TARGET_TRANSIENT; 
			target_base.get_front_byte_nondynamic() |= (target.get_front_byte_nondynamic() & F_TARGET_TRANSIENT); 
			print_traces(fmt("%s is declared here", 
					 target_base.format_word())); 
			raise(ERROR_LOGICAL);
			j= nullptr;
			found_error= true; 
			continue; 
		}
	}
	if (found_error) {
		assert(option_keep_going); 
		vector <shared_ptr <const Dependency> > dependencies_new;
		for (auto &j:  dependencies) {
			if (j)
				dependencies_new.push_back(j); 
		}
		swap(dependencies, dependencies_new); 
	}
}

bool Execution::find_cycle(Execution *parent, 
			   Execution *child,
			   shared_ptr <const Dependency> dependency_link)
{
	vector <Execution *> path;
	path.push_back(parent); 
	return find_cycle(path, child, dependency_link); 
}

bool Execution::find_cycle(vector <Execution *> &path,
			   Execution *child,
			   shared_ptr <const Dependency> dependency_link)
{
	if (same_rule(path.back(), child)) {
		cycle_print(path, dependency_link); 
		return true; 
	}

	for (auto &i:  path.back()->parents) {
		Execution *next= i.first; 
		assert(next != nullptr);
		path.push_back(next); 
		bool found= find_cycle(path, child, dependency_link);
		if (found)
			return true;
		path.pop_back(); 
	}
	
	return false; 
}

void Execution::cycle_print(const vector <Execution *> &path,
			    shared_ptr <const Dependency> dependency)
/*
 * Given PATH = [a, b, c, d, ..., x], we print:
 *
 * 	x depends on ...      \
 *      ... depends on d      |
 *      d depends on c        | printed from PATH
 *      c depends on b        |
 *      b depends on a        /
 *      a depends on x        > printed from DEPENDENCY
 *      x is needed by ...    \ 
 *      ...                   | printed by print_traces() 
 *      ...                   /
 */
{
	assert(path.size() > 0); 

	/* Indexes are parallel to PATH */ 
	vector <string> names;
	names.resize(path.size());
	
	for (size_t i= 0;  i + 1 < path.size();  ++i) {
		names[i]= path[i]->parents.at((Execution *) path[i+1])
			->get_target().format_word();
	}
	names.back()= path.back()->parents.begin()->second
		->get_target().format_word();
		
	for (ssize_t i= path.size() - 1;  i >= 0;  --i) {

		shared_ptr <const Dependency> d= i == 0 
			? dependency
			: path[i - 1]->parents.at(const_cast <Execution *> (path[i])); 

		/* Don't show a message for left-branch dynamic links */ 
		if (hide_link_from_message(d->flags)) {
			continue;
		}

		d->get_place()
			<< fmt("%s%s depends on %s",
			       i == (ssize_t)(path.size() - 1) 
			       ? (path.size() == 1 
				  || (path.size() == 2 && hide_link_from_message(dependency->flags))
				  ? "target must not depend on itself: " 
				  : "cyclic dependency: ") 
			       : "",
			       names[i],
			       i == 0 ? dependency->get_target().format_word() : names[i - 1]);
	}

	/* If the two targets are different (but have the same rule
	 * because they match the same pattern and/or because they are
	 * two different targets of a multitarget rule), then output a
	 * notice to that effect */ 
	Target t1= path.back()->parents.begin()->second->get_target();
	Target t2= dependency->get_target();
	const char *c1= t1.get_name_c_str_any();
	const char *c2= t2.get_name_c_str_any();
	if (strcmp(c1, c2)) {
		path.back()->get_place() 
			<<
			fmt("both %s and %s match the same rule",
			    name_format_word(c1), name_format_word(c2));
	}

	/* Remove the offending (cycle-generating) link between the
	 * two.  The offending link is from path[0] as a parent to
	 * path[end] (as a child).  */
	path.back()->parents.erase(path.at(0)); 
	path.at(0)->children.erase(path.back()); 

	path.back()->print_traces();

	explain_cycle(); 
}

bool Execution::same_rule(const Execution *execution_a,
			  const Execution *execution_b)
/* 
 * This must also take into account that two execution could use the
 * same rule but parametrized differently, thus the two executions could
 * have different targets, but the same rule. 
 */ 
{
	return 
		execution_a->param_rule != nullptr &&
		execution_b->param_rule != nullptr &&
		execution_a->get_depth() == execution_b->get_depth() &&
		execution_a->param_rule == execution_b->param_rule;
}

void Execution::print_traces(string text) const
/* The following traverses the execution graph backwards until it finds
 * the root.  We always take the first found parent, which is an
 * arbitrary choice, but it doesn't matter here which dependency path
 * we point out as an error, so the first one it is.  */
{	
	const Execution *execution= this; 

	/* If the error happens directly for the root execution, it was
	 * an error on the command line; don't output anything beyond
	 * the error message. */
	if (dynamic_cast <const Root_Execution *> (execution)) 
		return;

	bool first= true; 

	/* If there is a rule for this target, show the message with the
	 * rule's trace, otherwise show the message with the first
	 * dependency trace */ 
	if (execution->get_place().type != Place::Type::EMPTY && text != "") {
		execution->get_place() << text;
		first= false;
	}

	string text_parent= parents.begin()->second->get_target().format_word();

	while (true) {

		auto i= execution->parents.begin(); 

		if (dynamic_cast <Root_Execution *> (i->first)) {
			/* We are in a child of the root execution */ 
			if (first && text != "") {
				/* No text was printed yet, but there
				 * was a TEXT passed:  Print it with the
				 * place available.  */ 
				/* This is a top-level target, i.e.,
				 * passed on the command line via an
 				 * argument or an option  */
				i->second->get_place() <<
					fmt("no rule to build %s", text_parent);
			}
			break; 
		}

		string text_child= text_parent; 

		text_parent= i->first->parents.begin()->second->get_target().format_word();

 		const Place place= i->second->get_place();
		/* Set even if not output, because it may be used later
		 * for the root target  */

		/* Don't show left-branch edges of dynamic executions */
		if (hide_link_from_message(i->second->flags)) {
			execution= i->first; 
			continue;
		}

		string msg;
		if (first && text != "") {
			msg= fmt("%s, needed by %s", text, text_parent); 
			first= false;
		} else {	
			msg= fmt("%s is needed by %s",
				 text_child, text_parent);
		}
		place << msg;
		
		execution= i->first; 
	}
}

Proceed Execution::execute_children()
{
	/* Since disconnect() may change execution->children, we must first
	 * copy it over locally, and then iterate through it */ 

	vector <Execution *> executions_children_vector
		(children.begin(), children.end()); 

	Proceed proceed_all= 0;

	while (! executions_children_vector.empty()) {

		assert(jobs >= 0);

		if (order_vec) {
			/* Exchange a random position with last position */ 
			size_t p_last= executions_children_vector.size() - 1;
			size_t p_random= random_number(executions_children_vector.size());
			if (p_last != p_random) {
				swap(executions_children_vector[p_last],
				     executions_children_vector[p_random]); 
			}
		}

		Execution *child= executions_children_vector.at
			(executions_children_vector.size() - 1);
		executions_children_vector.resize(executions_children_vector.size() - 1); 
		
		assert(child != nullptr);

		shared_ptr <const Dependency> dependency_child= child->parents.at(this);

		Proceed proceed_child= child->execute(dependency_child);
		assert(proceed_child); 

		proceed_all |= (proceed_child & ~(P_FINISHED | P_ABORT));
		/* The finished and abort flags of the child only apply to the
		 * child, not to us  */

		assert(((proceed_child & P_FINISHED) == 0) == 
		       ((child->finished(dependency_child->flags)) == 0));

		if (proceed_child & P_FINISHED) {
			disconnect(child, dependency_child); 
		} else {
			assert((proceed_child & ~P_FINISHED) != 0); 
			/* If the child execution is not finished, it
			 * must have returned either the P_WAIT or
			 * P_PENDING bit.  */
		}
	}

	if (error) {
		assert(option_keep_going); 
		/* Otherwise, Stu would have aborted */ 
	}

	if (proceed_all == 0) {
		/* If there are still children, they must have returned
		 * WAIT or PENDING */ 
		assert(children.empty()); 
		if (error) {
			assert(option_keep_going); 
		}
	}

	return proceed_all; 
}

void Execution::push(shared_ptr <const Dependency> dependency)
{
	assert(dependency); 
	dependency->check();
	
	vector <shared_ptr <const Dependency> > dependencies;
	Dependency::make_normalized(dependencies, dependency); 
       
	for (const auto &d:  dependencies) {
		d->check(); 
		buffer_A.push(d);
	}
}

Proceed Execution::execute_base_A(shared_ptr <const Dependency> dependency_this)
{
	Debug debug(this);

	assert(jobs >= 0); 
	assert(dependency_this); 

	shared_ptr <Dependency> dependency_this2= Dependency::clone(dependency_this); 
	Proceed proceed= 0; 

	if (finished(dependency_this2->flags)) {
		Debug::print(this, "finished"); 
		return proceed |= P_FINISHED; 
	}

	if (optional_finished(dependency_this2)) {
		Debug::print(this, "finished"); 
		return proceed |= P_FINISHED; 
	}

	/* In DFS mode, first continue the already-open children, then
	 * open new children.  In random mode, start new children first
	 * and continue already-open children second */ 
	/* Continue the already-active child executions */  
	if (order != Order::RANDOM) {
		Proceed proceed_2= execute_children();
		proceed |= proceed_2;
		if (proceed & P_WAIT) {
			if (jobs == 0) 
				return proceed; 
		} else if (finished(dependency_this2->flags) && ! option_keep_going) {
			Debug::print(this, "finished"); 
			return proceed |= P_FINISHED;
		}
	} 

	/* Is this a trivial run?  Then skip the dependency. */
	if (dependency_this2->flags & F_TRIVIAL) {
		return proceed |= P_ABORT | P_FINISHED; 
	}

	assert(error == 0 || option_keep_going); 

	/* 
	 * Deploy dependencies (first pass), with the F_NOTRIVIAL flag
	 */ 

	if (jobs == 0) {
		return proceed |= P_WAIT;
	}

	while (! buffer_A.empty()) {
		shared_ptr <const Dependency> dependency_child= buffer_A.next(); 
		if ((dependency_child->flags & (F_RESULT_NOTIFY | F_TRIVIAL)) == F_TRIVIAL) {
			shared_ptr <Dependency> dependency_child_2= 
				Dependency::clone(dependency_child);
			dependency_child_2->flags &= ~F_TRIVIAL; 
			dependency_child_2->get_place_flag(I_TRIVIAL)= Place::place_empty; 
			buffer_B.push(dependency_child_2); 
		}
		Proceed proceed_2= connect(dependency_this2, dependency_child);
		proceed |= proceed_2;
		if (jobs == 0) {
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
		assert(option_keep_going == true); 
		return proceed |= P_ABORT | P_FINISHED; 
	}

	if (proceed)
		return proceed; 

	return proceed |= P_FINISHED; 
}

Proceed Execution::connect(shared_ptr <const Dependency> dependency_this,
				      shared_ptr <const Dependency> dependency_child)
{
	Debug::print(this, fmt("connect %s",  dependency_child->format_src())); 

	assert(dependency_child->is_normalized()); 
	assert(! dynamic_pointer_cast <const Root_Dependency> (dependency_child)); 

	shared_ptr <const Plain_Dependency> plain_dependency_this=
		dynamic_pointer_cast <const Plain_Dependency> (dependency_this);

	/*
	 * Check for various invalid types of connections 
	 */

	/* '-p' and '-o' do not mix */ 
	if (dependency_child->flags & F_PERSISTENT && dependency_child->flags & F_OPTIONAL) {

		/* '-p' and '-o' encountered for the same target */ 
		const Place &place_persistent= 
			dependency_child->get_place_flag(I_PERSISTENT);
		const Place &place_optional= 
			dependency_child->get_place_flag(I_OPTIONAL);
		place_persistent <<
			fmt("declaration of persistent dependency with %s",
			    multichar_format_word("-p")); 
		place_optional <<
			fmt("clashes with declaration of optional dependency with %s",
			    multichar_format_word("-o")); 
		dependency_child->get_place() <<
			fmt("in declaration of dependency %s, needed by %s", 
			    dependency_child->get_target().format_word(),
			    dependency_this->get_target().format_word()); 
		print_traces();
		explain_clash(); 
		raise(ERROR_LOGICAL);
		return 0;
	}

	/* '-o' does not mix with '$[' */
	if (dependency_child->flags & F_VARIABLE && dependency_child->flags & F_OPTIONAL) {
		shared_ptr <const Plain_Dependency> plain_dependency_child=
			dynamic_pointer_cast <const Plain_Dependency> (dependency_child); 
		assert(plain_dependency_child); 
		assert(!(dependency_child->flags & F_TARGET_TRANSIENT)); 
		const Place &place_variable= dependency_child->get_place();
		const Place &place_flag= dependency_child->get_place_flag(I_OPTIONAL); 
		place_variable << 
			fmt("variable dependency %s must not be declared "
			    "as optional dependency",
			    dynamic_variable_format_word
			    (plain_dependency_child->place_param_target.place_name.unparametrized())); 
		place_flag << fmt("using %s",
				  multichar_format_word("-o")); 
		print_traces();
		raise(ERROR_LOGICAL);
		return 0;
	}

	/*
	 * Actually do the connection 
	 */

	Execution *child= get_execution(dependency_child);  
	if (child == nullptr) {
		/* Strong cycle was found */ 
		return 0;
	}

	children.insert(child);

	if (dependency_child->flags & F_RESULT_NOTIFY) {
		for (const auto &dependency:  child->result) {
			this->notify_result(dependency, this, F_RESULT_NOTIFY); 
		}
	}

	Proceed proceed_child= child->execute(dependency_child);
	assert(proceed_child); 
	if (proceed_child & (P_WAIT | P_PENDING))
		return proceed_child; 
			
	if (child->finished(dependency_child->flags)) {
		disconnect(child, dependency_child);
	}
	
	return 0;
}

void Execution::raise(int error_)
{
	assert(error_ >= 1 && error_ <= 3); 

	error |= error_;

	if (! option_keep_going)
		throw error;
}

void Execution::disconnect(Execution *const child,
			   shared_ptr <const Dependency> dependency_child)
{
	Debug::print(this, fmt("disconnect %s", dependency_child->format_src())); 

	assert(child != nullptr); 
	assert(child != this); 
	assert(child->finished(dependency_child->flags)); 
	assert(option_keep_going || child->error == 0); 
	dependency_child->check(); 

	if (dependency_child->flags & F_RESULT_NOTIFY && dynamic_cast <File_Execution *> (child)) {
		shared_ptr <Dependency> d= Dependency::clone(dependency_child);
		d->flags &= ~F_RESULT_NOTIFY; 
		notify_result(d, child, F_RESULT_NOTIFY); 
	}

	if (dependency_child->flags & F_RESULT_PUT && dynamic_cast <File_Execution *> (child)) {
		shared_ptr <Dependency> d= Dependency::clone(dependency_child);
		d->flags &= ~F_RESULT_PUT; 
		notify_result(d, child, F_RESULT_PUT); 
	}

	/* Propagate timestamp */
	/* Don't propagate the timestamp of the dynamic dependency itself */ 
	if (! (dependency_child->flags & F_PERSISTENT) && 
	    ! (dependency_child->flags & F_RESULT_NOTIFY)) {
		if (child->timestamp.defined()) {
			if (! timestamp.defined()) {
				timestamp= child->timestamp;
			} else if (timestamp < child->timestamp) {
				timestamp= child->timestamp; 
			}
		}
	}

	/* Propagate variables */
	if ((dependency_child->flags & F_VARIABLE)) { 
		assert(dynamic_cast <File_Execution *> (child)); 
		dynamic_cast <File_Execution *> (child)->read_variable(dependency_child);
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
	    && ! (dependency_child->flags & F_PERSISTENT)
	    && ! (dependency_child->flags & F_RESULT_NOTIFY)) {
		bits |= B_NEED_BUILD; 
	}

	/* Remove the links between them */ 
	assert(children.count(child) == 1); 
	assert(child->parents.count(this) == 1);
	children.erase(child);
	child->parents.erase(this);

	/* Delete the Execution object */
	if (child->want_delete())
		delete child; 
}

Proceed Execution::execute_base_B(shared_ptr <const Dependency> dependency_link)
{
	Proceed proceed= 0;
	while (! buffer_B.empty()) {
		shared_ptr <const Dependency> dependency_child= buffer_B.next(); 
		Proceed proceed_2= connect(dependency_link, dependency_child);
		proceed |= proceed_2; 
		assert(jobs >= 0);
		if (jobs == 0) {
			return proceed |= P_WAIT; 
		}
	} 
	assert(buffer_B.empty()); 

	return proceed; 
}

Execution *Execution::get_execution(shared_ptr <const Dependency> dependency)
{
	const Target target= dependency->get_target(); 

	/* Set to the returned Execution object when one is found or created */    
	Execution *execution= nullptr; 

	const Target target_for_cache= get_target_for_cache(target); 
	auto it= executions_by_target.find(target_for_cache);

	if (it != executions_by_target.end()) {
		/* An Execution object already exists for the target */ 
		execution= it->second; 
		if (execution->parents.count(this)) {
			/* THIS and CHILD are already connected -- add the
			 * necessary flags */ 
			Flags flags= dependency->flags; 
			if (flags & ~execution->parents.at(this)->flags) {
				shared_ptr <Dependency> dependency_new= Dependency::clone(execution->parents.at(this));
				dependency_new->flags |= flags;
				dependency= dependency_new;
				/* No need to check for cycles here,
				 * because a link between the two
				 * already exists and therefore a cycle
				 * cannot be present.  */
				execution->parents[this]= dependency; 
			}
		} else {
			if (find_cycle(this, execution, dependency)) {
				raise(ERROR_LOGICAL);
				return nullptr;
			}
			/* The parent and child are not connected -- add the
			 * connection */ 
			execution->parents[this]= dependency;
		}
		return execution;
	} 

	/* Create a new Execution object */ 

	int error_additional= 0; /* Passed to the execution */
	
	if (! target.is_dynamic()) {
		/* Plain execution */ 

		shared_ptr <Rule> rule_child, param_rule_child; 
		map <string, string> mapping_parameter;
		bool use_file_execution= false;
		try {
			Target target_without_flags= target; 
			target_without_flags.get_front_byte_nondynamic() &= F_TARGET_TRANSIENT; 
			rule_child= rule_set.get(target_without_flags, 
						 param_rule_child, mapping_parameter, 
						 dependency->get_place()); 
		} catch (int e) {
			assert(e); 
			error_additional= e; 
		}
		assert((rule_child == nullptr) == (param_rule_child == nullptr)); 

		/* RULE_CHILD may be null here; this is handled in the constructors */ 
		
		/* We use a File_Execution if:  there is at
		 * least one file target in the rule OR there is
		 * a command in the rule.  When there is no
		 * rule, we consult the type of TARGET.  */

		if (target.is_file()) {
			use_file_execution= true;
		} else if (rule_child == nullptr) {
			use_file_execution= target.is_file(); /* Always FALSE */
			assert(! use_file_execution); 
		} else if (rule_child->command) {
			use_file_execution= true; 
		} else {
			for (auto &i:  rule_child->place_param_targets) {
				if ((i->flags & F_TARGET_TRANSIENT) == 0) 
					use_file_execution= true; 
			}
		}
		
		if (use_file_execution) {

			execution= new File_Execution
				(dependency, 
				 this,
				 rule_child, 
				 param_rule_child, 
				 mapping_parameter,
				 error_additional); 
		} else if (target.is_transient()) {
			execution= new Transient_Execution
				(dependency, 
				 this,
				 rule_child, param_rule_child, mapping_parameter,
				 error_additional);
		}
	} else {
		shared_ptr <const Dynamic_Dependency> dynamic_dependency=
			dynamic_pointer_cast <const Dynamic_Dependency> (dependency); 
		execution= new Dynamic_Execution(dynamic_dependency, 
						 this,
						 error_additional); 
	}

	if (error_additional) {
		error |= error_additional; 
		if (execution->want_delete())
			delete execution; 
		return nullptr; 
	}
	
	assert(execution->parents.size() == 1); 

	return execution;
}

void Execution::copy_result(Execution *parent, Execution *child)
{
	/* Check that the child is not of a type for which RESULT is not
	 * used */
	if (dynamic_cast <File_Execution *> (child)) {
		File_Execution *file_child= dynamic_cast <File_Execution *> (child);
		assert(file_child->targets.size() == 1 &&
		       file_child->targets.at(0).is_transient()); 
	}

	for (auto &i:  child->result) {
		parent->result.push_back(i);
	}
}

void Execution::push_result(shared_ptr <const Dependency> dd)
{
	Debug::print(this, fmt("push_result %s", dd->format_src())); 

	assert(! dynamic_cast <File_Execution *> (this)); 
	assert(! (dd->flags & F_RESULT_NOTIFY)); 
	dd->check(); 

	/* Add to own */
	result.push_back(dd); 

	/* Notify parents */
	for (auto &i:  parents) {
		Flags flags= i.second->flags & (F_RESULT_NOTIFY | F_RESULT_PUT); 
		if (flags) {
			i.first->notify_result(dd, this, flags); 
		}
	}
}

Target Execution::get_target_for_cache(Target target)
{
	if (target.is_file()) {
		/* For file targets, we don't use flags for hashing. 
		 * Zero is the byte for file targets.  */
		target.get_front_byte_nondynamic()= 0; 
	}

	return target; 
}

File_Execution::~File_Execution()
/* Objects of this type are never deleted */ 
{
	assert(false);
}

void File_Execution::wait() 
{
	Debug::print(nullptr, "wait...");

	assert(File_Execution::executions_by_pid.size() != 0); 

	int status;
	pid_t pid= Job::wait(&status); 

	Debug::print(nullptr, frmt("pid = %ld", (long) pid)); 

	timestamp_last= Timestamp::now(); 

	if (executions_by_pid.count(pid) == 0) {
		/* No File_Execution is registered for the PID that
		 * just finished.  Should not happen, but since the PID
		 * value came from outside this process, we better
		 * handle this case gracefully, i.e., do nothing.  */
		print_warning(Place(), frmt("The function waitpid(2) returned the invalid process ID %jd", (intmax_t)pid)); 
		return; 
	}

	File_Execution *const execution= executions_by_pid.at(pid); 
	execution->waited(pid, status); 
	++jobs; 
}

void File_Execution::waited(pid_t pid, int status) 
{
	assert(job.started()); 
	assert(job.get_pid() == pid); 

	Execution::check_waited(); 

	flags_finished= ~0; 

	{
		Job::Signal_Blocker sb;
		executions_by_pid.erase(pid); 
	}

	/* The file(s) may have been built, so forget that it was known
	 * to not exist */
	bits &= ~B_MISSING; 

	if (job.waited(status, pid)) {
		/* Command was successful */ 

		bits |=  B_EXISTING; 
		bits &= ~B_MISSING;
		/* Subsequently set to B_MISSING if at least one target file is missing */

		/* For file targets, check that the file was built */ 
		for (size_t i= 0;  i < targets.size();  ++i) {
			const Target target= targets[i]; 

			if (! target.is_file()) {
				continue;
			}

			const char *const filename= target.get_name_c_str_nondynamic();
			struct stat buf;

			if (0 == stat(filename, &buf)) {

				/* The file exists */ 

				warn_future_file(&buf, 
						 filename,
						 rule->place_param_targets[i]->place,
						 "after execution of command"); 

				/* Check that file is not older that Stu
				 * startup */ 
				Timestamp timestamp_file(&buf);
				if (! timestamp.defined() ||
				    timestamp < timestamp_file)
					timestamp= timestamp_file; 
				if (timestamp_file < Timestamp::startup) {
					/* The target is older than Stu startup */ 

					/* Check whether the file is actually a symlink, in
					 * which case we ignore that error */ 
					if (0 > lstat(filename, &buf)) {
						rule->place_param_targets[i]->place <<
							system_format(target.format_word()); 
						raise(ERROR_BUILD);
					}
					if (S_ISLNK(buf.st_mode)) 
						continue;
					rule->place_param_targets[i]->place
						<< fmt("timestamp of file %s after execution of its command is older than %s startup", 
						       target.format_word(), 
						       dollar_zero)
						<< fmt("timestamp of %s is %s",
						       target.format_word(), timestamp_file.format())
						<< fmt("startup timestamp is %s", 
						       Timestamp::startup.format()); 
					print_traces();
					explain_startup_time();
					raise(ERROR_BUILD);
				}
			} else {
				bits |= B_MISSING; 
				bits &= ~B_EXISTING;
				rule->place_param_targets[i]->place <<
					fmt("file %s was not built by command", 
					    target.format_word()); 
				print_traces(); 

				raise(ERROR_BUILD);
			}
		}
		/* In parallel mode, print "done" message */
		if (option_parallel) {
			string text= targets[0].format_src();
			printf("Successfully built %s\n", text.c_str()); 
		}

	} else {
		/* Command failed */ 
		
		string reason;
		if (WIFEXITED(status)) {
			reason= frmt("failed with exit status %s%d%s", 
				     Color::word,
				     WEXITSTATUS(status),
				     Color::end);
		} else if (WIFSIGNALED(status)) {
			int sig= WTERMSIG(status);
			reason= frmt("received signal %d (%s)", 
				     sig,
				     strsignal(sig));
		} else {
			/* This should not happen but the standard does not exclude
			 * it  */ 
			reason= frmt("failed with status %s%d%s",
				     Color::word,
				     status,
				     Color::end); 
		}

		if (! param_rule->is_copy) {
			Target target= parents.begin()->second
				->get_target(); 
			param_rule->command->place <<
				fmt("command for %s %s", 
				    target.format_word(), 
				    reason); 
		} else {
			/* Copy rule */
			param_rule->place <<
				fmt("cp to %s %s", targets.front().format_word(), reason); 
		}

		print_traces(); 
		remove_if_existing(true); 
		raise(ERROR_BUILD);
	}
}

File_Execution::File_Execution(shared_ptr <const Dependency> dependency,
			       Execution *parent, 
			       shared_ptr <Rule> rule_,
			       shared_ptr <Rule> param_rule_,
			       map <string, string> &mapping_parameter_,
			       int &error_additional)
	:  Execution(param_rule_),
	   rule(rule_),
	   flags_finished(0)
{
	assert((param_rule_ == nullptr) == (rule_ == nullptr)); 

	swap(mapping_parameter, mapping_parameter_); 
	Target target_= dependency->get_target(); 

	/* Later replaced with all targets from the rule, when a rule exists */ 
	Target target_no_flags= target_;
	target_no_flags.get_front_byte_nondynamic() &= F_TARGET_TRANSIENT; 
	targets.push_back(target_no_flags); 
	executions_by_target[target_no_flags]= this; 

	parents[parent]= dependency; 
	if (error_additional) {
		print_traces(); 
		flags_finished= ~0; 
		parents.erase(parent); 
		raise(error_additional); 
		return;
	}

	if (rule == nullptr) {
		/* TARGETS contains only DEPENDENCY->TARGET */
	} else {
		targets.clear(); 
		for (auto &place_param_target:  rule->place_param_targets) {
			targets.push_back(place_param_target->unparametrized()); 
		}
		assert(targets.size()); 
	}

	/* Fill EXECUTIONS_BY_TARGET with all targets from the rule, not
	 * just the one given in the dependency.  */
	for (const Target &target:  targets) {
		executions_by_target[target]= this; 
	}

	if (rule != nullptr) {
		/* There is a rule for this execution */ 
		for (auto &d:  rule->dependencies) {
			push(d); 
		}
	} else {
		/* There is no rule for this execution */ 

		bool rule_not_found= false;
		/* Whether to produce the "no rule to build target" error */ 

		if (target_.is_file()) {
			if (! (dependency->flags & F_OPTIONAL)) {
				/* Check that the file is present,
				 * or make it an error */ 
				struct stat buf;
				int ret_stat= stat(target_.get_name_c_str_nondynamic(), &buf);
				if (0 > ret_stat) {
					if (errno != ENOENT) {
						string text= target_.format_word();
						perror(text.c_str()); 
						raise(ERROR_BUILD);
					}
					/* File does not exist and there is no rule for it */ 
					rule_not_found= true;
				} else {
					/* File exists:  Do nothing, and there are no
					 * dependencies to build */  
					if (dynamic_cast <Root_Execution *> (parent)) {
						/* Output this only for top-level targets, and
						 * therefore we don't need traces */ 
						print_out(fmt("No rule for building %s, but the file exists", 
							      target_.format_out_print_word())); 
						hide_out_message= true; 
					} 
				}
			}
		} else if (target_.is_transient()) {
			rule_not_found= true;
		} else {
			assert(false); 
		}
		
		if (rule_not_found) {
			assert(rule == nullptr); 
			print_traces(fmt("no rule to build %s", target_.format_word()));
			error_additional |= error |= ERROR_BUILD;
			raise(ERROR_BUILD);
			return; 
		}
	}

	/* It is not allowed to have a dynamic of a non-transitive transient */
	if (dynamic_cast <Dynamic_Execution *> (parent) &&
	    dependency->flags & F_RESULT_NOTIFY &&
	    dependency->flags & F_TARGET_TRANSIENT) {

		Place place_target;
		for (auto &i:  rule->place_param_targets) {
			if (i->place_name.unparametrized() == target_.get_name_nondynamic()) {
				place_target= i->place;
				break;
			}
		}
		assert(! place_target.empty());
		if (rule->command) {
			place_target << fmt("rule for transient target %s must not have a command", 
					   target_.format_word());
		} else {
			place_target << fmt("rule for transient target %s must not have file targets",
					   target_.format_word());
		}
		dependency->get_place() << fmt("when used as dynamic dependency of %s",
					       parent->get_parents().begin()->second->get_target().format_word());
		parent->get_parents().begin()->first->print_traces();
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}

	/* -o and -p are not allowed on non-transitive transients */
	if (dependency->flags & F_TARGET_TRANSIENT &&
	    dependency->flags & (F_OPTIONAL | F_PERSISTENT)) {

		Place place_target;
		for (auto &i:  rule->place_param_targets) {
			if (i->place_name.unparametrized() == target_.get_name_nondynamic()) {
				place_target= i->place;
				break;
			}
		}
		assert(! place_target.empty());
		dependency->get_place() << fmt((dependency->flags & F_OPTIONAL)
					       ? "dependency %s must not be optional"
					       : "dependency %s must not be persistent",
					       dependency->format_word()); 
		if (rule->command) 
			place_target << fmt("because rule for transient target %s has a command", target_.format_word());
		else 
			place_target << fmt("because rule for transient target %s has file targets", target_.format_word()); 
		print_traces();
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}

	parents.erase(parent); 
	if (find_cycle(parent, this, dependency)) {
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL; 
		return;
	}
	parents[parent]= dependency; 
}

bool File_Execution::finished() const 
{
	return ((~ flags_finished) & ((1 << C_PLACED) - 1)) == 0; 
}

bool File_Execution::finished(Flags flags) const
{
	return ((~ flags_finished & ~ flags) & ((1 << C_PLACED) - 1)) == 0; 
}

void job_terminate_all() 
/* 
 * The declaration of this function is in job.hh 
 */ 
{
	/* Strictly speaking, there is a bug here because the C++
	 * containers are not async signal-safe.  But we block the
	 * relevant signals while we update the containers, so it
	 * *should* be OK.  */ 

	write_safe(2, "stu: Terminating all jobs\n"); 
	
	for (auto i= File_Execution::executions_by_pid.begin();
	     i != File_Execution::executions_by_pid.end();  ++i) {

		const pid_t pid= i->first;

		Job::kill(pid); 
	}

	int count_terminated= 0;

	for (auto i= File_Execution::executions_by_pid.begin();
	     i != File_Execution::executions_by_pid.end();  ++i) {

		if (i->second->remove_if_existing(false))
			++count_terminated;
	}

	if (count_terminated) {
		write_safe(2, "stu: Removing partially built files (");
		constexpr int len= sizeof(int) / 3 + 3;
		char out[len];
		out[len - 1]= '\n';
		out[len - 2]= ')';
		int i= len - 3;
		int n= count_terminated;
		do {
			assert(i >= 0); 
			out[i]= '0' + n % 10;
			n /= 10;
		} while (n > 0 && --i >= 0);
		int r= write(2, out + i, len - i);
		(void) r;
	}

	/* Check that all children are terminated */ 
	while (true) {
		int status;
		int ret= wait(&status); 

		if (ret < 0) {
			/* wait() sets errno to ECHILD when there was no
			 * child to wait for */ 
			if (errno != ECHILD) {
				write_safe(2, "*** Error: wait\n"); 
			}

			return; 
		}
		assert(ret > 0); 
	}
}

void job_print_jobs()
/* The definition of this function is in job.hh */ 
{
	for (auto &i:  File_Execution::executions_by_pid) {
		i.second->print_as_job(); 
	}
}

bool File_Execution::remove_if_existing(bool output) 
{
	if (option_no_delete)
		return false;

	/* Whether anything was removed */ 
	bool removed= false;

	for (size_t i= 0;  i < targets.size();  ++i) {
		const Target &target= targets[i]; 

		if (! target.is_file())  
			continue;

		const char *filename= target.get_name_c_str_nondynamic(); 

		/* Remove the file if it exists.  If it is a symlink, only the
		 * symlink itself is removed, not the file it links to.  */ 

		struct stat buf;
		if (0 > stat(filename, &buf))
			continue;

		/* If the file existed before building, remove it only if it now
		 * has a newer timestamp.  */

		if (! (! timestamps_old[i].defined() ||
		       timestamps_old[i] < Timestamp(&buf)))
			continue;

		string text_filename= name_format_src(filename); 
		Debug::print(this, fmt("remove %s", text_filename)); 
		
		if (output) {
			print_error_reminder(fmt("Removing file %s because command failed",
						 name_format_word(filename))); 
		}
			
		removed= true;

		if (0 > unlink(filename)) {
			if (output) {
				rule->place
					<< system_format(target.format_word()); 
			} else {
				write_safe(2, "*** Error: unlink\n");
			}
		}
	}

	return removed; 
}

void File_Execution::warn_future_file(struct stat *buf, 
					const char *filename,
					const Place &place,
					const char *message_extra)
{
  	if (timestamp_last < Timestamp(buf)) {
		string suffix=
			message_extra == nullptr 
			? ""
			: string(" ") + message_extra;
		print_warning(place,
			      fmt("File %s has modification time in the future%s",
				  name_format_word(filename),
				  suffix)); 
	}
}

void File_Execution::print_command() const
{
	static const int SIZE_MAX_PRINT_CONTENT= 20;
	
	if (option_silent)
		return; 

	if (rule->is_hardcode) {
		assert(targets.size() == 1); 
		string content= rule->command->command;
		bool is_printable= false;
		if (content.size() < SIZE_MAX_PRINT_CONTENT) {
			is_printable= true;
			for (const char c:  content) {
				int cc= c;
				if (! (cc >= ' ' && c <= '~'))
					is_printable= false;
			}
		}
		string text= targets.front().format_src(); 
		if (is_printable) {
			string content_src= name_format_src(content); 
			printf("Creating %s: %s\n", text.c_str(), content_src.c_str());
		} else {
			printf("Creating %s\n", text.c_str());
		}
		return;
	} 

	if (rule->is_copy) {
		assert(rule->place_param_targets.size() == 1); 
		string cp_target= rule->place_param_targets[0]->place_name.format_src();
		string cp_source= rule->filename.format_src();
		printf("cp %s %s\n", cp_source.c_str(), cp_target.c_str()); 
		return; 
	}

	/* We are printing a regular command */

	bool single_line= rule->command->get_lines().size() == 1;

	if (! single_line || option_parallel) {
		string text= targets.front().format_src();
		printf("Building %s\n", text.c_str());
		return; 
	}

	if (option_individual)
		return; 

	bool begin= true; 
	/* For single-line commands, show the variables on the same line.
	 * For multi-line commands, show them on a separate line. */ 

	string filename_output= rule->redirect_index < 0 ? "" :
		rule->place_param_targets[rule->redirect_index]
		->place_name.unparametrized();
	string filename_input= rule->filename.unparametrized(); 

	/* Redirections */
	if (filename_output != "") {
		if (! begin)
			putchar(' '); 
		begin= false;
		printf(">%s", filename_output.c_str()); 
	}
	if (filename_input != "") {
		if (! begin)
			putchar(' '); 
		begin= false;
		printf("<%s", filename_input.c_str()); 
	}

	/* Print the parameter values (variable assignments are not printed) */ 
	for (auto i= mapping_parameter.begin(); i != mapping_parameter.end();  ++i) {
		string name= i->first;
		string value= i->second;
		if (! begin)
			putchar(' '); 
		begin= false;
		printf("%s=%s", name.c_str(), value.c_str());
	}

	/* Colon */ 
	if (! begin) {
		if (! single_line) 
			puts(":"); 
		else
			fputs(": ", stdout);
	}

	/* The command itself */ 
	for (auto &i:  rule->command->get_lines()) {
		puts(i.c_str()); 
	}
}

Proceed File_Execution::execute(shared_ptr <const Dependency> dependency_this)
{
	assert(! job.started() || children.empty()); 

	Proceed proceed= execute_base_A(dependency_this); 
	assert(proceed); 
	if (proceed & P_ABORT) {
		assert(proceed & P_FINISHED); 
		flags_finished |= ~dependency_this->flags;
		return proceed; 
	}
	if (proceed & (P_WAIT | P_PENDING)) {
		return proceed; 
	}

	assert(proceed & P_FINISHED);
	proceed &= ~P_FINISHED; 
	
	Debug debug(this);

	if (finished(dependency_this->flags)) {
		assert(! (proceed & P_WAIT)); 
		return proceed |= P_FINISHED; 
	}

	/* Job has already been started */ 
	if (job.started_or_waited()) {
		return proceed |= P_WAIT;
	}

	/* The file must now be built */ 

	assert(! targets.empty());
	assert(! targets.front().is_dynamic()); 
	assert(! targets.back().is_dynamic()); 
	assert(get_buffer_A().empty()); 
	assert(children.empty()); 
	assert(error == 0);

	/*
	 * Check whether execution has to be built
	 */

	/* Check existence of file */
	timestamps_old.assign(targets.size(), Timestamp::UNDEFINED); 

	/* A target for which no execution has to be done */ 
	const bool no_execution= 
		rule != nullptr && rule->command == nullptr && ! rule->is_copy;

	if (! (bits & B_CHECKED)) {
		bits |= B_CHECKED; 

		bits |= B_EXISTING;
		bits &= ~B_MISSING;
		/* Now, set to B_MISSING when a file is found not to exist */ 

		for (size_t i= 0;  i < targets.size();  ++i) {
			const Target &target= targets[i]; 

			if (! target.is_file()) 
				continue;

			/* We save the return value of stat() and handle errors later */ 
			struct stat buf;
			int ret_stat= stat(target.get_name_c_str_nondynamic(), &buf);

			/* Warn when file has timestamp in the future */ 
			if (ret_stat == 0) { 
				/* File exists */ 
				Timestamp timestamp_file= Timestamp(&buf); 
				timestamps_old[i]= timestamp_file;
 				if (! (dependency_this->flags & F_PERSISTENT)) 
					warn_future_file(&buf, 
							 target.get_name_c_str_nondynamic(), 
							 rule == nullptr 
							 ? parents.begin()->second->get_place()
							 : rule->place_param_targets[i]->place); 
				/* EXISTS is not changed */ 
			} else {
				bits |= B_MISSING;
				bits &= ~B_EXISTING; 
			}

			if (! (bits & B_NEED_BUILD)
			    && ret_stat == 0 
			    && timestamp.defined() 
			    && timestamps_old[i] < timestamp 
			    && ! no_execution) {
				bits |= B_NEED_BUILD;
			}

			if (ret_stat == 0) {

				assert(timestamps_old[i].defined()); 
				if (timestamp.defined() && 
				    timestamps_old[i] < timestamp &&
				    no_execution) {
					print_warning
						(rule->place_param_targets[i]->place,
						 fmt("File target %s which has no command is older than its dependency",
						     target.format_word())); 
				} 
			}
			
			if (! (bits & B_NEED_BUILD)
			    && ret_stat != 0 && errno == ENOENT) {
				/* File does not exist */

				if (! (dependency_this->flags & F_OPTIONAL)) {
					/* Non-optional dependency */  
					bits |= B_NEED_BUILD;
				} else {
					/* Optional dependency:  don't create the file;
					 * it will then not exist when the parent is
					 * called. */ 
					flags_finished |= ~F_OPTIONAL; 
					return proceed |= P_FINISHED; 
				}
			}

			if (ret_stat != 0 && errno != ENOENT) {
				/* stat() returned an actual error,
				 * e.g. permission denied.  This is a
				 * build error.  */
				rule->place_param_targets[i]->place
					<< system_format(target.format_word()); 
				raise(ERROR_BUILD);
				flags_finished |= ~ dependency_this->flags; 
				return proceed |= P_ABORT | P_FINISHED; 
			}

			/* File does not exist, all its dependencies are up to
			 * date, and the file has no commands: that's an error */  
			if (ret_stat != 0 && no_execution) { 

				assert(errno == ENOENT); 

				if (rule->dependencies.size()) {
					print_traces
						(fmt("expected the file without command %s to exist because all its dependencies are up to date, but it does not", 
						     target.format_word())); 
					explain_file_without_command_with_dependencies(); 
				} else {
					rule->place_param_targets[i]->place
						<< fmt("expected the file without command and without dependencies %s to exist, but it does not",
						       target.format_word()); 
					print_traces();
					explain_file_without_command_without_dependencies(); 
				}
				flags_finished |= ~ dependency_this->flags; 
				raise(ERROR_BUILD);
				return proceed |= P_ABORT | P_FINISHED; 
			}		
		}
		
		/* We cannot update TIMESTAMP within the loop above
		 * because we need to compare each TIMESTAMP_OLD with
		 * the previous value of TIMESTAMP. */
		for (Timestamp timestamp_old_i:  timestamps_old) {
			if (timestamp_old_i.defined() &&
			    (! timestamp.defined() || timestamp < timestamp_old_i)) {
				timestamp= timestamp_old_i; 
			}
		}
	}

	if (! (bits & B_NEED_BUILD)) {
		bool has_file= false; /* One of the targets is a file */
		for (const Target &target:  targets) {
			if (target.is_file()) {
				has_file= true; 
				break; 
			}
		}
		for (const Target &target:  targets) {
			if (! target.is_transient()) 
				continue; 
			if (transients.count(target.get_name_nondynamic()) == 0) {
				/* Transient was not yet executed */ 
				if (! no_execution && ! has_file) {
					bits |= B_NEED_BUILD; 
				}
				break;
			}
		}
	}

	if (! (bits & B_NEED_BUILD)) {
		/* The file does not have to be built */ 
		flags_finished |= ~ dependency_this->flags; 
		return proceed |= P_FINISHED; 
	}

	/* We now know that the command must be run, or that there is no
	 * command.  */

	/* Re-deploy all dependencies (second pass to execute also all
	 * transient targets) */
	Proceed proceed_2= Execution::execute_base_B(dependency_this); 
	if (proceed_2 & P_WAIT) {
		return proceed_2; 
	}
	assert(children.empty()); 

	if (no_execution) {
		/* A target without a command:  Nothing to do anymore */ 
		flags_finished |= ~ dependency_this->flags; 
		return proceed |= P_FINISHED; 
	}

	/* The command must be run (or the file created, etc.) now */

	if (option_question) {
		print_error_silenceable("Targets are not up to date");
		exit(ERROR_BUILD);
	}

	out_message_done= true;

	assert(jobs >= 0); 

	/* For hardcoded rules (i.e., static content), we don't need to
	 * start a job, and therefore this is executed even if JOBS is
	 * zero.  */
	if (rule->is_hardcode) {
		assert(targets.size() == 1);
		assert(targets.front().is_file()); 
		
		Debug::print(this, "create_content"); 

		print_command();
		write_content(targets.front().get_name_c_str_nondynamic(), *(rule->command)); 
		flags_finished= ~0;
		assert(proceed == 0); 
		return proceed |= P_FINISHED; 
	}

	/* We know that a job has to be started now */

	if (jobs == 0) {
		return proceed |= P_WAIT;
	}
       
	/* We have to start a job now */ 

	print_command();

	for (const Target &target:  targets) {
		if (! target.is_transient())  
			continue; 
		Timestamp timestamp_now= Timestamp::now(); 
		assert(timestamp_now.defined()); 
		assert(transients.count(target.get_name_nondynamic()) == 0); 
		transients[target.get_name_nondynamic()]= timestamp_now; 
	}

	if (rule->redirect_index >= 0)
		assert(! (rule->place_param_targets[rule->redirect_index]->flags & F_TARGET_TRANSIENT)); 

	assert(jobs >= 1); 

	map <string, string> mapping;
	mapping.insert(mapping_parameter.begin(), mapping_parameter.end());
	mapping.insert(mapping_variable.begin(), mapping_variable.end());
	mapping_parameter.clear();
	mapping_variable.clear(); 

	pid_t pid; 
	{
		/* Block signals from the time the process is started,
		 * to after we have entered it in the map */
		Job::Signal_Blocker sb;

		if (rule->is_copy) {

			assert(rule->place_param_targets.size() == 1); 
			assert(! (rule->place_param_targets.front()->flags & F_TARGET_TRANSIENT)); 

			string source= rule->filename.unparametrized();
			
			/* If optional copy, don't just call 'cp' and
			 * let it fail:  look up whether the source
			 * exists in the cache */
			if (rule->dependencies.at(0)->flags & F_OPTIONAL) {
				Execution *execution_source_base=
					executions_by_target.at(Target(0, source));
				assert(execution_source_base); 
				File_Execution *execution_source
					= dynamic_cast <File_Execution *> (execution_source_base); 
				assert(execution_source); 
				if (execution_source->bits & B_MISSING) {
					/* Neither the source file nor
					 * the target file exist:  an
					 * error  */
					rule->dependencies.at(0)->get_place()
						<< fmt("source file %s in optional copy rule "
						       "must exist",
						       name_format_word(source));
					print_traces(fmt("when target file %s does not exist",
							 targets.at(0).format_word())); 
					raise(ERROR_BUILD);
					flags_finished |= ~ dependency_this->flags; 
					assert(proceed == 0); 
					return proceed |= P_ABORT | P_FINISHED; 
				}
			}
			
			pid= job.start_copy
				(rule->place_param_targets[0]->place_name.unparametrized(),
				 source);
		} else {
			pid= job.start
				(rule->command->command, 
				 mapping,
				 rule->redirect_index < 0 ? "" :
				 rule->place_param_targets[rule->redirect_index]
				 ->place_name.unparametrized(),
				 rule->filename.unparametrized(),
				 rule->command->place); 
		}

		assert(pid != 0 && pid != 1); 

		Debug::print(this, frmt("execute: pid = %ld", (long) pid)); 

		if (pid < 0) {
			/* Starting the job failed */ 
			print_traces(fmt("error executing command for %s", 
					 targets.front().format_word())); 
			raise(ERROR_BUILD);
			flags_finished |= ~ dependency_this->flags; 
			assert(proceed == 0); 
			proceed |= P_ABORT | P_FINISHED; 
			return proceed;
		}

		executions_by_pid[pid]= this;
	}

	assert(executions_by_pid.at(pid)->job.started()); 
	assert(pid == executions_by_pid.at(pid)->job.get_pid()); 
	--jobs;
	assert(jobs >= 0);

	proceed |= P_WAIT; 
	if (order == Order::RANDOM && jobs > 0)
		proceed |= P_PENDING; 
	return proceed;
}

void File_Execution::print_as_job() const
{
	pid_t pid= job.get_pid();

	string text_target= targets.front().format_src(); 

	printf("%7ld %s\n", (long) pid, text_target.c_str());
}

void File_Execution::write_content(const char *filename, 
				     const Command &command)
{
	FILE *file= fopen(filename, "w"); 

	if (file == nullptr) {
		rule->place << system_format(name_format_word(filename)); 
		raise(ERROR_BUILD); 
		return;
	}

	for (const string &line:  command.get_lines()) {
		if (fwrite(line.c_str(), 1, line.size(), file) != line.size()) {
			assert(ferror(file));
			fclose(file); 
			rule->place <<
				system_format(name_format_word(filename)); 
			raise(ERROR_BUILD); 
		}
		if (EOF == putc('\n', file)) {
			fclose(file); 
			rule->place <<
				system_format(name_format_word(filename)); 
			raise(ERROR_BUILD); 
		}
	}

	if (0 != fclose(file)) {
		rule->place <<
			system_format(name_format_word(filename)); 
		command.get_place() << 
			fmt("error creating %s", 
			    name_format_word(filename)); 
		raise(ERROR_BUILD); 
	}

	bits |= B_EXISTING;
	bits &= ~B_MISSING; 
}

void File_Execution::read_variable(shared_ptr <const Dependency> dependency)
{
	Debug::print(this, fmt("read_variable %s", dependency->format_src())); 
	
	assert(dynamic_pointer_cast <const Plain_Dependency> (dependency)); 

	if (! result_variable.empty()) {
		/* It was already read */
		return; 
	}
	
	/* It could be that the file exists but the bit is not set --
	 * this would happen if the file was not there before and we had
	 * no reason to check.  In such cases, we don't need the
	 * variable.  */
	if (!(bits & B_EXISTING)) {
		assert(dependency->flags & F_TRIVIAL); 
		return;
	}

	if (error) {
		return; 
	}

	Target target= dependency->get_target(); 
	assert(! target.is_dynamic()); 

	size_t filesize;
	struct stat buf;
	string dependency_variable_name;
	string content; 
	
	int fd= open(target.get_name_c_str_nondynamic(), O_RDONLY);
	if (fd < 0) {
		if (errno != ENOENT) {
			dependency->get_place() << target.format_word();
		}
		goto error;
	}
	if (0 > fstat(fd, &buf)) {
		dependency->get_place() << target.format_word(); 
		goto error_fd;
	}

	filesize= buf.st_size;
	content.resize(filesize);
	if ((ssize_t) filesize != read(fd, (void *) content.c_str(), filesize)) {
		dependency->get_place() << target.format_word(); 
		goto error_fd;
	}

	if (0 > close(fd)) { 
		dependency->get_place() << target.format_word(); 
		goto error;
	}

	/* Remove space at beginning and end of the content.
	 * The characters are exactly those used by isspace() in
	 * the C locale.  */ 
	content.erase(0, content.find_first_not_of(" \n\t\f\r\v")); 
	content.erase(content.find_last_not_of(" \n\t\f\r\v") + 1);  

	/* The variable name */ 
	dependency_variable_name=
		dynamic_pointer_cast <const Plain_Dependency> (dependency)->variable_name; 

	{
		string variable_name= 
			dependency_variable_name == "" ?
			target.get_name_nondynamic() : dependency_variable_name;

		result_variable[variable_name]= content; 
	}

	return;

 error_fd:
	close(fd); 
 error:
	Target target_variable= 
		dynamic_pointer_cast <const Plain_Dependency> (dependency)->place_param_target
		.unparametrized(); 

	if (rule == nullptr) {
		dependency->get_place() <<
			fmt("file %s was up to date but cannot be found now", 
			    target_variable.format_word());
	} else {
		for (auto const &place_param_target: rule->place_param_targets) {
			if (place_param_target->unparametrized() == target_variable) {
				place_param_target->place <<
					fmt("generated file %s was built but cannot be found now", 
					    place_param_target->format_word());
				break;
			}
		}
	}
	print_traces();

	raise(ERROR_BUILD); 
}

bool File_Execution::optional_finished(shared_ptr <const Dependency> dependency_link)
{
	if ((dependency_link->flags & F_OPTIONAL) 
	    && dynamic_pointer_cast <const Plain_Dependency> (dependency_link)
	    && ! (dynamic_pointer_cast <const Plain_Dependency> (dependency_link)
		  ->place_param_target.flags & F_TARGET_TRANSIENT)) {

		/* We already know a file to be missing */ 
		if (bits & B_MISSING) {
			flags_finished |= ~dependency_link->flags; 
			return true; 
		}
		
		const char *name= dynamic_pointer_cast <const Plain_Dependency> (dependency_link)
			->place_param_target.place_name.unparametrized().c_str();

		struct stat buf;
		int ret_stat= stat(name, &buf);
		if (ret_stat < 0) {
			bits |= B_MISSING;
			bits &= ~B_EXISTING; 
			if (errno != ENOENT) {
				dynamic_pointer_cast <const Plain_Dependency> (dependency_link)
					->place_param_target.place <<
					system_format(name_format_word(name)); 
				raise(ERROR_BUILD);
				flags_finished |= ~dependency_link->flags; 
				return true;
			}
			flags_finished |= ~dependency_link->flags; 
			return true;
		} else {
			assert(ret_stat == 0);
			bits |= B_EXISTING;
			bits &= ~B_MISSING;
		}
	}

	return false; 
}

bool Root_Execution::finished() const
{
	return is_finished; 
}

bool Root_Execution::finished(Flags flags) const
{
	(void) flags; 

	return is_finished; 
}

Root_Execution::Root_Execution(const vector <shared_ptr <const Dependency> > &dependencies)
	:  is_finished(false)
{
	for (auto &d:  dependencies) {
		push(d); 
	}
}

Proceed Root_Execution::execute(shared_ptr <const Dependency> dependency_this)
{
	/* This is an example of a "plain" execute() function,
	 * containing the minimal wrapper around execute_base_?()  */ 
	
	Proceed proceed= execute_base_A(dependency_this); 
	assert(proceed); 
	if (proceed & (P_WAIT | P_PENDING)) {
		assert((proceed & P_FINISHED) == 0); 
		return proceed;
	}
	if (proceed & P_FINISHED) {
		is_finished= true; 
		return proceed; 
	}

	proceed |= execute_base_B(dependency_this);
	if (proceed & (P_WAIT | P_PENDING)) {
		assert((proceed & P_FINISHED) == 0); 
		return proceed;
	}
	if (proceed & P_FINISHED) {
		is_finished= true; 
	}

	return proceed; 
}

Concatenated_Execution::Concatenated_Execution(shared_ptr <const Concatenated_Dependency> dependency_,
					       shared_ptr <const Dependency> dependency_link,
					       Execution *parent,
					       bool &found_cycle)
	:  Execution(),
	   dependency(dependency_),
	   stage(0)
{
	assert(dependency_->is_normalized()); 

	/* Check the structure of the dependency */
	shared_ptr <const Dependency> dep= dependency;
	dep= Dependency::strip_dynamic(dep); 
	assert(dynamic_pointer_cast <const Concatenated_Dependency> (dep));
	shared_ptr <Concatenated_Dependency> concatenated_dependency= 
		dynamic_pointer_cast <Concatenated_Dependency> (Dependency::clone(dep));

	for (size_t i= 0;  i < concatenated_dependency->get_dependencies().size();  ++i) {

		shared_ptr <const Dependency> d= concatenated_dependency->get_dependencies()[i]; 

		shared_ptr <const Dependency> dependency_normalized= 
			Dependency::make_normalized_compound(d); 

		concatenated_dependency->get_dependencies()[i]= dependency_normalized; 
	}

	dependency= concatenated_dependency; 

	if (find_cycle(parent, this, dependency_link)) {
		parent->raise(ERROR_LOGICAL);
		found_cycle= true; 
		return;
	}
	parents[parent]= dependency_link; 

}

Proceed Concatenated_Execution::execute(shared_ptr <const Dependency> dependency_this)
{
	assert(stage >= 0 && stage <= 3); 

	if (stage == 0) {
		/* Construct all initial dependencies */ 
		/* Not all parts need to have something constructed.
		 * Only those that are dynamic do:
		 *
		 *    list.(X Y Z)     # Nothing to build in stage 0
		 *    list.[X Y Z]     # Build X, Y, Z in stage 0
		 *
		 * Add, as extra dependencies, all sub-dependencies of
		 * the concatenated dependency, minus one dynamic level,
		 * or not at all if they are not dynamic.  */

		shared_ptr <const Dependency> dep= Dependency::strip_dynamic(dependency); 
		shared_ptr <const Concatenated_Dependency> concatenated_dependency= 
			dynamic_pointer_cast <const Concatenated_Dependency> (dep);
		assert(concatenated_dependency != nullptr); 

		const size_t n= concatenated_dependency->get_dependencies().size(); 

		for (size_t i= 0;  i < n;  ++i) {
			shared_ptr <const Dependency> d= concatenated_dependency->get_dependencies()[i]; 
			if (dynamic_pointer_cast <const Compound_Dependency> (d)) {
				for (shared_ptr <const Dependency> dd:  
					     dynamic_pointer_cast <const Compound_Dependency> (d)->get_dependencies()) {
					add_stage0_dependency(dd, i); 
				}
			} else {
				add_stage0_dependency(d, i); 
			}
		}

		/* Initialize parts */
		parts.resize(n); 

		stage= 1; 
		/* Fall through to stage 1 */ 
	} 

	if (stage == 1) {
		/* First phase:  we build all individual targets, if there are some */ 
		Proceed proceed= execute_base_A(dependency_this); 
		assert(proceed); 
		if (proceed & P_WAIT) {
			return proceed;
		}

		proceed |= execute_base_B(dependency_this); 
		assert(proceed); 
		if (proceed & P_WAIT) {
			return proceed;
		}

		/* The parts are filled incrementally when the children
		 * are unlinked  */

		/* Put all the parts together */
		assemble_parts(); 

		stage= 2; 

		/* Fall through to stage 2 */
		
	} 

	if (stage == 2) {
		/* Second phase:  normal child executions */
		assert(! dependency); 
		Proceed proceed= execute_base_A(dependency_this); 
		assert(proceed); 
		if (proceed & P_WAIT) {
			return proceed;
		}

		proceed |= execute_base_B(dependency_this); 
		assert(proceed); 
		if (proceed & P_WAIT) {
			return proceed;
		}

		assert((proceed & P_WAIT) == 0); 

		stage= 3; 

		/* No need to set a DONE variable for concatenated
		 * executions -- whether we are done is indicated by
		 * STAGE == 3.  */
		
		return proceed; 

	} else if (stage == 3) {
		return 0; 
	} else {
		assert(false);  /* Invalid stage */ 
		return 0; 
	}
}

bool Concatenated_Execution::finished() const
{
	/* We ignore DONE here */
	return stage == 3;
}

bool Concatenated_Execution::finished(Flags flags) const
/* Since Concatenated_Execution objects are used just once, by a single
 * parent, this always returns the same as finished() itself.
 * Therefore, the FLAGS parameter is ignored.  */
{
	(void) flags; 
	return finished(); 
}

void Concatenated_Execution::add_stage0_dependency(shared_ptr <const Dependency> d,
						   unsigned concatenate_index)
/* 
 * The given dependency can be non-normalized. 
 */
{
	assert(false);
	(void) d;  (void) concatenate_index;  
}

shared_ptr <const Dependency> Concatenated_Execution::
concatenate_dependency_one(shared_ptr <const Plain_Dependency> dependency_1,
			   shared_ptr <const Plain_Dependency> dependency_2,
			   Flags dependency_flags)
/* 
 * Rules for concatenation:
 *   - Flags are not allowed on the second component.
 *   - The second component must not be transient. 
 */
{
	assert(dependency_2->flags == 0);
	// XXX test:  replace by a proper error 

	assert(! (dependency_2->place_param_target.flags & F_TARGET_TRANSIENT)); 
	// XXX proper test.  

	Place_Param_Target target= dependency_1->place_param_target; 
	target.place_name.append(dependency_2->place_param_target.place_name); 

	return make_shared <Plain_Dependency> 
		(dependency_flags & dependency_1->flags,
		 target,
		 dependency_1->place,
		 "");
}

Concatenated_Execution::~Concatenated_Execution()
{
	/* Nop */ 
}

void Concatenated_Execution::assemble_parts()
{
	if (parts.size() == 0) {
		/* This is theoretically and empty product and should
		 * have a single element which is the empty string, but
		 * that is not possible.  */
		assert(false);
		return; 
	}

	vector <shared_ptr <const Dependency> > dependencies_read; 

	for (size_t i= 0;  i < parts.size();  ++i) {

		vector <shared_ptr <const Dependency> > dependencies_read_new; 
		
		/* If a single index is empty, the whole result is
		 * an empty set of dependencies.  */
		if (parts[i].empty()) {
			dependencies_read= vector <shared_ptr <const Dependency> > (); 
			return; 
		}

		if (i == 0) {
			/* The leftmost components are special:  we
			 * don't perform any checks on them, as they can
			 * be transient and have flags, while subsequent
			 * parts cannot.  */
			for (size_t k= 0;  k < parts[i].size();  ++k) {
				dependencies_read_new.push_back(parts[i][k]);
			}
		} else {
			for (size_t j= 0;  j < dependencies_read.size();  ++j) {
				for (size_t k= 0;  k < parts[i].size();  ++k) {
					// XXX do something here ...
				}
			}
		}
		
		swap(dependencies_read, dependencies_read_new); 
	}

	for (auto &i:  dependencies_read) {
		push(i); 
	}
}

Dynamic_Execution::Dynamic_Execution(shared_ptr <const Dynamic_Dependency> dependency_,
				     Execution *parent,
				     int &error_additional)
	:  dependency(dependency_),
	   is_finished(false)
{
	assert(dependency_); 
	assert(parent); 
	dependency->check();
	
	/* Set the rule here, so cycles in the dependency graph can be
	 * detected.  Note however that the rule of dynamic executions
	 * is otherwise not used.  */ 

	parents[parent]= dependency;
	if (error_additional) {
		print_traces(); 
		is_finished= true; 
		parents.erase(parent); 
		raise(error_additional); 
		return;
	}

	/* Find the rule of the inner dependency */
	shared_ptr <const Dependency> inner_dependency= Dependency::strip_dynamic(dependency);
	if (dynamic_pointer_cast <const Plain_Dependency> (inner_dependency)) {
		shared_ptr <const Plain_Dependency> inner_plain_dependency
			= dynamic_pointer_cast <const Plain_Dependency> (inner_dependency); 
		Target target_base(inner_plain_dependency->place_param_target.flags,
				   inner_plain_dependency->place_param_target.place_name.unparametrized());
		Target target= dependency->get_target(); 
		try {
			map <string, string> mapping_parameter; 
			shared_ptr <Rule> rule= 
				rule_set.get(target_base, param_rule, mapping_parameter, 
					     dependency->get_place()); 
		} catch (int e) {
			assert(e); 
			print_traces(); 
			error_additional |= e;
			raise(e); 
			return; 
		}
		executions_by_target[target]= this; 
	}

	parents.erase(parent); 
	if (find_cycle(parent, this, dependency)) {
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL; 
		return;
	}
	parents[parent]= dependency; 

	/* Push single initial dependency */ 
	shared_ptr <Dependency> dependency_child= Dependency::clone(dependency->dependency);
	dependency_child->flags |= F_RESULT_NOTIFY; 
	push(dependency_child); 
}

Proceed Dynamic_Execution::execute(shared_ptr <const Dependency> dependency_this)
{
	Proceed proceed= execute_base_A(dependency_this); 
	assert(proceed); 
	if (proceed & P_FINISHED) {
		is_finished= true; 
		return proceed;
	}
	proceed |= execute_base_B(dependency_this);
	if (proceed & P_FINISHED) {
		is_finished= true; 
	}

	return proceed; 
}

bool Dynamic_Execution::finished() const 
{
	return is_finished; 
}

bool Dynamic_Execution::finished(Flags) const
{
	return is_finished; 
}

bool Dynamic_Execution::want_delete() const
{
	return dynamic_pointer_cast <const Plain_Dependency> (Dependency::strip_dynamic(dependency)) == nullptr; 
}

string Dynamic_Execution::format_src() const
{
	return dependency->format_src();
}

void Dynamic_Execution::notify_result(shared_ptr <const Dependency> d, Execution *source, Flags flags)
{
	assert(!(flags & ~(F_RESULT_NOTIFY | F_RESULT_PUT))); 
	assert((flags & ~(F_RESULT_NOTIFY | F_RESULT_PUT)) != (F_RESULT_NOTIFY | F_RESULT_PUT)); 

	if (flags & F_RESULT_NOTIFY) {
	if (dynamic_pointer_cast <const Plain_Dependency> (d)) {
		try {
			const Place_Param_Target &place_param_target= 
				dynamic_pointer_cast <const Plain_Dependency> (d)->place_param_target; 

			/* Check:  variable dependencies are not allowed in multiply
			 * dynamic dependencies.  */
			if (d->flags & F_VARIABLE) {
				bool quotes= false;
				string s= d->get_target().format(S_MARKERS | S_NOEMPTY, quotes);
				d->get_place() << fmt("variable dependency %s$[%s%s%s]%s must not appear", 
						      Color::word,
						      quotes ? "'" : "",
						      s,
						      quotes ? "'" : "",
						      Color::end); 
				source->print_traces
					(fmt("within multiply-dynamic dependency %s", 
					     dependency->get_target().format_word()));
				raise(ERROR_LOGICAL);
			} 

			if ((place_param_target.flags & F_TARGET_TRANSIENT) == 0) {
				vector <shared_ptr <const Dependency> > dependencies;
				source->read_dynamic(d->flags,
						     place_param_target,
						     dependencies); 
				for (auto &j:  dependencies) {
					shared_ptr <Dependency> j_new= Dependency::clone(j); 
					/* Add -% flag */
					j_new->flags |= F_RESULT_PUT;
					/* Add flags from self  */  
					j_new->flags |= dependency->flags & (F_TARGET_BYTE & ~F_TARGET_DYNAMIC); 
					for (int i= 0;  i < C_PLACED;  ++i) {
						if (j_new->get_place_flag(i).empty() && 
						    ! dependency->get_place_flag(i).empty())
							j_new->set_place_flag(i, dependency->get_place_flag(i)); 
					}
					j= j_new; 
					push(j); 
				}
			}
		} catch (int e) {
			/* We catch not only the errors raised in this function,
			 * but also the errors raised in read_dynamic().  */
			raise(e); 
		}
	} else {
		assert(false);
	}
	} else {
		assert(flags & F_RESULT_PUT);
		push_result(d); 
	}
}

Transient_Execution::~Transient_Execution()
/* Objects of this type are never deleted */ 
{
	assert(false);
}

Proceed Transient_Execution::execute(shared_ptr <const Dependency> dependency_this)
{
	Proceed proceed= execute_base_A(dependency_this); 
	assert(proceed); 
	if (proceed & (P_WAIT | P_PENDING)) {
		return proceed; 
	}
	if (proceed & P_FINISHED) {
		is_finished= true; 
	}

	return proceed; 
}

bool Transient_Execution::finished() const
{
	return is_finished; 
}

bool Transient_Execution::finished(Flags) const
{
	return is_finished; 
}

Transient_Execution::Transient_Execution(shared_ptr <const Dependency> dependency_link,
					 Execution *parent,
					 shared_ptr <Rule> rule_,
					 shared_ptr <Rule> param_rule_,
					 map <string, string> &mapping_parameter_,
					 int &error_additional)
	:  Execution(param_rule_),
	   rule(rule_)
{
	swap(mapping_parameter, mapping_parameter_); 

	assert(dynamic_pointer_cast <const Plain_Dependency> (dependency_link)); 
	shared_ptr <const Plain_Dependency> plain_dependency= 
		dynamic_pointer_cast <const Plain_Dependency> (dependency_link);

	Target target= plain_dependency->place_param_target.unparametrized();
	assert(target.is_transient()); 

	if (rule == nullptr) {
		targets.push_back(dependency_link->get_target());
	}

	parents[parent]= dependency_link; 
	if (error_additional) {
		print_traces();
		is_finished= true;
		parents.erase(parent); 
		raise(error_additional); 
		return; 
	}

	if (rule == nullptr) {
		/* There must be a rule for transient targets (as
		 * opposed to file targets), so this is an error.  */
		is_finished= true; 
		print_traces(fmt("no rule to build %s", target.format_word()));
		parents.erase(parent); 
		error_additional |= ERROR_BUILD; 
		raise(ERROR_BUILD);
		return; 
	} 

	for (auto &place_param_target:  rule->place_param_targets) {
		targets.push_back(place_param_target->unparametrized()); 
	}
	assert(targets.size()); 

	assert((param_rule == nullptr) == (rule == nullptr)); 

	/* Fill EXECUTIONS_BY_TARGET with all targets from the rule, not
	 * just the one given in the dependency.  Also, add the flags.  */
	for (Target t:  targets) {
		t.get_front_byte_nondynamic() |= (char)(unsigned char)
			(dependency_link->flags & (F_TARGET_BYTE & ~F_TARGET_DYNAMIC)); 
		executions_by_target[t]= this; 
	}

	for (auto &dependency:  rule->dependencies) {
		shared_ptr <const Dependency> dep= dependency;
		if (dependency_link->flags) {
			shared_ptr <Dependency> dep_new= Dependency::clone(dep); 
			dep_new->flags |= dependency_link->flags & (F_PLACED | F_ATTRIBUTE);
			dep_new->flags |= F_RESULT_PUT; 
			for (int i= 0;  i < C_PLACED;  ++i) {
				assert(!(dependency_link->flags & (1 << i)) ==
				       dependency_link->get_place_flag(i).empty());
				if (dep_new->get_place_flag(i).empty() && ! dependency_link->get_place_flag(i).empty())
					dep_new->set_place_flag(i, dependency_link->get_place_flag(i)); 
			}
			dep= dep_new;
		}
		push(dep); 
	}

	parents.erase(parent); 
	if (find_cycle(parent, this, dependency_link)) {
		raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL; 
		return; 
	}
	parents[parent]= dependency_link; 
}
	
string Transient_Execution::format_src() const {
	assert(targets.size()); 
	return targets.front().format_src(); 
}

void Transient_Execution::notify_result(shared_ptr <const Dependency> dependency,
					Execution *,
					Flags flags)
{
	assert(flags == F_RESULT_PUT); 
	push_result(dependency); 
}

void Debug::print(Execution *e, string text) 
{
	if (e == nullptr) {
		print("", text);
	} else {
		if (executions.size() > 0 &&
		    executions[executions.size() - 1] == e) {
			print(e->format_src(), text); 
		} else {
			Debug debug(e);
			print(e->format_src(), text); 
		}
	}
}

void Debug::print(string text_target,
		  string text)
{
	assert(text != "");
	assert(text[0] >= 'a' && text[0] <= 'z'); 
	assert(text[text.size() - 1] != '\n');

	if (! option_debug) 
		return;

	if (text_target != "")
		text_target += ' ';

	fprintf(stderr, "DEBUG  %s%s%s\n",
		padding(),
		text_target.c_str(),
		text.c_str()); 
}

#endif /* ! EXECUTION_HH */
