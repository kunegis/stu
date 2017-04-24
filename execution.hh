#ifndef EXECUTION_HH
#define EXECUTION_HH

/* 
 * Code for executing the building process itself.  
 *
 * If there is ever a "libstu", this will be its main entry point. 
 */

#include <sys/stat.h>

#include "buffer.hh"
#include "parser.hh"
#include "job.hh"
#include "link.hh"
#include "tokenizer.hh"
#include "rule.hh"
#include "timestamp.hh"

class Execution
/*
 * Base class of all executions.
 *
 * Executions are allocated with new(), are used via ordinary pointers,
 * and deleted (if nercessary), via delete().  
 *
 * The set of active Execution objects forms a directed acyclic graph,
 * rooted at the root Execution object.  Edges in this graph are
 * represented by Link objects.  Each Execution object corresponds to
 * one or more unique Target objects.  Two Execution objects are
 * connected if there is a dependency between them.  If there is an edge
 * A ---> B, A is said to be the parent of B, and B the child of A.
 * Also, B is a dependency of A.  If A is a dynamic target, then it has
 * as an initial child only the corresponding target with one less
 * level of depth; other dependencies are added later.
 */
{
public: 

	typedef unsigned Proceed;

	enum {
		/* This is used as the return value of the functions execute()
		 * and similar.  Defined as a typedef to make arithmetic with it.  */
	
		P_BIT_WAIT =     1 << 0,
		P_BIT_LATER =    1 << 1,

		P_CONTINUE = 0, 
		/* Execution can continue in the process */
	};

	void raise(int error_);
	/* All errors by Execution call this function.  Set the error
	 * code, and throw an error except with the keep-going option.  */

	Proceed execute_base(const Link &link, Stack &done_here);
	int get_error() const {  return error;  }

	virtual Proceed execute(Execution *parent, const Link &link)= 0;
	/* Start the next job(s).  This will also terminate jobs when
	 * they don't need to be run anymore, and thus it can be called
	 * when K = 0 just to terminate jobs that need to be terminated.
	 * The passed LINK.FLAG is the ORed combination of all FLAGs up
	 * the dependency chain.
	 * Can only return LATER in random mode. 
	 * When returning LATER, not all possible child jobs where started.  
	 * Child implementations call this implementation.  
	 * Never returns P_CONTINUE:  When everything is finished, the
	 * FINISHED bit is set.  
	 * In DONE, set those bits that have been done. 
	 */

	virtual bool finished() const= 0;
	/* Whether the execution is completely finished */ 

	virtual bool finished(Stack avoid) const= 0; 
	/* Whether the execution is finished working for the given tasks */ 

	virtual string debug_done_text() const
	/* Extra string for the "done" information; may be empty.  */
	{ 
		return ""; 
	}

	/* Fill the passes vector with the "list" of dependency, i.e.,
	 * the dependencies "contained in" this dependency.  This is the
	 * list of dependencies equivalent to building the dependency of
	 * this execution as a dynamic dependency.  The passed list must
	 * be empty on call.  */
	virtual void next_list(vector <shared_ptr <Dependency> > &list) const= 0; 

	static long jobs;
	/* Number of free slots for jobs.  This is a long because
	 * strtol() gives a long.  Set before calling main() from the -j
	 * option, and then changed internally by this class.  */ 

	static Rule_Set rule_set; 
	/* Set once before calling Execution::main().  Unchanging during
	 * the whole call to Execution::main().  */ 

	static void main(const vector <shared_ptr <Dependency> > &dependencies);
	/* Main execution loop.  This throws ERROR_BUILD and
	 * ERROR_LOGICAL.  */

protected: 
	
	int error;
	/* Error value of this execution.  The value is propagated
	 * (using '|') to the parent.  Values correspond to constants
	 * defined in error.hh; zero denotes the absence of an
	 * error.  */ 

	set <Execution *> children;
	/* Currently running executions.  Allocated with operator new()
	 * and never deleted.  */ 

	map <Execution *, Link> parents; 
	/* The parent executions.  This is a map rather than an
	 * unsorted_map because typically, the
	 * number of elements is always very small, i.e., mostly one,
	 * and a map is better suited in this case.  */ 

	Timestamp timestamp; 
	/* Latest timestamp of a (direct or indirect) dependency
	 * that was not rebuilt.  Files that were rebuilt are not
	 * considered, since they make the target be rebuilt anyway.
	 * Implementations also changes this to consider the file
	 * itself, if any.  This final timestamp is then carried over to the
	 * parent executions.  */

	bool need_build;
	/* Whether this target needs to be built.  When a target is
	 * finished, this value is propagated to the parent executions
	 * (except when the F_PERSISTENT flag is set).  */ 

	vector <shared_ptr <Single_Dependency> > result; 
	/* The final list of dependencies represented by the target.
	 * This does not include any dynamic dependencies, i.e., all
	 * dependencies are Single_Dependency's.  */

	shared_ptr <Rule> param_rule;
	/* The (possibly parametrized) rule from which this execution
	 * was derived.  This is only used to detect strong cycles.  To
	 * manage the dependencies, the instantiated general rule is
	 * used.  Null by default, and set by individual implementations
	 * in their constructor if necessary.  */ 

	Execution(Link &link, Execution *parent)
		/* K is the depth of the done field, i.e. the depth of
		 * the dependency for purposes of keeping track of
		 * caching.  */
		:  error(0),
		   timestamp(Timestamp::UNDEFINED),
		   need_build(false)
	{  
		assert(parent != nullptr); 
		parents[parent]= link; 
	}

	explicit Execution(Execution *parent_null)
		/* Without a parent.  PARENT_NULL must be null. */
		:  error(0),
		   timestamp(Timestamp::UNDEFINED),
		   need_build(false)
	{  
		assert(parent_null == nullptr); 
	}

	void print_traces(string text= "") const;
	/* Print full trace for the execution.  First the message is
	 * printed, then all traces for it starting at this execution,
	 * up to the root execution. 
	 * TEXT may be "" to not print the first message.  */ 

	Proceed execute_children(const Link &link, Stack &done_here);
	/* Execute already-active children */

	Proceed execute_second_pass(const Link &link); 
	/* Second pass (trivial dependencies).  Called once we are sure
	 * that the target must be built.  */

	void check_waited() const {
		assert(buffer_default.empty()); 
		assert(buffer_trivial.empty()); 
		assert(children.empty()); 
	}

	const Buffer &get_buffer_default() const {  return buffer_default;  }
	const Buffer &get_buffer_trivial() const {  return buffer_trivial;  }
	
	void push_default(shared_ptr <Dependency> );
	/* Push a default to the default buffer, breaking down non-normalized
	 * dependencies while doing so.  */

	void read_dynamic(Stack avoid_this, 
			  shared_ptr <Dynamic_Dependency> dependency_this, 
			  vector <shared_ptr <Dependency> > &dependencies);
  	/* Read dynamic dependencies.  The only reason this is not
	 * static is that errors can be raised correctly.
	 * DEPENDENCY_THIS is normalized.  Dependencies that were read
	 * are written into DEPENDENCIES, which must be empty on calling.  */

	virtual ~Execution(); 

	virtual int get_depth() const= 0;
	/* The dynamic depth, or -1 when
	 * undefined as in concatenated executions and the root execution. */ 

	virtual const Place &get_place() const= 0;
	/* The place for the execution; e.g. the rule; empty if there is no place */

	virtual bool optional_finished(const Link &)= 0;
	/* Should children even be started?  Check whether this is an
	 * optional dependency and if it is, return TRUE when the file does not
	 * exist.  Return FALSE when children should be started.  */

#ifndef NDEBUG
	virtual void check_execution(const Link &link) const= 0; 
	/* Perform consistency assertions */ 
#endif

	virtual string debug_text() const= 0;
	/* The text shown for this execution in verbose output */ 

	virtual bool want_delete() const= 0; 

	static Timestamp timestamp_last; 
	/* The timepoint of the last time wait() returned.  No file in the
	 * file system should be newer than this.  */ 

	static bool hide_out_message;
	/* Whether to show a STDOUT message at the end */

	static bool out_message_done;
	/* Whether the STDOUT message is not "Targets are up to date" */

	static unordered_map <Target, Execution *> executions_by_target;
	/* The Execution objects by each of their target, for
	 * non-concatenated targets.  Such Execution objects
	 * are never deleted.  This serves as a caching mechanism.  
	 * Non-dynamic execution objects are shared by the multiple
	 * targets of a multi-target rule.  A dynamic multi-target rule
	 * result in multiple non-shared execution objects.  
	 * The objects are of type Single_Execution (when not dynamic)
	 * or Dynamic_Execution (when dynamic). 
	 */

	static bool find_cycle(const Execution *const parent,
			       const Execution *const child,
			       const Link &link);
	/* Find a cycle.  Assuming that the edge parent->child will be added, find a
	 * directed cycle that would be created.  Start at PARENT and
	 * perform a depth-first search upwards in the hierarchy to find
	 * CHILD.  LINK is the LINK that would be added between child
	 * and parent, and would create a cycle.  */

	static bool find_cycle(vector <const Execution *> &path,
				const Execution *const child,
				const Link &link); 
	/* Helper function */ 

	static void cycle_print(const vector <const Execution *> &path,
				const Link &link);
	/* Print the error message of a cycle on rule level.
	 * Given the path [a, b, c, d, ..., x], the found cycle is
	 * [x <- a <- b <- c <- d <- ... <- x], where A <- B denotes
	 * that A is a dependency of B.  For each edge in this cycle,
	 * output one line.  LINK is the link (x <- a), which is not yet
	 * created in the execution objects.  */ 

	static bool same_rule(const Execution *execution_a,
			      const Execution *execution_b);
	/* Whether both executions have the same parametrized rule.
	 * Only used for finding cycle.  */ 

	static void unlink(Execution *const parent, 
			   Execution *const child,
			   shared_ptr <Dependency> dependency_parent,
			   Stack avoid_parent,
			   shared_ptr <Dependency> dependency_child,
			   Stack avoid_child,
			   Flags flags_child); 
	/* Propagate information from the subexecution to the execution, and
	 * then delete the child execution if necessary.  */

private: 

	Buffer buffer_default;
	/* Dependencies that have not yet begun to be built.
	 * Initialized with all dependencies, and emptied over time when
	 * things are built, and filled over time when dynamic
	 * dependencies are worked on.  Entries are not necessarily
	 * unique.  Does not contain compound dependencies, except under
	 * concatenating ones.  */  

	Buffer buffer_trivial; 
	/* The buffer for dependencies in the second pass.  They are
	 * only started if, after (potentially) starting all non-trivial
	 * dependencies, the target must be rebuilt anyway.  Does not
	 * contain compound dependencies.  */

	Proceed execute_deploy(const Link &link,
			       shared_ptr <Dependency> dependency_child);
	/* Deploy a new child execution.  LINK is the link from the
	 * THIS's parent to THIS.  Note: the top-level flags of
	 * LINK.DEPENDENCY may be modified.   DEPENDENCY_CHILD must be
	 * normalized.  */
	
	static Execution *get_execution(const Target &target, 
					Link &link,
					Execution *parent); 
	/* Get an existing Execution or create a new one for the
	 * given TARGET.  Return null when a strong cycle was found;
	 * return the execution otherwise.  PLACE is the place of where
	 * the dependency was declared.  LINK is the link from the
	 * existing parent to the new execution.  */ 

	static void copy_result(Execution *parent, Execution *child); 
	/* Copy the result list from CHILD to PARENT */
};

class Single_Execution
/*
 * Each file or transient target is represented at run time by one
 * Single_Execution object.  Each Single_Execution object may correspond
 * to multiple files or transients, but not to dynamic targets. 
 *
 * All Single_Execution objects are allocated with new Single_Execution(...), and are
 * never deleted, as the information contained in them needs to be
 * cached.
 */
	:  public Execution 
{
public:

	Single_Execution(Target target_,
			 Link &link,
			 Execution *parent);
	/* The TARGET must not by dynamic */ 
	// TODO remove the TARGET parameter (?).  It is already
	// contained in LINK.DEPENDENCY. 

	void propagate_variable(shared_ptr <Dependency> dependency,
			   Execution *parent); 
	/* Read the content of the file into a string as the
	 * variable value.  THIS is the variable target.  */

	bool is_dynamic() const;
	/* Whether the target is dynamic */ 

	shared_ptr <const Rule> get_rule() const { return rule; }

	void add_variables(map <string, string> mapping) {
		mapping_variable.insert(mapping.begin(), mapping.end()); 
	}

	const map <string, string> &get_mapping_variable() const {
		return mapping_variable; 
	}

	virtual string debug_done_text() const {
		return done.format(); 
	}

	virtual Proceed execute(Execution *parent, const Link &link);
	virtual bool finished() const;
	virtual bool finished(Stack avoid) const; 

	static unordered_map <pid_t, Single_Execution *> executions_by_pid;
	/* The currently running executions by process IDs */ 

	static void wait();
	/* Wait for next job to finish and finish it.  Do not start anything
	 * new.  */ 


protected:

	virtual bool optional_finished(const Link &);
	virtual void check_execution(const Link &link) const;
	virtual bool want_delete() const {  return false;  }

	virtual string debug_text() const {
		assert(targets.size()); 
		return targets.front().format_out(); 
	}

private:

	friend class Execution; 
	friend void job_terminate_all(); 
	friend void job_print_jobs(); 

	vector <Target> targets; 
	/* The targets to which this execution object corresponds.
	 * Otherwise, all entries have the same depth.  If the dynamic
	 * depth is larger than one, then there is exactly one target.
	 * There are multiple targets here when the rule had multiple
	 * targets.  Never empty.  */   

	shared_ptr <Rule> rule;
	/* The instantiated file rule for this execution.  Null when
	 * there is no rule for this file (this happens for instance
	 * when a source code file is given as a dependency, or when
	 * this is a complex dependency).  Individual dynamic
	 * dependencies do have rules, in order for cycles to be
	 * detected.  Null if and only if PARAM_RULE is null.  */ 

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

	bool checked;
	/* Whether we performed the check in execute()  */ 

	signed char exists;
	/* 
	 * Whether the file target(s) are known to exist.  
	 *     -1 = at least one file target is known not to exist (only
	 *     	    possible when there is at least one file target)
	 *      0 = status unknown:  nothing has been checked yet
	 *     +1 = all file targets are known to exist (possible when
	 *          there are no file targets)
	 * When there are no file targets (i.e., when all targets are
	 * transients), the value may be both 0 or +1.  
	 */
	
	Stack done;
	/* What parts of this target have been done.  Each bit
	 * represents one aspect that was done.  The depth is always
	 * zero.  */
	// TODO replace by a single Flag, since it always has depth
	// zero. 

	~Single_Execution(); 

	virtual const Place &get_place() const {
		if (param_rule == nullptr)
			return Place::place_empty;
		else
			return param_rule->place; 
	}

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

	int get_depth() const {
		return 0; 
	}

	void done_set_all_one() 
	/* Set all flags in DONE, given that the depth of DONE is zero */
	// TODO deprecate and replace by direct call
	{
		done.add_one_neg(0); 
	}

	void done_add_neg(Stack stack_) {
	// TODO deprecate and replace by direct call
		done.add_neg(stack_); 
	}

	void done_add_one_neg(Flags flags_) {
	// TODO deprecate and replace by direct call
		done.add_one_neg(flags_); 
	}

	void done_add_one_neg(Stack stack_) {
	// TODO deprecate and replace by direct call
		done.add_one_neg(stack_); 
	}

	void done_add_highest_neg(Flags flags_) {
	// TODO deprecate and replace by direct call
		done.add_highest_neg(flags_); 
	}

	static unordered_map <string, Timestamp> transients;
	/* The timestamps for transient targets.  This container plays the role of
	 * the file system for transient targets, holding their timestamps, and
	 * remembering whether they have been executed.  Note that if a
	 * rule has both file targets and transient targets, and all
	 * file targets are up to date and the transient targets have
	 * all their dependencies up to date, then the command is not
	 * executed, even though it was never executed in the current
	 * invocation of Stu. In that case, the transient targets are
	 * never insert in this map.  */
};

class Root_Execution
	:  public Execution
{
public:

	Root_Execution(const vector <shared_ptr <Dependency> > &dependencies); 

	virtual Proceed execute(Execution *parent, const Link &link);
	virtual bool finished() const; 
	virtual bool finished(Stack avoid) const;
	virtual void next_list(vector <shared_ptr <Dependency> > &) const {  assert(false);  }

protected:

	virtual int get_depth() const {  return -1;  }
	virtual const Place &get_place() const {  return Place::place_empty;  }
	virtual bool optional_finished(const Link &) {  return false;  }
	virtual void check_execution(const Link &) const  {   }
	virtual string debug_text() const { return "ROOT"; }
	virtual bool want_delete() const {  return true;  }

private:

	Stack done; 
	/* Always has depth zero */
};

class Concatenated_Execution
/* 
 * An execution representating a concatenation.  Its dependency is
 * always a compound dependency containing normalized dependencies, whose
 * results are concatenated as new targets added to the parent.
 *
 * Concatenated executions always have exactly one parent.  They are not
 * cached, and they are deleted when done.  Thus, they also don't need
 * the 'done' field.  (But the parent class has it.)
 */
	:  public Execution
{
public:

	Concatenated_Execution(shared_ptr <Dependency> dependency_,
			       Link &link,
			       Execution *parent);
	/* The given dependency must be normalized, and contain at least
	 * one Concatenated_Dependency.  */

	~Concatenated_Execution(); 

	void add_part(shared_ptr <Single_Dependency> dependency, 
		      int concatenation_index);
	/* Add a single part -- exclude the outer layer */

	void assemble_parts(); 

	virtual int get_depth() const { return -1; }
	virtual const Place &get_place() const {  return dependency->get_place();  }
	virtual Proceed execute(Execution *parent, const Link &link);
	virtual bool finished() const;
	virtual bool finished(Stack avoid) const; 
	virtual void next_list(vector <shared_ptr <Dependency> > &list) const;

	virtual string debug_text() const {  return "CONCAT";  }

protected:

	virtual bool optional_finished(const Link &) {  return false;  }

#ifndef NDEBUG
	virtual void check_execution(const Link &) const  {  }
	/* Perform consistency assertions */ 
#endif

private:

	shared_ptr <Dependency> dependency;
	/* Contains the concatenation. 
	 * This is a Dynamic_Dependency^* of a Concatenated_Dependency,
	 * itself containing each a Compound_Dependency^{0,1} of
	 * Dynamic_Dependency^* of a single dependency. 
	 * Is normalized.  */

	int stage;
	/* 0:  Nothing done yet. 
	 *  --> put dependencies into the queue
	 * 1:  We're building the normal dependencies.
	 *  --> read out the dependencies and construct the list of actual dependencies
	 * 2:  Building actual dependencies.
	 * 3:  Finished.  */
	
	vector <vector <shared_ptr <Single_Dependency> > > parts; 
	/* 
	 * The individual parts, inserted here during stage 1 by
	 * Execution::unlink().  
	 * Excludes the outer layer. 
	 */

	void add_stage0_dependency(shared_ptr <Dependency> d, unsigned concatenate_index);
	/* Add a dependency during Stage 0.  The given dependency can be
	 * non-normalized, because it comes from within a concatenated
	 * dependency.  */
	
//	void read_concatenation(Stack avoid,
//				shared_ptr <Dependency> dependency,
//				vector <shared_ptr <Dependency> > &dependencies_read);
//	/* Extract individual dependencies from a given concatenated
//	 * dependency.  The read dependencies are stored into
//	 * DEPENDENCIES_READ, which must be empty on calling this
//	 * function.  DEPENDENCY has the same structural constraints as
//	 * this->dependency.  Assume that all mentioned dependencies
//	 * have been built.  The only reason this is not static is that
//	 * errors can be raised.  */

	// void concatenate_dependency(Stack avoid,
	// 			    shared_ptr <Dependency> dependency_1,
	// 			    shared_ptr <Dependency> dependency_2,
	// 			    Flags dependency_flags,
	// 			    vector <shared_ptr <Dependency> > &dependencies);
	// /* Concatenate DEPENDENCY_{1,2}, adding flags from
	//  * DEPENDENCY_FLAGS, appending the result to DEPENDENCIES.  Each
	//  * of the parameters DEPENDENCY_{1,2} is a
	//  * Dynamic_Dependency^{0,1} of Single_Dependency.  
	//  * The only reason this is not static is that errors can be
	//  * raised.  */

	virtual bool want_delete() const {  return true;  }

	static shared_ptr <Dependency> concatenate_dependency_one(shared_ptr <Single_Dependency> dependency_1,
								  shared_ptr <Single_Dependency> dependency_2,
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
 */
	:  public Execution 
{
public:
	
	Dynamic_Execution(Link &link, Execution *parent);

	virtual Proceed execute(Execution *parent, const Link &link);
	virtual bool finished() const;
	virtual bool finished(Stack avoid) const; 
	virtual int get_depth() const {  return done.get_depth();  }
	virtual const Place &get_place() const {  return dependency->get_place();  }
	virtual bool optional_finished(const Link &) {  return false;  }

	void propagate_to_dynamic(Execution *child,
				  Flags flags_child,
				  Stack avoid_parent,
				  shared_ptr <Dependency> dependency_parent,
				  shared_ptr <Dependency> dependency_child);
	/* Propagate dynamic dependencies from CHILD to its parent (THIS)  */ 


protected:

	virtual string debug_text() const {  return "DYNAMIC";  }
	virtual bool want_delete() const;

#ifndef NDEBUG
	virtual void check_execution(const Link &) const {  } 
	/* Perform consistency assertions */ 
#endif

private: 

	shared_ptr <Dynamic_Dependency> dependency; 
	/* A dynamic of anything */

	Stack done;
	/* Same semantics as in Single_Execution */ 

	shared_ptr <Rule> param_rule;
	/* The (possibly parametrized) rule from which this execution
	 * was derived.  This is only used to detect strong cycles.  To
	 * manage the dependencies, the instantiated general rule is
	 * used.  Null if and only if RULE is null.  */ 

//	void read_dynamic_dependency(Stack avoid_this, shared_ptr <Dependency> dependency_this);
//	/* Read dynamic dependencies from a file.  Called for the parent
//	 * of a left dynamic branch.  */ 
};

long Execution::jobs= 1;
Rule_Set Execution::rule_set; 
Timestamp Execution::timestamp_last;
bool Execution::hide_out_message= false;
bool Execution::out_message_done= false;
unordered_map <Target, Execution *> Execution::executions_by_target;

unordered_map <pid_t, Single_Execution *> Single_Execution::executions_by_pid;
unordered_map <string, Timestamp> Single_Execution::transients;

Execution::~Execution()
{
	/* Nop */
}

void Execution::main(const vector <shared_ptr <Dependency> > &dependencies)
{
	assert(jobs >= 0);
	timestamp_last= Timestamp::now(); 
	Root_Execution *root_execution= new Root_Execution(dependencies); 
	int error= 0; 

	try {
		while (! root_execution->finished()) {

			Link link(Stack(), (Flags) 0, Place(), shared_ptr <Dependency> ());

			Proceed proceed;
			do {
				if (option_debug) {
					fprintf(stderr, "DEBUG %s main.next\n", 
						Verbose::padding());
				}
				proceed= root_execution->execute(nullptr, move(link));
//				assert(! (proceed & P_BIT_FINISHED)); 
			} while (proceed & P_BIT_LATER);

			if (Single_Execution::executions_by_pid.size()) {
				Single_Execution::wait();
			}
		}

		assert(root_execution->finished()); 
		assert(Single_Execution::executions_by_pid.size() == 0);

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
			if (option_keep_going) 
				print_error_reminder("Targets not rebuilt because of errors");
		}
	} 

	/* A build error is only thrown when option_keep_going is
	 * not set */ 
	catch (int e) {

		assert(! option_keep_going); 
		assert(e >= 1 && e <= 4); 

		/* Terminate all jobs */ 
		if (Single_Execution::executions_by_pid.size()) {
			print_error_reminder("Terminating all jobs"); 
			job_terminate_all();
		}

		if (e == ERROR_FATAL)
			exit(ERROR_FATAL); 

		error= e; 
	}

	if (error)
		throw error; 
}

void Execution::read_dynamic(Stack avoid, 
			     shared_ptr <Dynamic_Dependency> dependency_this, 
			     vector <shared_ptr <Dependency> > &dependencies)
{
	assert(dependency_this->is_normalized()); 

	Target target= dependency_this->get_individual_target().unparametrized(); 

	assert(dependencies.empty()); 
	assert(target.type.is_dynamic());
	assert(target.type.is_any_file()); 
	assert(avoid.get_depth() == target.type.get_depth()); 

	string filename= target.name;

	Flags flags= dependency_this->dependency->get_flags();

	if (! (flags & (F_NEWLINE_SEPARATED | F_NUL_SEPARATED))) {

		/* Parse dynamic dependency in full Stu syntax */ 

		vector <shared_ptr <Token> > tokens;
		Place place_end; 

		Tokenizer::parse_tokens_file
			(tokens, 
			 Tokenizer::DYNAMIC,
			 place_end, filename, dependency_this->get_place()); 

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
			place_input <<
				fmt("dynamic dependency %s "
				    "must not contain input redirection %s", 
				    target.format_word(),
				    prefix_format_word(input.raw(), "<")); 
			Target target_file= target;
			target_file.type= Type::FILE;
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
			
		const char c= (flags & F_NEWLINE_SEPARATED) ? '\n' : '\0';
		/* The delimiter */ 

		const char c_printed
			/* The character to print as the delimiter */
			= (flags & F_NEWLINE_SEPARATED) ? 'n' : '0';
		
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
				(make_shared <Single_Dependency>
				 (0,
				  Place_Param_Target
				  (Type::FILE, 
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
			shared_ptr <Dependency> dep= j;
			while (dynamic_pointer_cast <Dynamic_Dependency> (dep)) {
				shared_ptr <Dynamic_Dependency> dep2= 
					dynamic_pointer_cast <Dynamic_Dependency> (dep);
				dep= dep2->dependency; 
			}
			dynamic_pointer_cast <Single_Dependency> (dep)
				->place_param_target.place_name.places[0] <<
				fmt("dynamic dependency %s must not contain "
				    "parametrized dependencies",
				    target.format_word());
			Target target_base= target;
			target_base.type= target.type.get_base();
			print_traces(fmt("%s is declared here", 
					 target_base.format_word())); 
			raise(ERROR_LOGICAL);
			j= nullptr;
			found_error= true; 
			continue; 
		}

		/* Check that there is no multiply-dynamic variable dependency */ 
		if (j->has_flags(F_VARIABLE) && 
		    target.type.is_dynamic() && target.type != Type::DYNAMIC_FILE) {
			
			/* Only single dependencies can have the F_VARIABLE flag set */ 
			assert(dynamic_pointer_cast <Single_Dependency> (j));
			
			shared_ptr <Single_Dependency> dep= 
				dynamic_pointer_cast <Single_Dependency> (j);

			bool quotes= false;
			string s= dep->place_param_target.format(0, quotes);

			j->get_place() <<
				fmt("variable dependency %s$[%s%s%s]%s must not appear",
				    Color::word,
				    quotes ? "'" : "",
				    s,
				    quotes ? "'" : "",
				    Color::end);
			print_traces(fmt("within multiply-dynamic dependency %s", 
					 target.format_word())); 
			raise(ERROR_LOGICAL);
			j= nullptr; 
			found_error= true; 
			continue; 
		}
	}
	if (found_error) {
		assert(option_keep_going); 
		vector <shared_ptr <Dependency> > dependencies_new;
		for (auto &j:  dependencies) {
			if (j)
				dependencies_new.push_back(j); 
		}
		swap(dependencies, dependencies_new); 
	}
}

bool Execution::find_cycle(const Execution *const parent, 
			   const Execution *const child,
			   const Link &link)
{
	if (dynamic_cast <const Root_Execution *> (parent))
		return false;
		
	/* Happens with files that should be there and have no rule */ 
//	if (child->get_param_rule() == nullptr)
//		return false; 

	vector <const Execution *> path;
	path.push_back(parent); 

	return find_cycle(path, child, link); 
}

bool Execution::find_cycle(vector <const Execution *> &path,
			   const Execution *const child,
			   const Link &link)
{
	if (same_rule(path.back(), child)) {
		cycle_print(path, link); 
		return true; 
	}

	for (auto &i:  path.back()->parents) {
		const Execution *next= i.first; 
		assert(next != nullptr);
		if (dynamic_cast <const Root_Execution *> (next)) 
			continue;

		path.push_back(next); 

		bool found= find_cycle(path, child, link);
		if (found)
			return true;

		path.pop_back(); 
	}
	
	return false; 
}

void Execution::cycle_print(const vector <const Execution *> &path,
			    const Link &link)
{
	assert(path.size() > 0); 

	/* Indexes are parallel to PATH */ 
	vector <string> names;
	names.resize(path.size());
	
	for (unsigned i= 0;  i + 1 < path.size();  ++i) {
		names[i]= 
			path[i]->parents.at((Execution *) path[i+1])
			.dependency->get_individual_target().format_word();
	}

	names.back()= path.back()->parents.begin()->second
		.dependency->get_individual_target().format_word();
		
	for (ssize_t i= path.size() - 1;  i >= 0;  --i) {

		/* Don't show a message for left-branch dynamic links */ 
		if (i != 0 &&
		    path[i - 1]->parents.at(const_cast <Execution *> (path[i]))
		    .dependency->get_flags() & F_DYNAMIC_LIST)
			continue;

		/* Same, but when the dynamic execution is at the bottom */
		if (i == 0 && link.dependency->get_flags() & F_DYNAMIC_LIST) 
			continue;

		(i == 0 ? link : path[i - 1]->parents.at((Execution *) path[i])).place
			<< fmt("%s%s depends on %s",
			       i == (ssize_t)(path.size() - 1) 
			       ? (path.size() == 1 
				  || (path.size() == 2 &&
				      link.dependency->get_flags() & F_DYNAMIC_LIST)
				  ? "target must not depend on itself: " 
				  : "cyclic dependency: ") 
			       : "",
			       names[i],
			       i == 0
			       ? link.dependency->get_individual_target().format_word()
			       : names[i - 1]);
	}

	/* If the two targets are different (but have the same rule
	 * because they match the same pattern), then output a notice to
	 * that effect */ 
	if (link.dependency->get_individual_target() !=
	    path.back()->parents.begin()->second
	    .dependency->get_individual_target()) {

		
		Param_Target t1= path.back()->parents.begin()->second
			.dependency->get_individual_target();
		Param_Target t2= link.dependency->get_individual_target();
		t1.type= t1.type.get_base();
		t2.type= t2.type.get_base(); 

		path.back()->get_place() <<
			fmt("both %s and %s match the same rule",
			    t1.format_word(), t2.format_word());
	}

	path.back()->print_traces();

	explain_cycle(); 
}

bool Execution::same_rule(const Execution *execution_a,
			  const Execution *execution_b)
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

	string text_parent= parents.begin()->second.dependency
		->get_individual_target().format_word();

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

				i->second.place <<
					fmt("no rule to build %s", 
					    text_parent);
			}
			break; 
		}

		string text_child= text_parent; 

		text_parent= i->first->parents.begin()->second.dependency
			->get_individual_target().format_word();

 		const Place place= i->second.place;
		/* Set even if not output, because it may be used later
		 * for the root target  */

		/* Don't show left-branch edges of dynamic executions */
		if (i->second.flags & F_DYNAMIC_LIST) {
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

Execution::Proceed Execution::execute_children(const Link &link, Stack &done_here)
{
	/* Since unlink() may change execution->children, we must first
	 * copy it over locally, and then iterate through it */ 

	vector <Execution *> executions_children_vector
		(children.begin(), children.end()); 

	Proceed proceed_all= P_CONTINUE;

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

		Stack avoid_child= child->parents.at(this).avoid;
		Flags flags_child= child->parents.at(this).flags;

		if (link.dependency != nullptr 
		    && dynamic_pointer_cast <Single_Dependency> (link.dependency)
		    && dynamic_pointer_cast <Single_Dependency> (link.dependency)
		    ->place_param_target.type == Type::TRANSIENT) {
			flags_child |= link.flags; 
		}

		shared_ptr <Dependency> dependency_child= child->parents.at(this).dependency;
		
		Link link_child(avoid_child, flags_child, child->parents.at(this).place,
				dependency_child);

		Proceed proceed_child= child->execute(this, move(link_child));
//		assert(! (proceed & P_BIT_FINISHED)); 
//		assert(proceed != P_CONTINUE); 

		proceed_all |= proceed_child; 

		if (child->finished(done_here)) {
//		if (proceed & P_BIT_FINISHED) {
			unlink(this, child, 
			       link.dependency,
			       link.avoid, 
			       dependency_child, avoid_child, flags_child); 
		}
	}

	if (error) {
		assert(option_keep_going); 
	}

	if ((proceed_all & (P_BIT_WAIT | P_BIT_LATER)) == P_CONTINUE) {
		/* If there are still children, they must have returned
		 * WAIT or LATER */ 
		assert(children.empty()); 
		if (error) {
			done_here.add_neg(link.avoid); 
//			proceed_all |= P_BIT_FINISHED; 
		}
//		assert(finished(link.avoid)); 
	}

	return proceed_all; 
}

void Execution::push_default(shared_ptr <Dependency> dependency)
{
	vector <shared_ptr <Dependency> > dependencies;
	Dependency::make_normalized(dependencies, dependency); 
       
	for (const auto &d:  dependencies) {
		buffer_default.push(d);
	}
}

Execution::Proceed Execution::execute_base(const Link &link, Stack &done_here)
{
	Verbose verbose;

	assert(jobs >= 0); 

#ifndef NDEBUG
	check_execution(link); 
#endif

	if (option_debug) {
		string text_target= debug_text(); 
		string text_flags= flags_format(link.flags);
		string text_avoid= link.avoid.format(); 

		fprintf(stderr, "DEBUG %s %s execute %s %s\n", 
			Verbose::padding(),
			text_target.c_str(),
			text_flags.c_str(),
			text_avoid.c_str()); 
	}

	/* Override the trivial flag */ 
	Link link2{link}; 
	if (link2.flags & F_OVERRIDE_TRIVIAL) {
		link2.flags &= ~F_TRIVIAL; 
		link2.avoid.rem_highest(F_TRIVIAL); 
	}

 	if (finished(link2.avoid)) {
		if (option_debug) {
			string text_target= debug_text(); 
			fprintf(stderr, "DEBUG %s %s finished\n",
				Verbose::padding(),
				text_target.c_str());
		}
		return P_CONTINUE; 
//		return P_BIT_FINISHED;
	}

	/* In DFS mode, first continue the already-open children, then
	 * open new children.  In random mode, start new children first
	 * and continue already-open children second */ 

	/* 
	 * Continue the already-active child executions 
	 */  

	Proceed proceed_all= P_CONTINUE; 

	if (order != Order::RANDOM) {
		Proceed proceed= execute_children(link2, done_here);
		proceed_all |= proceed;
//		proceed_all |= (proceed & ~P_BIT_FINISHED); 
		if (proceed_all & P_BIT_WAIT) {
			return proceed_all; 
		}
		if (
		    finished(link2.avoid) 
		    && ! option_keep_going) {
			if (option_debug) {
				string text_target= debug_text(); 
				fprintf(stderr, "DEBUG %s %s finished\n",
					Verbose::padding(),
					text_target.c_str());
			}
//			done_here.add_neg(link2.avoid); 
			return proceed_all;
		}
	}

	// TODO but this *before* the execution of already-opened
	// children. 
	if (optional_finished(link2)) {
//	proceed_all |= proceed_optional; 
//	if (proceed_optional & P_BIT_FINISHED) {
		return proceed_all;
	}

	/* Is this a trivial run?  Then skip the dependency. */
	if (link2.flags & F_TRIVIAL) {
		done_here.add_neg(link2.avoid); 
		return proceed_all; 
//		return proceed_all | P_BIT_FINISHED;
	}

	if (error) 
		assert(option_keep_going); 

	/* 
	 * Deploy dependencies (first pass), with the F_NOTRIVIAL flag
	 */ 

	if (jobs == 0) {
		assert(proceed_all & P_BIT_WAIT); 
		return proceed_all;
	}

	while (! buffer_default.empty()) {
		shared_ptr <Dependency> dependency_child= buffer_default.next(); 
		shared_ptr <Dependency> dependency_child_overridetrivial= 
			Dependency::clone_dependency(dependency_child);
		dependency_child_overridetrivial->add_flags(F_OVERRIDE_TRIVIAL); 
		buffer_trivial.push(dependency_child_overridetrivial); 
		Proceed proceed_2= execute_deploy(link2, dependency_child);
		proceed_all |= proceed_2;
//		if (jobs == 0)
//			return proceed_all;
	} 
	assert(buffer_default.empty()); 

	if (order == Order::RANDOM) {
		Proceed proceed_2= execute_children(link2, done_here);
		if (proceed_2 & P_BIT_WAIT)
			return proceed_2;
		if (
		    finished(done_here)
//		    (proceed_2 & P_BIT_FINISHED) 
		    && ! option_keep_going) {
//			done_here.add_neg(link2.avoid); 
			return proceed_2;
		}
	}

	/* Some dependencies are still running */ 
	if (! children.empty()) {
		assert(proceed_all != P_CONTINUE); 
		return proceed_all;
	}

	/* There was an error in a child */ 
	if (error) {
		assert(option_keep_going == true); 
		done_here.add_neg(link2.avoid); 
		return proceed_all;
	}

	return proceed_all; 
}

Execution::Proceed Execution::execute_deploy(const Link &link,
					     shared_ptr <Dependency> dependency_child)
{
	assert(dependency_child->is_normalized()); 

	if (option_debug) {
		string text_target= debug_text();
		string text_child= dependency_child->format_out(); 
		fprintf(stderr, "DEBUG %s %s deploy %s\n",
			Verbose::padding(),
			text_target.c_str(),
			text_child.c_str());
	}

	Flags flags_child= dependency_child->get_flags(); 
	Flags flags_child_additional= 0; 

	// TODO check directly whether the top-level dependency is
	// dynamic, and if it is, return a Dynamic_Execution

	unsigned depth= 0;
	shared_ptr <Dependency> dep= dependency_child;
	Stack avoid_child;
	avoid_child.add_lowest(dep->get_flags()); 
	while (dynamic_pointer_cast <Dynamic_Dependency> (dep)) {
		dep= dynamic_pointer_cast <Dynamic_Dependency> (dep)->dependency;
		++depth;
		avoid_child.push();
		avoid_child.add_lowest(dep->get_flags()); 
	}

	if (dynamic_pointer_cast <Concatenated_Dependency> (dep)) {
		/* This is a concatenated dependency:  Create a new
		 * concatenated execution for it. */ 
		shared_ptr <Concatenated_Dependency> concatenated_dependency=
			dynamic_pointer_cast <Concatenated_Dependency> (dep); 

		Link link_child_new(avoid_child, flags_child,
				    dependency_child->get_place(),
				    dependency_child);

		Concatenated_Execution *child= new Concatenated_Execution
			(dependency_child, link_child_new, this);

		if (child == nullptr) {
			/* Strong cycle was found */ 
			return P_CONTINUE;
		}

		children.insert(child);

		Proceed proceed_child= child->execute(this, move(link_child_new));
//		assert(! (proceed & P_BIT_FINISHED)); 
		if (proceed_child & P_BIT_WAIT)
			return proceed_child; 
//			return (proceed & ~P_BIT_FINISHED);
//		assert(jobs >= 1); 
			
		if (child->finished(avoid_child)) {
			unlink(this, child, 
			       link.dependency,
			       link.avoid, 
			       dependency_child, avoid_child, flags_child);
		}

		return P_CONTINUE;

	} else if (dynamic_pointer_cast <Single_Dependency> (dep)) {

 		shared_ptr <Single_Dependency> single_dependency=
			dynamic_pointer_cast <Single_Dependency> (dep);
		assert(single_dependency != nullptr); 
		assert(! single_dependency->place_param_target.place_name.empty()); 
		Target target_child= single_dependency->place_param_target.unparametrized();
		assert(target_child.type == Type::FILE || target_child.type == Type::TRANSIENT);
		assert(target_child.type.get_depth() == 0); 
		if (depth != 0) {
			assert(depth > 0);
			target_child.type += depth; 
		}

		/* Carry flags over transient targets */ 
		// TODO instead of this test, use a new virtual function
		// Dependency::get_type() to determine whether this is a
		// transient. 
		if (link.dependency != nullptr &&
		    dynamic_pointer_cast <Single_Dependency> (link.dependency)) {
			Target target= link.dependency->get_individual_target().unparametrized();

			if (target.type == Type::TRANSIENT) { 
				flags_child_additional |= link.flags; 
				if (link.flags & F_PERSISTENT) {
					dependency_child->set_place_flag
						(I_PERSISTENT,
						 link.dependency->get_place_flag(I_PERSISTENT)); 
				}
				if (link.flags & F_OPTIONAL) {
					dependency_child->set_place_flag
						(I_OPTIONAL,
						 link.dependency->get_place_flag(I_OPTIONAL)); 
				}
				if (link.flags & F_TRIVIAL) {
					dependency_child->set_place_flag
						(I_TRIVIAL,
						 link.dependency->get_place_flag(I_TRIVIAL)); 
				}
			}
		}
	
		Flags flags_child_new= flags_child | flags_child_additional; 
		
		/* '-p' and '-o' do not mix, even for old flags */ 
		if ((flags_child_new & F_PERSISTENT) && 
		    (flags_child_new & F_OPTIONAL)) {

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
			single_dependency->place <<
				fmt("in declaration of dependency %s", 
				    target_child.format_word());
			print_traces();
			explain_clash(); 
			raise(ERROR_LOGICAL);
			return P_CONTINUE;
		}

		/* Either of '-p'/'-o'/'-t' does not mix with '$[' */
		if ((flags_child & F_VARIABLE) &&
		    (flags_child_additional & (F_PERSISTENT | F_OPTIONAL | F_TRIVIAL))) {

			assert(target_child.type == Type::FILE); 
			const Place &place_variable= single_dependency->place;
			if (flags_child_additional & F_PERSISTENT) {
				const Place &place_flag= 
					dependency_child->get_place_flag(I_PERSISTENT); 
				place_variable << 
					fmt("variable dependency %s must not be declared "
					    "as persistent dependency",
					    dynamic_variable_format_word(target_child.name)); 
				place_flag << fmt("using %s",
						  multichar_format_word("-p")); 
			} else if (flags_child_additional & F_OPTIONAL) {
				const Place &place_flag= 
					dependency_child->get_place_flag(I_OPTIONAL); 
				place_variable << 
					fmt("variable dependency %s must not be declared "
					    "as optional dependency",
					    dynamic_variable_format_word(target_child.name)); 
				place_flag << fmt("using %s",
						  multichar_format_word("-o")); 
			} else {
				assert(flags_child_additional & F_TRIVIAL); 
				const Place &place_flag= 
					dependency_child->get_place_flag(I_TRIVIAL); 
				place_variable << 
					fmt("variable dependency %s must not be declared "
					    "as trivial dependency",
					    dynamic_variable_format_word(target_child.name)); 
				place_flag << fmt("using %s",
						  multichar_format_word("-t")); 
			} 
			print_traces();
			raise(ERROR_LOGICAL);
			return P_CONTINUE;
		}

		flags_child= flags_child_new; 

		Link link_child_new(avoid_child, flags_child, 
				    dependency_child->get_place(), 
				    dependency_child); 

		Execution *child= get_execution(target_child, link_child_new, this);  
		if (child == nullptr) {
			/* Strong cycle was found */ 
			return P_CONTINUE;
		}

		children.insert(child);

		Proceed proceed_child= child->execute(this, move(link_child_new));
//		assert(! (proceed & P_BIT_FINISHED)); 
		if (proceed_child & (P_BIT_WAIT | P_BIT_LATER))
			return proceed_child; 
//			return proceed & ~P_BIT_FINISHED;
//		assert(jobs >= 1); 
			
		if (child->finished(avoid_child)) {
			unlink(this, child, 
			       link.dependency,
			       link.avoid, 
			       dependency_child,
			       avoid_child, flags_child);
		}

		return P_CONTINUE;

	} else {
		/* Invalid dependency type.  The dependency must be normalized. */ 
		assert(false); 
		return P_CONTINUE;
	}
}

void Execution::raise(int error_)
{
	assert(error_ >= 1 && error_ <= 3); 

	error |= error_;

	if (! option_keep_going)
		throw error;
}

void Execution::unlink(Execution *const parent, 
		       Execution *const child,
		       shared_ptr <Dependency> dependency_parent,
		       Stack avoid_parent,
		       shared_ptr <Dependency> dependency_child,
		       Stack avoid_child,
		       Flags flags_child)
{
	(void) avoid_child;

	if (option_debug) {
		string text_parent= parent->debug_text();
		string text_child= child->debug_text();
		string text_done_child= child->debug_done_text();
		fprintf(stderr, "DEBUG %s %s unlink %s %s\n",
			Verbose::padding(),
			text_parent.c_str(),
			text_child.c_str(),
			text_done_child.c_str());
	}

	assert(parent != nullptr);
	assert(child != nullptr); 
	assert(parent != child); 
	assert(child->finished(avoid_child)); 

	if (! option_keep_going)  
		assert(child->error == 0); 

	/*
	 * Propagations
	 */

	/* Propagate dynamic dependencies */ 
	if (flags_child & F_DYNAMIC_LIST) {
		/* This was the left branch between a dynamic dependency
		 * and its child.  Add the right branch.  */

		Dynamic_Execution *parent_dynamic= dynamic_cast <Dynamic_Execution *> (parent);
		assert(parent_dynamic); 

		parent_dynamic->propagate_to_dynamic(child,
						     flags_child,
						     avoid_parent,
						     dependency_parent,
						     dependency_child);  
	}

	// /* Propagate concatenated dependencies */
	// if ((flags_child & F_CONCATENATE) && ! (flags_child & F_DYNAMIC)) {
	// 	unsigned concatenation_index= 
	// 		(flags_child >> C_CONCATENATE_BASE) & ((1 << C_CONCATENATE_COUNT) - 1); 

	// 	// TODO if DEPENDENCY_CHILD is a Single_Dependency, add
	// 	// it to the parts of the parent.  If it is dynamic, do
	// 	// the same that is done
	// 	...; 

	// 	// shared_ptr <Dynamic_Dependency> dynamic_dependency_child=
	// 	// 	dynamic_pointer_cast <Dynamic_Dependency> (dependency_child); 
	// 	// if (dynamic_dependency_child) {
	// 	// 	shared_ptr <Single_Dependency> dependency_inner=
	// 	// 		dynamic_pointer_cast <Single_Dependency> (dynamic_dependency_child->dependency); 
	// 	// 	if (dependency_inner && dependency_inner->place_param_target.type == Type::FILE) {
	// 	// 		vector <shared_ptr <Single_Dependency> > list;
	// 	// 		shared_ptr <Dynamic_Dependency> dynamic_dependency
	// 	// 			= make_shared <Dynamic_Dependency> 
	// 	// 			(flags_child & ~(((1 << C_CONCATENATE_COUNT) - 1) << C_CONCATENATE_BASE), 
	// 	// 			 dependency_inner); 
	// 	// 		parent->read_dynamic(avoid_parent, dynamic_dependency, list); 
	// 	// 		for (auto &i:  list) {
	// 	// 			dynamic_cast <Concatenated_Execution *> (parent)
	// 	// 				->add_part(i, concatenation_index);
	// 	// 		}
	// 	// 	}
	// 	// }
	// }

	/* Propagate timestamp.  Note:  When the parent execution has
	 * filename == "", this is unneccesary, but it's easier to not
	 * check, since that happens only once. */
	/* Don't propagate the timestamp of the dynamic dependency itself */ 
	if (! (flags_child & F_PERSISTENT) && ! (flags_child & F_DYNAMIC_LIST)) {
		if (child->timestamp.defined()) {
			if (! parent->timestamp.defined()) {
				parent->timestamp= child->timestamp;
			} else {
				if (parent->timestamp < child->timestamp) {
					parent->timestamp= child->timestamp; 
				}
			}
		}
	}

	/* Propagate variable dependencies */
	if (flags_child & F_VARIABLE) { 
		dynamic_cast <Single_Execution *> (child)
			->propagate_variable(dependency_child, parent);
	}

	/*
	 * Propagate variables over transient targets without commands
	 * and dynamic targets
	 */
	if ((dynamic_cast <Single_Execution *>(child) && 
	     dynamic_cast <Single_Execution *> (child)->is_dynamic()) ||
	    (dynamic_pointer_cast <Single_Dependency> (dependency_child)
	     && dynamic_pointer_cast <Single_Dependency> (dependency_child)
	     ->place_param_target.type == Type::TRANSIENT
	     && dynamic_cast <Single_Execution *> (child)->get_rule() != nullptr 
	     && dynamic_cast <Single_Execution *> (child)->get_rule()->command == nullptr)) {

		if (dynamic_cast <Single_Execution *> (parent)) {
			dynamic_cast <Single_Execution *> (parent)->add_variables
				(dynamic_cast <Single_Execution *> (child)->get_mapping_variable()); 
		}
	}

	/* 
	 * Propagate attributes
	 */ 

	/* Note: propagate the flags after propagating other things,
	 * since flags can be changed by the propagations done
	 * before.  */ 

	parent->error |= child->error; 

	if (child->need_build 
	    && ! (flags_child & F_PERSISTENT)
	    && ! (flags_child & F_DYNAMIC_LIST)) {
		parent->need_build= true; 
	}

	/* 
	 * Remove the links between them 
	 */ 

	assert(parent->children.count(child) == 1); 
	parent->children.erase(child);

	assert(child->parents.count(parent) == 1);
	child->parents.erase(parent);

	/*
	 * Delete the Execution object
	 */
	if (child->want_delete())
		delete child; 
}

Execution::Proceed Execution::execute_second_pass(const Link &link)
{
	Proceed proceed_all= P_CONTINUE;
	while (! buffer_trivial.empty()) {
		shared_ptr <Dependency> dependency_child= buffer_trivial.next(); 
		Proceed proceed= execute_deploy(link, dependency_child);
		proceed_all |= proceed; 
		assert(jobs >= 0);
//		if (jobs == 0)
//			return proceed_all; 
	} 
	assert(buffer_trivial.empty()); 

	return P_CONTINUE; 
}

Execution *Execution::get_execution(const Target &target, 
				    Link &link,
				    Execution *parent)
{
	/* Set to the returned Execution object when one is found or created */    
	Execution *execution= nullptr; 

	auto it= executions_by_target.find(target);

	if (it != executions_by_target.end()) {
		/* An Execution object already exists for the target */ 

		execution= it->second; 
		if (execution->parents.count(parent)) {
			/* The parent and child are already connected -- add the
			 * necessary flags */ 
			execution->parents.at(parent).add(link.avoid, 
							  link.flags);
		} else {
			/* The parent and child are not connected -- add the
			 * connection */ 
			execution->parents[parent]= link;
		}
		
	} else { 
		/* Create a new Execution object */ 

		if (! target.type.is_dynamic()) {
			execution= new Single_Execution(target, link, parent);  
		} else {
			execution= new Dynamic_Execution(link, parent); 
		}

		assert(execution->parents.size() == 1); 
	}

	if (find_cycle(parent, execution, link)) {
		parent->raise(ERROR_LOGICAL);
		return nullptr;
	}

//	execution->initialize(link.avoid); 

	return execution;
}

void Execution::copy_result(Execution *parent, Execution *child)
{
	parent->result= child->result; 
}

Single_Execution::~Single_Execution()
/* Objects of this type are never deleted */ 
{
	assert(false);
}

void Single_Execution::wait() 
{
	// TODO use a non-blocking wait() function to handle all
	// finished jobs in a loop in this function. 
	
	if (option_debug) {
		fprintf(stderr, "DEBUG %s wait\n",
			Verbose::padding()); 
	}

	assert(Single_Execution::executions_by_pid.size() != 0); 

	int status;
	pid_t pid= Job::wait(&status); 

	if (option_debug) {
		fprintf(stderr, "DEBUG %s wait pid = %ld\n", 
			Verbose::padding(),
			(long) pid);
	}

	timestamp_last= Timestamp::now(); 

	if (executions_by_pid.count(pid) == 0) {
		/* No Single_Execution is registered for the PID that
		 * just finished.  Should not happen, but since the PID
		 * value came from outside this process, we better
		 * handle this case gracefully, i.e., do nothing.  */
		print_warning(Place(), frmt("The function waitpid(2) returned the invalid proceed ID %jd", (intmax_t)pid)); 
		return; 
	}

	Single_Execution *const execution= executions_by_pid.at(pid); 

	execution->waited(pid, status); 

	++jobs; 
}

void Single_Execution::waited(pid_t pid, int status) 
{
	assert(job.started()); 
	assert(job.get_pid() == pid); 

	Execution::check_waited(); 

	assert(done.get_depth() == 0); 
	done_set_all_one(); 

	{
		Job::Signal_Blocker sb;
		executions_by_pid.erase(pid); 
	}

	/* The file(s) may have been built, so forget that it was known
	 * to not exist */
	if (exists < 0)  
		exists= 0;
	
	if (job.waited(status, pid)) {
		/* Command was successful */ 

		exists= +1; 
		/* Subsequently set to -1 if at least one target file is missing */

		/* For file targets, check that the file was built */ 
		for (unsigned i= 0;  i < targets.size();  ++i) {
			const Target &target= targets[i]; 

			if (target.type != Type::FILE)
				continue;

			struct stat buf;

			if (0 == stat(target.name.c_str(), &buf)) {

				/* The file exists */ 

				warn_future_file(&buf, 
						 target.name.c_str(),
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
					if (0 > lstat(target.name.c_str(), &buf)) {
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
				exists= -1;
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
			Target target= parents.begin()->second.dependency
				->get_individual_target().unparametrized(); 
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

Single_Execution::Single_Execution(Target target_,
				   Link &link,
				   Execution *parent)
	:  Execution(link, parent),
	   checked(false),
	   exists(0),
	   done(target_.type.get_depth(), 0)
{
	assert(parent != nullptr); 
	assert(parents.size() == 1); 
	assert(target_.type.get_depth() == 0); // CHANGED

	targets.push_back(target_); 

	/* 
	 * Fill in the rules and their parameters 
	 */ 
	if (target_.type == Type::FILE || target_.type == Type::TRANSIENT) {
		try {
			rule= rule_set.get(target_, param_rule, mapping_parameter, 
					   link.dependency->get_place()); 
		} catch (int e) {
			print_traces(); 
			raise(e); 
			return; 
		}
		if (rule == nullptr) {
			/* TARGETS contains only TARGET_ */
		} else {
			targets.clear(); 
			for (auto &place_param_target:  rule->place_param_targets) {
				targets.push_back(place_param_target->unparametrized()); 
			}
			assert(targets.size()); 
		}
	} else {
		assert(false); 

		// assert(target_.type.is_dynamic()); 
		// /* We must set the rule here, so cycles in the
		//  * dependency graph can be detected.  Note however that
		//  * the rule of dynamic file dependency executions is
		//  * otherwise not used.  */ 
		// Target target_base(target_.type.get_base(), target_.name);
		// try {
		// 	rule= rule_set.get(target_base, param_rule, mapping_parameter, 
		// 			   link.dependency->get_place()); 
		// } catch (int e) {
		// 	print_traces(); 
		// 	raise(e); 
		// 	return; 
		// }

		// /* For dynamic executions, the TARGETS variables
		//  * contains only a single target, which is already
		//  * contained in TARGETS.  */   
	}
	assert((param_rule == nullptr) == (rule == nullptr)); 

	/* Fill TARGETS with *all* targets from the rule */
	for (const Target &target:  targets) {
		executions_by_target[target]= this; 
	}

	if (option_debug) {
		string text_target= debug_text();
		string text_rule= rule == nullptr ? "(no rule)" : rule->format_out(); 
		fprintf(stderr, "DEBUG  %s   %s %s\n",
			Verbose::padding(),
			text_target.c_str(),
			text_rule.c_str()); 
	}

	if (! (target_.type.is_dynamic() && target_.type.is_any_file()) 
	    && rule != nullptr) {
		/* There is a rule for this execution */ 

		for (auto &dependency:  rule->dependencies) {

			shared_ptr <Dependency> dep= dependency;
			if (target_.type.is_any_transient()) {
				dep->add_flags(link.avoid.get_lowest());
			
				for (unsigned i= 0;  i < target_.type.get_depth();  ++i) {
					Flags flags= link.avoid.get(i + 1);
					dep= make_shared <Dynamic_Dependency> (flags, dep);
				}
			} 

			if (option_debug) {
				string text_target= debug_text();
				string text_link_new= dep->format_out(); 
				fprintf(stderr, "DEBUG %s    %s push %s\n",
					Verbose::padding(),
					text_target.c_str(),
					text_link_new.c_str()); 
			}
			push_default(dep); 
		}
	} else {
		/* There is no rule for this execution */ 

		bool rule_not_found= false;
		/* Whether to produce the "no rule to build target" error */ 

		if (target_.type == Type::FILE) {
			if (! (link.flags & F_OPTIONAL)) {
				/* Check that the file is present,
				 * or make it an error */ 
				struct stat buf;
				int ret_stat= stat(target_.name.c_str(), &buf);
				if (0 > ret_stat) {
					if (errno != ENOENT) {
						string text= target_.format_word();
						perror(text.c_str()); 
						raise(ERROR_BUILD); 
					}
					/* File does not exist and there is no rule for it */ 
					error |= ERROR_BUILD;
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
		} else if (target_.type == Type::TRANSIENT) {
			rule_not_found= true;
		} else {
			assert(false); 
//			assert(target_.type.is_dynamic()); 
		}
		
		if (rule_not_found) {
			assert(rule == nullptr); 
			print_traces(fmt("no rule to build %s", 
					 target_.format_word()));
			raise(ERROR_BUILD);
			/* Even when a rule was not found, the Single_Execution object remains
			 * in memory  */  
		}
	}

}

bool Single_Execution::finished() const 
{
	assert(done.get_depth() == 0); 

//	Flags to_do_aggregate= 0;
	
//	for (unsigned j= 0;  j <= done.get_depth();  ++j) {
//		to_do_aggregate |= ~done.get(j); 
//	}

	return ((~done.get(0)) & ((1 << C_TRANSITIVE) - 1)) == 0; 
}

bool Single_Execution::finished(Stack avoid) const
{
	assert(done.get_depth() == 0); 
	assert(avoid.get_depth() == 0); 

//	Flags to_do_aggregate= 0;
	
//	for (unsigned j= 0;  j <= done.get_depth();  ++j) {
//		to_do_aggregate |= ~done.get(j) & ~avoid.get(j); 
//	}

	return ((~done.get(0) & ~avoid.get(0)) & ((1 << C_TRANSITIVE) - 1)) == 0; 
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
	
	for (auto i= Single_Execution::executions_by_pid.begin();
	     i != Single_Execution::executions_by_pid.end();  ++i) {

		const pid_t pid= i->first;

		Job::kill(pid); 
	}

	int count_terminated= 0;

	for (auto i= Single_Execution::executions_by_pid.begin();
	     i != Single_Execution::executions_by_pid.end();  ++i) {

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
	for (auto &i:  Single_Execution::executions_by_pid) {
		i.second->print_as_job(); 
	}
}

bool Single_Execution::remove_if_existing(bool output) 
{
	if (option_no_delete)
		return false;

	/* Whether anything was removed */ 
	bool removed= false;

	for (unsigned i= 0;  i < targets.size();  ++i) {
		const Target &target= targets[i]; 

		if (target.type != Type::FILE)  
			continue;

		const char *filename= target.name.c_str();

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

		if (option_debug) {
			string text_filename= name_format_word(filename); 
			fprintf(stderr, "DEBUG  remove %s\n",
				text_filename.c_str()); 
		}
		
		if (output) {
			print_error_reminder(fmt("Removing file %s because command failed",
						 name_format_word(filename))); 
		}
			
		removed= true;

		if (0 > ::unlink(filename)) {
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

void Single_Execution::warn_future_file(struct stat *buf, 
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

void Single_Execution::print_command() const
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

// void Single_Execution::initialize(Stack avoid) 
// {
// 	/* Add the special dynamic dependency, i.e., the [...[A]...]--A type
// 	 * link.  Add, as an initial dependency, the corresponding file
// 	 * or transient.  */
// 	if (targets.front().type.is_dynamic()) {

// 		assert(targets.size() == 1); 
// 		const Target &target= targets.front(); 

// 		Flags flags_child= avoid.get_lowest();
		
// 		if (target.type.is_any_file())
// 			flags_child |= F_DYNAMIC;

// 		shared_ptr <Dependency> dependency_child= make_shared <Single_Dependency>
// 			(flags_child,
// 			 Place_Param_Target(target.type.get_base(), 
// 					    Place_Name(target.name)));

// 		push_default(dependency_child); 
// 		/* The place of the [[A]]->A links is empty, meaning it will
// 		 * not be output in traces. */ 
// 	} 
// }

Execution::Proceed Single_Execution::execute(Execution *parent, const Link &link)
{
	if (job.started()) {
		assert(children.empty()); 
	}       

	Proceed proceed= Execution::execute_base(link, done); 
	if (proceed & (P_BIT_WAIT | P_BIT_LATER)) {
		return proceed; 
//		return proceed & ~P_BIT_FINISHED;
	}

	assert(children.empty()); 

	if (finished(link.avoid)) {
		return P_CONTINUE; 
	}

//	/* Rule does not have a command.  This includes the case of dynamic
//	 * executions, even though for dynamic executions the RULE variable
//	 * is set (to detect cycles). */ 

//	/* We cannot return here in the non-dynamic case, because we
//	 * must still check that the target files exist, even if they
//	 * don't have commands. */ 
//	if (get_depth() != 0) {
//		done_add_neg(link.avoid); 
//		return proceed;
//	}

	/* Job has already been started */ 
	if (job.started_or_waited()) {
		return proceed | P_BIT_WAIT;
	}

	/* The file must be built */ 

//	assert(jobs > 0); 
	assert(! targets.empty());
	assert(targets.front().type.get_depth() == 0); 
	assert(targets.back().type.get_depth() == 0); 
	assert(get_buffer_default().empty()); 
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

	if (! checked) {
		checked= true; 

		exists= +1; 
		/* Now, set EXISTS to -1 when a file is found not to exist */ 

		for (unsigned i= 0;  i < targets.size();  ++i) {
			const Target &target= targets[i]; 

			if (target.type != Type::FILE) 
				continue;

			/* We save the return value of stat() and handle errors later */ 
			struct stat buf;
			int ret_stat= stat(target.name.c_str(), &buf);

			/* Warn when file has timestamp in the future */ 
			if (ret_stat == 0) { 
				/* File exists */ 
				Timestamp timestamp_file= Timestamp(&buf); 
				timestamps_old[i]= timestamp_file;
				// TODO this is the only place execute()
				// accesses PARENT.  Why is this needed? 
 				if (parent == nullptr || ! (link.flags & F_PERSISTENT)) 
					warn_future_file(&buf, 
							 target.name.c_str(), 
							 rule == nullptr 
							 ? parents.begin()->second.place
							 : rule->place_param_targets[i]->place); 
				/* EXISTS is not changed */ 
			} else {
				exists= -1;
			}

			if (! need_build && ret_stat == 0 &&
			    timestamp.defined() && timestamps_old[i] < timestamp &&
			    ! no_execution) {
				need_build= true;
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
			
			if (! need_build && ret_stat != 0 && errno == ENOENT) {
				/* File does not exist */

				if (! (link.flags & F_OPTIONAL)) {
					/* Non-optional dependency */  
					need_build= true; 
				} else {
					/* Optional dependency:  don't create the file;
					 * it will then not exist when the parent is
					 * called. */ 
					done_add_one_neg(F_OPTIONAL); 
					return proceed;
				}
			}

			if (ret_stat != 0 && errno != ENOENT) {
				/* stat() returned an actual error,
				 * e.g. permission denied:  build error */
				rule->place_param_targets[i]->place
					<< system_format(target.format_word()); 
				raise(ERROR_BUILD);
				done_add_one_neg(link.avoid); 
				return proceed;
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
				done_add_one_neg(link.avoid); 
				raise(ERROR_BUILD);
				return proceed;
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

	if (! need_build) {
		bool has_file= false; /* One of the targets is a file */
		for (const Target &target:  targets) {
			if (target.type == Type::FILE) {
				has_file= true; 
			}
		}
		for (const Target &target:  targets) {
			if (target.type != Type::TRANSIENT) 
				continue; 
			if (transients.count(target.name) == 0) {
				/* Transient was not yet executed */ 
				if (! no_execution && ! has_file) {
					need_build= true; 
				}
				break;
			}
		}
	}

	if (! need_build) {
		/* The file does not have to be built */ 
		done_add_neg(link.avoid); 
		return proceed;
	}

	/*
	 * The command must be run now, or there is no command. 
	 */

	/* Re-deploy all dependencies (second pass) */
	Proceed proceed_2= Execution::execute_second_pass(link); 
	if (proceed_2 & P_BIT_WAIT)
		return proceed_2; 

	if (no_execution) {
		/* A target without a command */ 
		done_add_neg(link.avoid); 
		return proceed;
	}

	/* The command must be run or the file created now */

	if (option_question) {
		print_error_silenceable("Targets are not up to date");
		exit(ERROR_BUILD);
	}

	out_message_done= true;
	
	print_command();

	if (rule->is_hardcode) {
		assert(targets.size() == 1);
		assert(targets.front().type == Type::FILE); 
		
		done_add_one_neg(0); 

		if (option_debug) {
			string text_target= debug_text();
			fprintf(stderr, "DEBUG %s %s create content\n",
				Verbose::padding(),
				text_target.c_str());
		}

		write_content(targets.front().name.c_str(), *(rule->command)); 

		assert(proceed == P_CONTINUE); 
		return proceed;
	}

	/* We know that a job has to be started now */

	assert(jobs >= 0); 
	if (jobs == 0) 
		return proceed | P_BIT_WAIT; 
       
	/* We have to start the job now */ 

	for (const Target &target:  targets) {
		if (target.type != Type::TRANSIENT)  
			continue; 
		Timestamp timestamp_now= Timestamp::now(); 
		assert(timestamp_now.defined()); 
		assert(transients.count(target.name) == 0); 
		transients[target.name]= timestamp_now; 
	}

	if (rule->redirect_index >= 0)
		assert(rule->place_param_targets[rule->redirect_index]->type == Type::FILE); 

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
			assert(rule->place_param_targets.front()->type == Type::FILE); 

			string source= rule->filename.unparametrized();
			
			/* If optional copy, don't just call 'cp' and
			 * let it fail:  look up whether the source
			 * exists in the cache */
			if (rule->dependencies.at(0)->get_flags() & F_OPTIONAL) {
				Execution *execution_source_base=
					executions_by_target.at(Target(Type::FILE, source));
				assert(execution_source_base); 
				Single_Execution *execution_source
					= dynamic_cast <Single_Execution *> (execution_source_base); 
				assert(execution_source); 
				if (execution_source->exists < 0) {
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
					done_add_neg(link.avoid); 
					assert(proceed == P_CONTINUE); 
					return proceed;
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

		if (option_debug) {
			string text_target= debug_text();
			fprintf(stderr, "DEBUG %s %s execute pid = %ld\n", 
				Verbose::padding(),
				text_target.c_str(),
				(long) pid); 
		}

		if (pid < 0) {
			/* Starting the job failed */ 
			print_traces(fmt("error executing command for %s", 
					 targets.front().format_word())); 
			raise(ERROR_BUILD);
			done_add_neg(link.avoid); 
			assert(proceed == P_CONTINUE); 
			return proceed;
		}

		executions_by_pid[pid]= this;
	}

	assert(executions_by_pid.at(pid)->job.started()); 
	assert(pid == executions_by_pid.at(pid)->job.get_pid()); 
	--jobs;
	assert(jobs >= 0);

	Proceed p= P_BIT_WAIT;
	if (order == Order::RANDOM && jobs > 0)
		p |= P_BIT_LATER; 

	return p;
}

void Single_Execution::print_as_job() const
{
	pid_t pid= job.get_pid();

	string text_target= targets.front().format_src(); 

	printf("%7ld %s\n", (long) pid, text_target.c_str());
}

void Single_Execution::write_content(const char *filename, 
				     const Command &command)
{
	FILE *file= fopen(filename, "w"); 

	if (file == nullptr) {
		rule->place <<
			system_format(name_format_word(filename)); 
		raise(ERROR_BUILD); 
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

	exists= +1;
}

void Single_Execution::propagate_variable(shared_ptr <Dependency> dependency,
					  Execution *parent)
{
	assert(dynamic_pointer_cast <Single_Dependency> (dependency)); 

	if (exists <= 0)
		return;

	Target target= dependency->get_individual_target().unparametrized(); 
	assert(target.type == Type::FILE);

	size_t filesize;
	struct stat buf;
	string dependency_variable_name;
	string content; 
	
	int fd= open(target.name.c_str(), O_RDONLY);
	if (fd < 0) {
		if (errno != ENOENT) {
			dependency->get_place() <<
				target.format_word();
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
		dynamic_pointer_cast <Single_Dependency> (dependency)->name; 

	{
	string variable_name= 
		dependency_variable_name == "" ?
		target.name : dependency_variable_name;
	
	dynamic_cast <Single_Execution *> (parent)
		->mapping_variable[variable_name]= content;
	}

	return;

 error_fd:
	close(fd); 
 error:
	Target target_variable= 
		dynamic_pointer_cast <Single_Dependency> (dependency)->place_param_target
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

	/* Note:  we don't have to propagate the error via the return
	 * value, because we already raised the error, i.e., we either
	 * threw an error, or set the error, which will be picked up by
	 * the parent.  */
	return;
}

bool Single_Execution::is_dynamic() const
{
	assert(targets.size() != 0); 
	return targets.front().type.is_dynamic(); 
}

void Single_Execution::check_execution(const Link &link) const
{
	for (const Target &target:  targets) {
		assert(target.type.get_depth() == done.get_depth());
	}

	Stack tmp_stack= link.avoid; 
	(void) tmp_stack; 
	assert(done.get_depth() == link.avoid.get_depth()); 

	if (targets.size() && targets.front().type == 0) {
		assert(link.avoid.get_lowest() == (link.flags & ((1 << C_TRANSITIVE) - 1))); 
	}
	done.check();
}

bool Single_Execution::optional_finished(const Link &link)
{
	if ((link.flags & F_OPTIONAL) 
	    && link.dependency != nullptr
	    && dynamic_pointer_cast <Single_Dependency> (link.dependency)
	    && dynamic_pointer_cast <Single_Dependency> (link.dependency)
	    ->place_param_target.type == Type::FILE) {

		const char *name= dynamic_pointer_cast <Single_Dependency> (link.dependency)
			->place_param_target.place_name.unparametrized().c_str();

		struct stat buf;
		int ret_stat= stat(name, &buf);
		if (ret_stat < 0) {
			exists= -1;
			if (errno != ENOENT) {
				dynamic_pointer_cast <Single_Dependency> (link.dependency)
					->place_param_target.place <<
					system_format(name_format_word(name)); 
				raise(ERROR_BUILD);
				done_add_neg(link.avoid); 
				return true;
			}
			done_add_highest_neg(link.avoid.get_highest()); 
			return true;
		} else {
			assert(ret_stat == 0);
			exists= +1;
		}
	}

	return false; 
}

bool Root_Execution::finished() const
{
	assert(done.get_depth() == 0);

	return ((~ done.get(0)) & ((1 << C_TRANSITIVE) - 1)) == 0; 
}

bool Root_Execution::finished(Stack avoid) const
{
	assert(done.get_depth() == 0);

	return ((~ done.get(0) & ~avoid.get(0)) & ((1 << C_TRANSITIVE) - 1)) == 0; 
}

Root_Execution::Root_Execution(const vector <shared_ptr <Dependency> > &dependencies)
	:  Execution(nullptr),
	   done(0, 0)
{
	for (auto &d:  dependencies) {
		push_default(d); 
	}
}

Execution::Proceed Root_Execution::execute(Execution *, const Link &link)
{
	Proceed proceed= Execution::execute_base(link, done); 
	if (proceed & (P_BIT_WAIT | P_BIT_LATER)) {
		return proceed;
	}

	done.add_neg(link.avoid); 

	return proceed; 
}

void Concatenated_Execution::next_list(vector <shared_ptr <Dependency> > &list) const
{
	assert(false); // TODO
}

Concatenated_Execution::Concatenated_Execution(shared_ptr <Dependency> dependency_,
					       Link &link,
					       Execution *parent)
	:  Execution(link, parent),
	   dependency(dependency_),
	   stage(0)
{
	assert(dependency_->is_normalized()); 

	/* Check the structure of the dependency */
	shared_ptr <Dependency> dep= dependency;
	dep= Dependency::strip_dynamic(dep); 
	assert(dynamic_pointer_cast <Concatenated_Dependency> (dep));
	shared_ptr <Concatenated_Dependency> concatenated_dependency= 
		dynamic_pointer_cast <Concatenated_Dependency> (dep);

	for (size_t i= 0;  i < concatenated_dependency->get_dependencies().size();  ++i) {
//	for (shared_ptr <Dependency> d:  concatenated_dependency->get_dependencies()) {

		shared_ptr <Dependency> d= concatenated_dependency->get_dependencies()[i]; 

		shared_ptr <Dependency> dependency_normalized= 
			Dependency::make_normalized_compound(d); 
//	/		d->normalize_compound(); 

		concatenated_dependency->get_dependencies()[i]= dependency_normalized; 

//		assert(dependencies_new.size() > 0); 

//		if (dependencies_new.size() == 1) {
//			concatenated_dependency->get_dependencies()[i]= dependencies_new[0]; 
//		} else {
//			shared_ptr <Compound_Dependency> cd= make_shared <Compound_Dependency> (...);
//			swap(cd->get_dependencies(), dependencies_new); 
//			concatenated_dependency->get_dependencies()[i]= cd; 
//		}

//		if (dynamic_pointer_cast <Compound_Dependency> (d)) {
//			for (shared_ptr <Dependency> dd:  
//				     dynamic_pointer_cast <Compound_Dependency> (d)->get_dependencies()) {
//				shared_ptr <Dependency> dddd= Dependency::strip_dynamic(dd);
//				assert(dynamic_pointer_cast <Single_Dependency> (dddd)); 
//			}
//		} else {
//			shared_ptr <Dependency> ddd= Dependency::strip_dynamic(d);
//			assert(dynamic_pointer_cast <Single_Dependency> (ddd)); 
//		}
	}
}

Execution::Proceed Concatenated_Execution::execute(Execution *, 
						   const Link &link)
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

		shared_ptr <Dependency> dep= Dependency::strip_dynamic(dependency); 
		shared_ptr <Concatenated_Dependency> concatenated_dependency= 
			dynamic_pointer_cast <Concatenated_Dependency> (dep);
		assert(concatenated_dependency != nullptr); 

		const size_t n= concatenated_dependency->get_dependencies().size(); 

		for (size_t i= 0;  i < n;  ++i) {
//		for (shared_ptr <Dependency> d:  concatenated_dependency->get_dependencies()) {
			shared_ptr <Dependency> d= concatenated_dependency->get_dependencies()[i]; 
			if (dynamic_pointer_cast <Compound_Dependency> (d)) {
				for (shared_ptr <Dependency> dd:  
					     dynamic_pointer_cast <Compound_Dependency> (d)->get_dependencies()) {
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
		Stack done_here; 
		Proceed proceed= Execution::execute_base(link, done_here); 
		if (proceed & P_BIT_WAIT) {
			return proceed;
		}

//		vector <shared_ptr <Dependency> > dependencies_read; 

		// TODO we can't pass DEPENDENCY here, in the case of
		// multiply-nested concatenations with dynamics. 
		// Use a new dependency flag akin to READ. 
//		read_concatenation(link.avoid, dependency, dependencies_read); 

//		for (auto &i:  dependencies_read) {
//			push_default(i); 
//		}

//		dependency= nullptr; 

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
		Stack done_here;
		Proceed proceed= Execution::execute_base(link, done_here); 
		if (proceed & P_BIT_WAIT) {
			return proceed;
		}

		assert((proceed & P_BIT_WAIT) == 0); 

		stage= 3; 

		/* No need to set a DONE variable for concatenated
		 * executions -- whether we are done is indicated by
		 * STAGE == 3.  */
//		done.add_neg(link.avoid); 
		
		return proceed; 

	} else if (stage == 3) {
		return P_CONTINUE; 
	} else {
		assert(false);  /* Invalid stage */ 
		return P_CONTINUE; 
	}
}

bool Concatenated_Execution::finished() const
{
	/* We ignore DONE here */
	return stage == 3;
}

bool Concatenated_Execution::finished(Stack avoid) const
/* Since Concatenated_Execution objects are used just once, by a single
 * parent, this always returns the same as finished() itself.
 * Therefore, the AVOID parameter is ignored.  */
{
	(void) avoid;
	return finished(); 
}

void Concatenated_Execution::add_stage0_dependency(shared_ptr <Dependency> d,
						   unsigned concatenate_index)
/* 
 * The given dependency can be non-normalized. 
 */
{
	// XXX analogous behaviour to Dynamic_Execution
	assert(false);
	(void) d;  (void) concatenate_index;  // RM

// 	if (dynamic_pointer_cast <Single_Dependency> (d)) {
// 		/* We don't have to add the dependency to anything,
// 		 * because it is not dynamic.  This corresponds to the
// 		 * case of e.g.     
// 		 *
// 		 *                    list.(a b c)
// 		 *
// 		 * in which nothing is dynamic:  There is nothing to
// 		 * do in stage 1.  */

// //		add_part(d, concatenate_index); 

// 	} else if (dynamic_pointer_cast <Dynamic_Dependency> (d)) {
// 		shared_ptr <Dynamic_Dependency> dynamic_dependency=
// 			dynamic_pointer_cast <Dynamic_Dependency> (d); 
// //		shared_ptr <Dependency> dependency_inner=
// //			dynamic_dependency->dependency;

// 		shared_ptr <Dependency> dependency_bits
// 			= dynamic_dependency->clone_shallow(); 
// 		if (concatenate_index > C_CONCATENATE_MAX) {
// 			d->get_place() << "Number of elements in concatenation exceeds limits"; 
// 			raise(ERROR_LOGICAL); 
// 			return; 
// 		}
// 		dependency_bits->flags |= concatenate_index << C_CONCATENATE_BASE; 
// 		dependency_bits->flags |= F_CONCATENATE; 

// 		push_default(dependency_bits); 

// 	} else {
// 		/* Not implemented */
// 		// not needed
// 		// TODO implement 
// 		assert(false);
// 	}
}

// void Concatenated_Execution::read_concatenation(Stack avoid,
// //						shared_ptr <Dependency> dependency_x,
// 						vector <shared_ptr <Dependency> > &dependencies_read)
// {
// 	assert(dependencies_read.empty()); 

// 	/* First, handle the outermost dynamic dependencies */
// 	if (dynamic_pointer_cast <Dynamic_Dependency> (dependency_x)) {
// 		shared_ptr <Dynamic_Dependency> dependency_dynamic= 
// 			dynamic_pointer_cast <Dynamic_Dependency> (dependency_x);
// 		read_concatenation(avoid, dependency_dynamic->dependency, dependencies_read);
// 		for (size_t i= 0;  i < dependencies_read.size();  ++i) {
// 			dependencies_read[i]= 
// 				make_shared <Dynamic_Dependency> 
// 				(dependency_x->get_flags(),
// 				 dependency_x->places,
// 				 dependencies_read[i]);
// 		}
// 		return; 
// 	}

// 	/* Now DEPENDENCY is a Concatenated_Dependency, containing each:
// 	 * Compound_Dependency^{0,1} of Dynamic_Dependency^* of a
// 	 * Single_Dependency.  */ 

// 	shared_ptr <Concatenated_Dependency> concatenated_dependency=
// 		dynamic_pointer_cast <Concatenated_Dependency> (dependency_x);
// 	assert(concatenated_dependency);

// 	/* An concatenation of zero components would logically be an
// 	 * empty product and could be argued to result in a single
// 	 * empty-string dependency.  
// 	 * Does not happen.  */
// 	if (concatenated_dependency->get_dependencies().size() == 0) {
// 		assert(false);
// 		return; 
// 	}

// 	for (size_t i= 0;  i < concatenated_dependency->get_dependencies().size();  ++i) {

// 		vector <shared_ptr <Dependency> > dependencies_read_new; 
		
// 		shared_ptr <Dependency> d= concatenated_dependency->get_dependencies()[i]; 

// 		/* D is a Compound_Dependency^{0,1} of
// 		 * Dynamic_Dependency^* of a Single_Dependency */ 

// 		/* If a single component is empty, the whole result is
// 		 * an empty set of dependencies.  */
// 		if (dynamic_pointer_cast <Compound_Dependency> (d) &&
// 		    dynamic_pointer_cast <Compound_Dependency> (d)->get_dependencies().size() == 0) {
// 			dependencies_read= vector <shared_ptr <Dependency> > (); 
// 			return; 
// 		}

// 		/* If D is not a Compound_Dependency, then it contains
// 		 * only a single component.  Append it to all read
// 		 * dependencies */
// 		if (nullptr == dynamic_pointer_cast <Compound_Dependency> (d)) {
// 			if (i == 0) {
// 				dependencies_read_new.push_back(d);
// 			} else {
// 				for (size_t j= 0;  j < dependencies_read.size();  ++j) {
// 					concatenate_dependency(avoid,
// 							       dependencies_read[j], d, 
// 							       0, dependencies_read_new); 
// 				}
// 			}
// 		} else {
		
// 			shared_ptr <Compound_Dependency> compound_dependency=
// 				dynamic_pointer_cast <Compound_Dependency> (d);
// 			assert(compound_dependency); 
		
// 			if (i == 0) {
// 				for (size_t k= 0;  k < compound_dependency->get_dependencies().size();  ++k) {
// 					dependencies_read_new.push_back(compound_dependency->get_dependencies()[k]);
// 					// TODO add flags d->get_flags
// 				}
// 			} else {
// 				for (size_t j= 0;  j < dependencies_read.size();  ++j) {
// 					for (size_t k= 0;  k < compound_dependency->get_dependencies().size();  ++k) {
// 						concatenate_dependency(avoid, dependencies_read[j], 
// 								       compound_dependency->get_dependencies()[k],
// 								       d->get_flags(), dependencies_read_new); 
// 					}
// 				}
// 			}
// 		}
		
// 		swap(dependencies_read, dependencies_read_new); 
// 	}
// }

// void Concatenated_Execution::concatenate_dependency(Stack avoid,
// 						    shared_ptr <Dependency> dependency_1,
// 						    shared_ptr <Dependency> dependency_2,
// 						    Flags dependency_flags,
// 						    vector <shared_ptr <Dependency> > &dependencies)
// {
// 	assert(dependency_1->is_normalized()); 
// 	assert(dependency_2->is_normalized()); 

// 	vector <shared_ptr <Dependency> > dependencies_1, dependencies_2; 

// 	/* Replace dynamic dependencies by actual dependencies, if
// 	 * necessary  */ 
// 	if (dynamic_pointer_cast <Dynamic_Dependency> (dependency_1)) {
// 		read_dynamic(avoid, 
// 			     dynamic_pointer_cast <Dynamic_Dependency> (dependency_1), 
// 			     dependencies_1); 
// 	} else {
// 		dependencies_1.push_back(dependency_1); 
// 	}

// 	if (dynamic_pointer_cast <Dynamic_Dependency> (dependency_2)) {
// 		read_dynamic(avoid, 
// 			     dynamic_pointer_cast <Dynamic_Dependency> (dependency_2), 
// 			     dependencies_2); 
// 	} else {
// 		dependencies_2.push_back(dependency_2); 
// 	}

// 	/* Concatenate */ 
// 	for (auto &i:  dependencies_1) {
// 		for (auto &j:  dependencies_2) {
// 			assert(dynamic_pointer_cast <Single_Dependency> (i)); 
// 			assert(dynamic_pointer_cast <Single_Dependency> (j)); 
// 			shared_ptr <Dependency> d= concatenate_dependency_one
// 				(dynamic_pointer_cast <Single_Dependency> (i), 
// 				 dynamic_pointer_cast <Single_Dependency> (j), 
// 				 dependency_flags);
// 			assert(d);
// 			dependencies.push_back(d); 
// 		}
// 	}
// }

shared_ptr <Dependency> Concatenated_Execution::
concatenate_dependency_one(shared_ptr <Single_Dependency> dependency_1,
			   shared_ptr <Single_Dependency> dependency_2,
			   Flags dependency_flags)
/* 
 * Rules for concatenation:
 *   - Flags are not allowed on the second component.
 *   - The second component must not be transient. 
 */
{
//	assert(dynamic_pointer_cast <Single_Dependency> (Dependency::strip_dynamic(dependency_1))); 
//	assert(dynamic_pointer_cast <Single_Dependency> (Dependency::strip_dynamic(dependency_2))); 

	assert(dependency_2->get_flags() == 0);
	// TODO test:  replace by a proper error 

	assert(dependency_2->place_param_target.type == Type::FILE); 
	// TODO proper test.  

	Place_Param_Target target= dependency_1->place_param_target; 
	target.place_name.append(dependency_2->place_param_target.place_name); 

//	...; // prepend the dynamic outer layer 

	return make_shared <Single_Dependency> 
		(dependency_flags & dependency_1->get_flags(),
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

	vector <shared_ptr <Dependency> > dependencies_read; 

	for (size_t i= 0;  i < parts.size();  ++i) {

		vector <shared_ptr <Dependency> > dependencies_read_new; 
		
//		shared_ptr <Dependency> d= concatenated_dependency->get_dependencies()[i]; 

		/* If a single index is empty, the whole result is
		 * an empty set of dependencies.  */
		if (parts[i].empty()) {
//		if (dynamic_pointer_cast <Compound_Dependency> (d) &&
//		    dynamic_pointer_cast <Compound_Dependency> (d)->get_dependencies().size() == 0) {
			dependencies_read= vector <shared_ptr <Dependency> > (); 
			return; 
		}

		if (i == 0) {
			/* The leftmost components are special:  we
			 * don't perform any checks on them, as they can
			 * be transient and have flags, while subsequent
			 * parts cannot.  */
			for (size_t k= 0;  k < parts[i].size();  ++k) {
				dependencies_read_new.push_back(parts[i][k]);
				// TODO add flags d->get_flags
			}
		} else {
			for (size_t j= 0;  j < dependencies_read.size();  ++j) {
				for (size_t k= 0;  k < parts[i].size();  ++k) {
					// dependencies_read_new.push_back
					// 	(concatenate_dependency_one(dependencies_read[j], 
					// 				    parts[i][k],
					// 				    0)); 
				}
			}
		}
		
		swap(dependencies_read, dependencies_read_new); 
	}

	for (auto &i:  dependencies_read) {
//		...; // add the outer layer 
		push_default(i); 
	}
}

// void Concatenated_Execution::add_part(shared_ptr <Dependency> dependency_part, 
// 				      int concatenation_index)
// {
// 	shared_ptr <Dependency> inner= Dependency::strip_dynamic(dependency_part);
// 	assert(dynamic_pointer_cast <Single_Dependency> (inner)); 
	
// 	parts[concatenation_index].push_back(dependency_part); 
// }

Dynamic_Execution::Dynamic_Execution(Link &link,
				     Execution *parent)
	:  Execution(link, parent),
	   dependency(dynamic_pointer_cast <Dynamic_Dependency> (link.dependency)),
	   done(
		dynamic_pointer_cast <Dynamic_Dependency> (link.dependency)->get_depth(),
//		target_.type.get_depth()
		0)
{
	assert(parent != nullptr); 
	assert(parents.size() == 1); 
	assert(dynamic_pointer_cast <Dynamic_Dependency> (link.dependency)); 
	assert(dependency); 
	
//	assert(target_.type.is_dynamic()); 

	/* Set the rule here, so cycles in the dependency graph can be
	 * detected.  Note however that the rule of dynamic executions
	 * is otherwise not used.  */ 

	shared_ptr <Dependency> inner_dependency= Dependency::strip_dynamic(dependency);

	if (dynamic_pointer_cast <Single_Dependency> (inner_dependency)) {
		shared_ptr <Single_Dependency> inner_single_dependency
			= dynamic_pointer_cast <Single_Dependency> (inner_dependency); 
//		Target target_base(inner_dependency.type.get_base(), inner_dependency.target_.name);
		Target target_base(inner_single_dependency->place_param_target.type,
				   inner_single_dependency->place_param_target.place_name.unparametrized());
		Target target= dependency->get_individual_target().unparametrized(); 
		try {
			map <string, string> mapping_parameter; 
			shared_ptr <Rule> rule= 
				rule_set.get(target_base, param_rule, mapping_parameter, 
					     link.dependency->get_place()); 
		} catch (int e) {
			print_traces(); 
			raise(e); 
			return; 
		}

		executions_by_target[target]= this; 
	}

	/* Push left branch dependency */ 
	shared_ptr <Dependency> dependency_left=
		Dependency::clone_dependency(dependency->dependency);
	dependency_left->add_flags(F_DYNAMIC_LIST); 
	push_default(dependency_left); 
}

Execution::Proceed Dynamic_Execution::execute(Execution *, const Link &link)
{
	Proceed proceed= Execution::execute_base(link, done); 
	if (proceed & (P_BIT_WAIT | P_BIT_LATER)) {
		return proceed; 
	}

	done.add_neg(link.avoid); 

	return proceed; 
}

//void Dynamic_Execution::read_dynamic_dependency(Stack avoid,
//						shared_ptr <Dependency> dependency_this)
//{
//}

bool Dynamic_Execution::finished() const 
{
	assert(done.get_depth() == dependency->get_depth()); 
//	assert(done.get_depth() == targets.front().type.get_depth());
//	assert(done.get_depth() == targets.back().type.get_depth());

	Flags to_do_aggregate= 0;
	
	for (unsigned j= 0;  j <= done.get_depth();  ++j) {
		to_do_aggregate |= ~done.get(j); 
	}

	return (to_do_aggregate & ((1 << C_TRANSITIVE) - 1)) == 0; 
}

bool Dynamic_Execution::finished(Stack avoid) const
{
	assert(done.get_depth() == dependency->get_depth()); 
//	assert(done.get_depth() == targets.front().type.get_depth());
//	assert(done.get_depth() == targets.back().type.get_depth());

	assert(avoid.get_depth() == done.get_depth());

	Flags to_do_aggregate= 0;
	
	for (unsigned j= 0;  j <= done.get_depth();  ++j) {
		to_do_aggregate |= ~done.get(j) & ~avoid.get(j); 
	}

	return (to_do_aggregate & ((1 << C_TRANSITIVE) - 1)) == 0; 
}

bool Dynamic_Execution::want_delete() const
{
	return dynamic_pointer_cast <Single_Dependency> (Dependency::strip_dynamic(dependency)) == nullptr; 
}

void Dynamic_Execution::propagate_to_dynamic(Execution *child,
					     Flags flags_child,
					     Stack avoid_parent,
					     shared_ptr <Dependency> dependency_this,
					     shared_ptr <Dependency> dependency_child)
/* A left branch child is done */
{
	(void) child; 
	
	assert(flags_child & F_DYNAMIC_LIST); 
//	assert(dynamic_pointer_cast <Single_Dependency> (dependency_child)
//	       && dynamic_pointer_cast <Single_Dependency> (dependency_child)
//	       ->place_param_target.type == Type::FILE);
	assert(dependency_this->get_individual_target().type.is_dynamic());

	/* Even if the child produced an error, we still want to read
	 * its partially assembled list of filenames.  */

	/* 
	 * There are two cases:
	 *  - This is a singly dynamic file:  read out the file here. 
	 *  - Any other dynamic:  start, as right branch, the list of
	 *    the child with our own outer dynamic layer added. 
	 */

	try {
		shared_ptr <Single_Dependency> single_dependency_child=
		dynamic_pointer_cast <Single_Dependency> (dependency_child);

		if (single_dependency_child &&
		    single_dependency_child->place_param_target.type == Type::FILE) {

			/* This is a dynamic file dependency; read out
			 * the file.  This is not an optimization of a
			 * common case, but necessary because otherwise
			 * we would start, as a right branch, the same
			 * execution as ourselves.  */ 

			const Target target= dependency_this->get_individual_target().unparametrized(); 
			assert(dynamic_pointer_cast <Dynamic_Dependency> (dependency_this)); 

			vector <shared_ptr <Dependency> > dependencies;

			read_dynamic(avoid_parent, 
				     dynamic_pointer_cast <Dynamic_Dependency> (dependency_this), 
				     dependencies);

			for (auto &j:  dependencies) {

				/* Add the dependency, with one less dynamic level
				 * than the current target  */

				shared_ptr <Dependency> dd(j);

				vector <shared_ptr <Dynamic_Dependency> > vec;
				shared_ptr <Dependency> p= dependency_this;

				while (dynamic_pointer_cast <Dynamic_Dependency> (p)) {
					shared_ptr <Dynamic_Dependency> dynamic_dependency= 
						dynamic_pointer_cast <Dynamic_Dependency> (p);
					vec.resize(vec.size() + 1);
					vec[vec.size() - 1]= dynamic_dependency;
					p= dynamic_dependency->dependency;   
				}

				Stack avoid_this= avoid_parent;
				assert(vec.size() == avoid_this.get_depth());
				avoid_this.pop(); 
				dd->add_flags(avoid_this.get_lowest()); 
				if (dd->get_place_flag(I_PERSISTENT).empty())
					dd->set_place_flag
						(I_PERSISTENT,
						 vec[target.type.get_depth() - 1]
						 ->get_place_flag(I_PERSISTENT)); 
				if (dd->get_place_flag(I_OPTIONAL).empty())
					dd->set_place_flag
						(I_OPTIONAL,
						 vec[target.type.get_depth() - 1]
						 ->get_place_flag(I_OPTIONAL)); 
				if (dd->get_place_flag(I_TRIVIAL).empty())
					dd->set_place_flag
						(I_TRIVIAL,
						 vec[target.type.get_depth() - 1]
						 ->get_place_flag(I_TRIVIAL)); 

				for (Type k= target.type - 1;  k.is_dynamic();  --k) {
					avoid_this.pop(); 
					Flags flags_level= avoid_this.get_lowest(); 
					dd= make_shared <Dynamic_Dependency> 
						(flags_level, dd); 
					dd->set_place_flag
						(I_PERSISTENT,
						 vec[k.get_depth() - 1]->get_place_flag(I_PERSISTENT)); 
					dd->set_place_flag
						(I_OPTIONAL,
						 vec[k.get_depth() - 1]->get_place_flag(I_OPTIONAL)); 
					dd->set_place_flag
						(I_TRIVIAL,
						 vec[k.get_depth() - 1]->get_place_flag(I_TRIVIAL)); 
				}

				assert(avoid_this.get_depth() == 0); 

				push_default(dd); 
			}
				
		} else {
			/* This is another type of dynamic dependency:
			 * Add, as a right branch, each dependency from
			 * the child's list, with our own outer dynamic
			 * dependency added.  */

			shared_ptr <Dynamic_Dependency> dynamic_dependency_this=
				dynamic_pointer_cast <Dynamic_Dependency> (dependency_this); 

			for (auto &i:  child->get_list()) {
				dependency_right= make_shared <Dynamic_Dependency> 
					(dependency_this->flags,
					 dependency_this->places,
					 i); 
				push_default(dependency_right); 
			}
		}

	} catch (int e) {
		/* We catch not only the errors raised in this function,
		 * but also the errors raised in read_dynamic().  */
		raise(e); 
	}
}

#endif /* ! EXECUTION_HH */
