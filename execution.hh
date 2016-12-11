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
 * Each target is represented at run time by one Execution object.  
 *
 * The set of active Execution objects forms a directed acyclic graph,
 * rooted at the root Execution object.  Edges in this graph are
 * represented by Link objects.  Each Execution object corresponds to
 * one or more unique Target objects.  Two Execution objects are
 * connected if there is a dependency between them.  If there is an edge
 * A ---> B, A is said to be the parent of B, and B the child of A.
 * Also, B is a dependency of A.  If A is a dynamic target, then it has
 * as an initial child only the corresponding target with one less
 * dynamicity level; other dependencies are added later.
 *
 * All Execution objects are allocated with new Execution(...), and are
 * never deleted, as the information contained in them needs to be
 * cached.
 *
 * All Execution objects are linked through the map called
 * "executions_by_target" by all their targets.
 */
{
public:  
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

private:
	friend void job_terminate_all(); 
	friend void job_print_jobs(); 

	vector <Target> targets; 
	/* The targets to which this execution object corresponds.
	 * Empty only for the root target.  Otherwise, all entries have
	 * the same depth.  If the dynamic depth is larger than one,
	 * then there is exactly one target.  There are multiple targets
	 * here when the rule had multiple targets.  */   

	shared_ptr <Rule> rule;
	/* The instantiated file rule for this execution.  Null when
	 * there is no rule for this file (this happens for instance
	 * when a source code file is given as a dependency, or when
	 * this is a complex dependency).  Individual dynamic
	 * dependencies do have rules, in order for cycles to be
	 * detected.  */ 

	shared_ptr <Rule> param_rule;
	/* The rule from which this execution was derived.  This is only
	 * used to detect strong cycles.  To manage the dependencies,
	 * the instantiated general rule is used.  Null if and only if
	 * RULE is null.  */ 

	set <Execution *> children;
	/* Currently running executions.  Allocated with operator new()
	 * and never deleted.  */ 

	map <Execution *, Link> parents; 
	/* The parent executions.  This is a map because typically, the
	 * number of elements is always very small, i.e., mostly one,
	 * and a map is better suited in this case.  */ 

	Job job;
	/* The job used to execute this rule's command */ 

	Buffer buffer_default;
	/* Dependencies that have not yet begun to be built.
	 * Initialized with all dependencies, and emptied over time when
	 * things are built, and filled over time when dynamic
	 * dependencies are worked on.  Entries are not necessarily
	 * unique.  Does not contain compound dependencies.  */ 

	Buffer buffer_trivial; 
	/* The buffer for dependencies in the second pass.  They are
	 * only started if, after (potentially) starting all non-trivial
	 * dependencies, the target must be rebuilt anyway.  Does not
	 * contain compound dependencies.  */

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

	int error;
	/* Error value of this target.  The value is propagated (using '|')
	 * to the parent.  Values correspond to constants defined in
	 * error.hh; zero denotes the absence of an error.  */ 

	Stack done;
	/* What parts of this target have been done. Each bit represents
	 * one aspect that was done.  The depth K is equal to the dynamicity
	 * for dynamic targets, and to zero for non-dynamic targets.  */

	Timestamp timestamp; 
	/* Latest timestamp of a (direct or indirect) file dependency
	 * that was not rebuilt.  Files that were rebuilt are not
	 * considered, since they make the target be rebuilt anyway.
	 * The function execute() also changes this to consider the file
	 * itself.  This final timestamp is then carried over to the
	 * parent executions.  */

	bool need_build;
	/* Whether this target needs to be built.  When a target is
	 * finished, this value is propagated to the parent executions
	 * (except when the F_PERSISTENT flag is set).  */ 

	bool checked;
	/* Whether we performed the check in execute()  */ 

	signed char exists;
	/* 
	 * Whether the file targets are known to exist.  
	 *     -1 = at least one file target is known not to exist (only
	 *     	    possible when there is at least one file target)
	 *      0 = status unknown
	 *     +1 = all file targets are known to exist (possible when
	 *          there are no file targets)
	 * When there are no file targets, the value may be both 0 or
	 * +1.  
	 */
	
	Execution(Target target_,
		  Link &link,
		  Execution *parent);
	/* File, transient and dynamic targets (everything except the
	 * root target)  */ 

	Execution(const vector <shared_ptr <Dependency> > &dependencies_); 
	/* Root execution; DEPENDENCIES don't have to be unique */

	~Execution(); 
	/* There is no implementation of this, as it is not used.  This
	 * serves as a link-time check that Execution objects are not
	 * destroyed.  */ 

	bool finished(Stack avoid) const; 
	/* Whether the execution is finished working for the given tasks */ 

	bool finished() const;
	/* Whether the execution is completely finished */ 

	void read_dynamic_dependency(Stack avoid, shared_ptr <Dependency> dependency_this);
	/* Read dynamic dependencies from a file.  Only called for
	 * dynamic targets.  Called for the parent of a dynamic--file
	 * link.  */ 

	bool remove_if_existing(bool output); 
	/* Remove all file targets of this execution object if they
	 * exist.  If OUTPUT is true, output a corresponding message.
	 * Return whether the file was removed.  If OUTPUT is false,
	 * only do async signal-safe things.  */  

 	bool execute(Execution *parent, Link &&link);
	/* Start the next job(s). This will also terminate jobs when
	 * they don't need to be run anymore, and thus it can be called
	 * when K = 0 just to terminate jobs that need to be terminated.
	 * The passed LINK.FLAG is the ORed combination of all FLAGs up
	 * the dependency chain.  Return value: whether additional
	 * processes must be started.  Can only by TRUE in random mode.
	 * When TRUE, not all possible child jobs where started.  */

	int execute_children(const Link &link);
	/* Execute already-active children.  Parameters 
	 * are equivalent to those of execute(). Return value:
	 * -1: continue; 0: return false; 1: return true.  */ 

	void waited(pid_t pid, int status); 
	/* Called after the job was waited for.  The PID is only passed
	 * for checking that it is correct.  */

	void print_traces(string text= "") const;
	/* Print full trace for the execution.  First the message is
	 * printed, then all traces for it starting at this execution,
	 * up to the root execution. 
	 * TEXT may be "" to not print any additional message.  */ 

	void warn_future_file(struct stat *buf, 
			      const char *filename,
			      const Place &place,
			      const char *message_extra= nullptr);
	/* Warn when the file has a modification time in the future.
	 * MESSAGE_EXTRA may be null to not show an extra message.  */ 

	bool deploy(const Link &link,
		    shared_ptr <Dependency> dependency_child);
	/* Deploy a new child execution.  LINK is the link from the
	 * THIS's parent to this.  Note: the top-level flags of
	 * LINK.DEPENDENCY may be modified.  Return value: same
	 * semantics as for execute().  */

	void initialize(Stack avoid);
	/* Initialize the Execution object.  Used for dynamic dependencies.
	 * Called from get_execution() before the object is connected to a
	 * new parent.  */ 

	void print_command() const; 
	/* Print the command and its associated variable assignments,
	 * according to the selected verbosity level.  */

	void print_as_job() const;
	/* Print a line to stdout for a running job, as output of SIGUSR1.
	 * Is currently running.  */ 

	void raise(int error_);
	/* All errors by Execution call this function.  Set the error
	 * code, and throw an error except with the keep-going option.  */

	void write_content(const char *filename, const Command &command); 
	/* Create the file FILENAME with content from COMMAND */

	bool read_variable(string &variable_name,
			   string &content,
			   shared_ptr <Dependency> dependency); 
	/* Read the content of the file into a string as the
	 * variable value.  THIS is the variable target.  Return TRUE on
	 * success.  */  

	bool is_dynamic() const;

	int get_dynamic_depth() const 
	/* The dynamic depth, or -1 for the root execution. */ 
	{
		return targets.empty()
			? -1
			: targets.front().type.get_depth(); 
	}

	void push_default(shared_ptr <Dependency> );
	/* Push a link to the default buffer, breaking down compound
	 * dependencies while doing so.  */

	string verbose_target() const {
		return targets.empty() 
			? "ROOT"
			: targets.front().format_out(); 
	}

	static unordered_map <Target, Execution *> executions_by_target;
	/* The Execution objects by each of their target.  Execution objects
	 * are never deleted.  This serves as a caching mechanism.  The
	 * root Execution has no targets and therefore is not included.
	 * Non-dynamic execution objects are shared by the multiple
	 * targets of a multi-target rule.  A dynamic multi-target rule
	 * result in multiple non-shared execution objects.  */

	static unordered_map <pid_t, Execution *> executions_by_pid;
	/* The currently running executions by process IDs */ 

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

	static Timestamp timestamp_last; 
	/* The timepoint of the last time wait() returned.  No file in the
	 * file system should be newer than this.  */ 

	static bool hide_out_message;
	/* Whether to show a STDOUT message at the end */

	static bool out_message_done;
	/* Whether the STDOUT message is not "Targets are up to date" */

	static void unlink(Execution *const parent, 
			   Execution *const child,
			   shared_ptr <Dependency> dependency_parent,
			   Stack avoid_parent,
			   shared_ptr <Dependency> dependency_child,
			   Stack avoid_child,
			   Flags flags_child); 
	/* Propagate information from the subexecution to the execution, and
	 * then delete the child execution.  The child execution is
	 * however not deleted as it is kept for caching.  */

	static Execution *get_execution(const Target &target, 
					Link &link,
					Execution *parent); 
	/* Get an existing execution or create a new one.
	 * Return NULLPTR when a strong cycle was found; return the execution
	 * otherwise.  PLACE is the place of where the dependency was
	 * declared.  LINK is the link from the existing parent to the
	 * new execution.  */ 

	static void wait();
	/* Wait for next job to finish and finish it.  Do not start anything
	 * new.  */ 

	static bool find_cycle(const Execution *const parent,
			       const Execution *const child,
			       const Link &link);
	/* Find a cycle.  Assuming that the edge parent->child, find a
	 * directed cycle that would be created.  Start at PARENT and
	 * perform a depth-first search upwards in the hierarchy to find
	 * CHILD.  LINK is the LINK that would be added between child
	 * and parent, and would create a cycle.  */

	static bool find_cycle(vector <const Execution *> &path,
				const Execution *const child,
				const Link &link); 

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
};

unordered_map <pid_t, Execution *> Execution::executions_by_pid;
unordered_map <Target, Execution *> Execution::executions_by_target;
unordered_map <string, Timestamp> Execution::transients;
Timestamp Execution::timestamp_last;
Rule_Set Execution::rule_set; 
long Execution::jobs= 1;
bool Execution::hide_out_message= false;
bool Execution::out_message_done= false;

void Execution::wait() 
{
	if (option_debug) {
		fprintf(stderr, "DEBUG %s wait\n",
			Verbose::padding()); 
	}

	assert(Execution::executions_by_pid.size()); 

	int status;
	pid_t pid= Job::wait(&status); 

	if (option_debug) {
		fprintf(stderr, "DEBUG %s wait pid = %ld\n", 
			Verbose::padding(),
			(long) pid);
	}

	timestamp_last= Timestamp::now(); 

	if (executions_by_pid.count(pid) == 0) {
		assert(false);
		return; 
	}

	Execution *const execution= executions_by_pid.at(pid); 

	execution->waited(pid, status); 

	++jobs; 
}

bool Execution::execute(Execution *parent, Link &&link)
{
	Verbose verbose;

	assert(jobs >= 0); 

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

	if (option_debug) {
		string text_target= verbose_target(); 
		string text_flags= flags_format(link.flags);
		string text_avoid= link.avoid.format(); 

		fprintf(stderr, "DEBUG %s %s execute %s %s\n", 
			Verbose::padding(),
			text_target.c_str(),
			text_flags.c_str(),
			text_avoid.c_str()); 
	}

	/* Override the trivial flag */ 
	if (link.flags & F_OVERRIDE_TRIVIAL) {
		link.flags &= ~F_TRIVIAL; 
		link.avoid.rem_highest(F_TRIVIAL); 
	}

 	if (finished(link.avoid)) {
		if (option_debug) {
			string text_target= verbose_target(); 
			fprintf(stderr, "DEBUG %s %s finished\n",
				Verbose::padding(),
				text_target.c_str());
		}
		return false;
	}

	/* In DFS mode, first continue the already-open children, then
	 * open new children.  In random mode, start new children first
	 * and continue already-open children second */ 

	/* 
	 * Continue the already-active child executions 
	 */  

	if (order != Order::RANDOM) {
		int ret= execute_children(link);
		if (ret >= 0)
			return ret;
	}

	/* Should children even be started?  Check whether this is an
	 * optional dependency and if it is, return when the file does not
	 * exist.  */
	if ((link.flags & F_OPTIONAL) 
	    && link.dependency != nullptr
	    && dynamic_pointer_cast <Direct_Dependency> (link.dependency)
	    && dynamic_pointer_cast <Direct_Dependency> (link.dependency)
	    ->place_param_target.type == Type::FILE) {

		const char *name= dynamic_pointer_cast <Direct_Dependency> (link.dependency)
			->place_param_target.place_name.unparametrized().c_str();

		struct stat buf;
		int ret_stat= stat(name, &buf);
		if (ret_stat < 0) {
			exists= -1;
			if (errno != ENOENT) {
				dynamic_pointer_cast <Direct_Dependency> (link.dependency)
					->place_param_target.place <<
					system_format(name_format_word(name)); 
				raise(ERROR_BUILD);
				done.add_neg(link.avoid); 
				return false;
			}
			done.add_highest_neg(link.avoid.get_highest()); 
			return false;
		} else {
			assert(ret_stat == 0);
			exists= +1;
		}
	}

	/* Is this a trivial dependency and we are not in trivial
	 * override mode?  Then skip the dependency. */
	if (link.flags & F_TRIVIAL) {
		done.add_neg(link.avoid);
		return false;
	}

	if (targets.empty())
		assert(done.get_depth() == 0);
	else 
		assert(done.get_depth() == targets.front().type.get_depth());

	if (error) 
		assert(option_keep_going); 

	/* 
	 * Deploy dependencies (first pass), with the F_NOTRIVIAL flag
	 */ 
	while (! buffer_default.empty()) {
		shared_ptr <Dependency> dependency_child= buffer_default.next(); 
		shared_ptr <Dependency> dependency_child_overridetrivial= 
			Dependency::clone_dependency(dependency_child);
		dependency_child_overridetrivial->add_flags(F_OVERRIDE_TRIVIAL); 
//		link_child_overridetrivial.flags |= F_OVERRIDE_TRIVIAL; 
		buffer_trivial.push(dependency_child_overridetrivial); 
//		buffer_trivial.push(move(link_child_overridetrivial)); 
		if (deploy(link, dependency_child))
			return true;
		if (jobs == 0)
			return false;
	} 
	assert(buffer_default.empty()); 

	if (order == Order::RANDOM) {
		int ret= execute_children(link);
		if (ret >= 0)
			return ret;
	}

	/* Some dependencies are still running */ 
	if (children.size())
		return false;

	/* There was an error in a child */ 
	if (error != 0) {
		assert(option_keep_going == true); 
		done.add_neg(link.avoid);
		return false;
	}

	/* Rule does not have a command.  This includes the case of dynamic
	 * executions, even though for dynamic executions the RULE variable
	 * is set (to detect cycles). */ 
	/* We cannot return here in the non-dynamic case, because we
	 * must still check that the target files exist, even if they
	 * don't have commands. */ 
	if (targets.empty() || get_dynamic_depth() != 0) {
		done.add_neg(link.avoid);
		return false;
	}

	/* Job has already been started */ 
	if (job.started_or_waited()) {
		return false;
	}

	/* Build the file itself */ 

	assert(jobs > 0); 
	assert(! targets.empty());
	assert(targets.front().type.get_depth() == 0); 
	assert(targets.back().type.get_depth() == 0); 
	assert(buffer_default.empty()); 
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
		/* Set to -1 when a file is found not to exist */ 

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
 				if (parent == nullptr || ! (link.flags & F_PERSISTENT)) 
					warn_future_file(&buf, 
							 target.name.c_str(), 
							 rule == nullptr 
							 ? parents.begin()->second.place
							 : rule->place_param_targets[i]->place); 
				/* EXISTS is not changed */ 
			} else 
				exists= -1;

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
					done.add_one_neg(F_OPTIONAL); 
					return false;
				}
			}

			if (ret_stat != 0 && errno != ENOENT) {
				/* stat() returned an actual error,
				 * e.g. permission denied:  build error */
				rule->place_param_targets[i]->place
					<< system_format(target.format_word()); 
				raise(ERROR_BUILD);
				done.add_one_neg(link.avoid); 
				return false;
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
				done.add_one_neg(link.avoid); 
				raise(ERROR_BUILD);
				return false;
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
		bool has_file= false;
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
		done.add_neg(link.avoid);
		return false;
	}

	/*
	 * The command must be run now, or there is no command. 
	 */

	/*
	 * Re-deploy all dependencies (second pass)
	 */
	while (! buffer_trivial.empty()) {
		shared_ptr <Dependency> dependency_child= buffer_trivial.next(); 
		if (deploy(link, dependency_child))
			return true;
		if (jobs == 0)
			return false;
	} 
	assert(buffer_trivial.empty()); 

	if (no_execution) {
		/* A target without a command */ 
		done.add_neg(link.avoid); 
		return false;
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
		
		done.add_one_neg(0); 

		if (option_debug) {
			string text_target= verbose_target();
			fprintf(stderr, "DEBUG %s %s create content\n",
				Verbose::padding(),
				text_target.c_str());
		}

		write_content(targets.front().name.c_str(), *(rule->command)); 
		return false;
	}
       
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
				Execution *execution_source=
					executions_by_target.at(Target(Type::FILE, source));
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
					done.add_neg(link.avoid); 
					return false;
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
			string text_target= verbose_target();
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
			done.add_neg(link.avoid); 
			return false;
		}

		executions_by_pid[pid]= this;
	}

	assert(executions_by_pid.at(pid)->job.started()); 
	assert(pid == executions_by_pid.at(pid)->job.get_pid()); 
	--jobs;
	assert(jobs >= 0);

	if (order == Order::RANDOM) {
		return jobs > 0; 
	} else if (order == Order::DFS) {
		return false;
	} else {
		assert(false);
		return false;
	}
}

int Execution::execute_children(const Link &link)
{
	/* Since unlink() may change execution->children, we must first
	 * copy it over locally, and then iterate through it */ 

	vector <Execution *> executions_children_vector
		(children.begin(), children.end()); 

	while (! executions_children_vector.empty()) {

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
		    && dynamic_pointer_cast <Direct_Dependency> (link.dependency)
		    && dynamic_pointer_cast <Direct_Dependency> (link.dependency)
		    ->place_param_target.type == Type::TRANSIENT) {
			flags_child |= link.flags; 
		}

		shared_ptr <Dependency> dependency_child= child->parents.at(this).dependency;
		
		Link link_child(avoid_child, flags_child, child->parents.at(this).place,
				dependency_child);

		if (child->execute(this, move(link_child)))
			return 1;
		assert(jobs >= 0);
		if (jobs == 0)  
			return 0;

		if (child->finished(avoid_child)) {
			unlink(this, child, 
			       link.dependency,
			       link.avoid, 
			       dependency_child, avoid_child, flags_child); 
		}
	}

	if (error) 
		assert(option_keep_going); 

	return -1;
}

void Execution::waited(pid_t pid, int status) 
{
	assert(job.started()); 
	assert(job.get_pid() == pid); 
	assert(buffer_default.empty()); 
	assert(buffer_trivial.empty()); 
	assert(children.size() == 0); 

	assert(done.get_depth() == 0);
	done.add_one_neg(0); 

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

			/* In parallel mode, print "done" message */
			if (option_parallel) {
				string text= targets[0].format_src();
				printf("Successfully built %s\n", text.c_str()); 
			}
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
				->get_single_target().unparametrized(); 
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

void Execution::main(const vector <shared_ptr <Dependency> > &dependencies)
{
	assert(jobs >= 0);

	timestamp_last= Timestamp::now(); 

	Execution *execution_root= new Execution(dependencies); 

	int error= 0; 

	try {
		while (! execution_root->finished()) {

			Link link(Stack(), (Flags) 0, Place(), shared_ptr <Dependency> ());

			bool r;

			do {
				if (option_debug) {
					fprintf(stderr, "DEBUG %s main.next\n", 
						Verbose::padding());
				}
				r= execution_root->execute(nullptr, move(link));
			} while (r);

			if (executions_by_pid.size()) {
				wait();
			}
		}

		assert(execution_root->finished()); 

		bool success= (execution_root->error == 0);
		assert(option_keep_going || success); 

		error= execution_root->error; 
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
		if (executions_by_pid.size()) {
			print_error_reminder("Terminating all running jobs"); 
			job_terminate_all();
		}

		if (e == ERROR_FATAL)
			exit(ERROR_FATAL); 

		error= e; 
	}

	if (error)
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
		string text_parent= parent->verbose_target();
		string text_child= child->verbose_target();
		string text_done_child= child->done.format();
		fprintf(stderr, "DEBUG %s %s unlink %s %s\n",
			Verbose::padding(),
			text_parent.c_str(),
			text_child.c_str(),
			text_done_child.c_str());
	}

	assert(parent != child); 
	assert(child->finished(avoid_child)); 

	if (! option_keep_going)  
		assert(child->error == 0); 

	/*
	 * Propagations
	 */

	/* Propagate dynamic dependencies */ 
	if (flags_child & F_READ) {
		
		/* Always in a [...[A]...] -> A link */

		assert(dynamic_pointer_cast <Direct_Dependency> (dependency_child)
		       && dynamic_pointer_cast <Direct_Dependency> (dependency_child)
		       ->place_param_target.type == Type::FILE);

		assert(dependency_parent->get_single_target().type.is_dynamic());
		assert(dependency_parent->get_single_target().type.is_any_file());

		assert(parent->targets.size() == 1);
#ifndef NDEBUG
		bool found= false;
		for (const Target &target:  child->targets) {
			if (target.name == parent->targets.front().name)
				found= true;
		}
		assert(found); 
#endif 

		assert(child->done.get_depth() == 0); 
		
		bool do_read= true;

		if (child->error != 0) {
			do_read= false;
		}

		/* Don't read the dependencies when the target was optional and
		 * was not built */
		/* child->exists was set to +1 earlier when the optional
		 * dependency was found to exist  */
		else if (flags_child & F_OPTIONAL) {
			if (child->exists <= 0) {
				do_read= false;
			}
		}

		if (do_read) {
			parent->read_dynamic_dependency(avoid_parent, dependency_parent); 
		}
	}

	/* Propagate timestamp.  Note:  When the parent execution has
	 * filename == "", this is unneccesary, but it's easier to not
	 * check, since that happens only once. */
	/* Don't propagate the timestamp of the dynamic dependency itself */ 
	if (! (flags_child & F_PERSISTENT) && ! (flags_child & F_READ)) {
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
	if ((flags_child & F_VARIABLE) && child->exists > 0) {

		string variable_name;
		string content;
		if (child->read_variable(variable_name, content, dependency_child))
			parent->mapping_variable[variable_name]= content;
	}

	/*
	 * Propagate variables over transient targets without commands
	 * and dynamic targets
	 */
	if (child->is_dynamic() ||
	    (dynamic_pointer_cast <Direct_Dependency> (dependency_child)
	     && dynamic_pointer_cast <Direct_Dependency> (dependency_child)->place_param_target.type == Type::TRANSIENT
	     && child->rule != nullptr 
	     && child->rule->command == nullptr)) {
		parent->mapping_variable.insert
			(child->mapping_variable.begin(), child->mapping_variable.end()); 
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
	    && ! (flags_child & F_READ)) {
		parent->need_build= true; 
	}

	/* 
	 * Remove the links between them 
	 */ 

	assert(parent->children.count(child) == 1); 
	parent->children.erase(child);

	assert(child->parents.count(parent) == 1);
	child->parents.erase(parent);
}

Execution::Execution(Target target_,
		     Link &link,
		     Execution *parent)
	:  error(0),
	   done(target_.type.get_depth(), 0),
	   timestamp(Timestamp::UNDEFINED),
	   need_build(false),
	   checked(false),
	   exists(0)
{
	assert(parent != nullptr); 
	assert(parents.empty()); 

	parents[parent]= link; 
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
			/* TARGETS contains in TARGET_ */
		} else {
			targets.clear(); 
			for (auto &place_param_target:  rule->place_param_targets) {
				targets.push_back(place_param_target->unparametrized()); 
			}
		}
	} else {
		assert(target_.type.is_dynamic()); 
		/* We must set the rule here, so cycles in the
		 * dependency graph can be detected.  Note however that
		 * the rule of dynamic file dependency executions is
		 * otherwise not used.  */ 
		Target target_base(target_.type.get_base(), target_.name);
		try {
			rule= rule_set.get(target_base, param_rule, mapping_parameter, 
					   link.dependency->get_place()); 
		} catch (int e) {
			print_traces(); 
			raise(e); 
			return; 
		}

		/* For dynamic executions, the TARGETS variables
		 * contains only a single target, which is already contained in TARGETS. */ 
	}
	assert((param_rule == nullptr) == (rule == nullptr)); 

	/* Fill TARGETS with *all* targets from the rule */
	for (const Target &target:  targets) {
		executions_by_target[target]= this; 
	}

	if (option_debug) {
		string text_target= verbose_target();
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

//			Link link_new(dep); 

			if (option_debug) {
				string text_target= verbose_target();
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
					if (parent->targets.empty()) {
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
			assert(target_.type.is_dynamic()); 
		}
		
		if (rule_not_found) {
			assert(rule == nullptr); 
			print_traces(fmt("no rule to build %s", 
					 target_.format_word()));
			raise(ERROR_BUILD);
			/* Even when a rule was not found, the Execution object remains
			 * in memory  */  
		}
	}

}

Execution::Execution(const vector <shared_ptr <Dependency> > &dependencies_)
/* 
 * This is the root execution object.  It has an empty TARGET list, and
 * in principle should be deleted once its lifetime is over.  This is
 * not done however, as there is only a single such object, and its
 * lifetime span the whole lifetime of the Stu process anyway.
 */
	:  error(0),
	   need_build(false),
	   checked(false),
	   exists(0)
{
	for (auto &d:  dependencies_) {
		push_default(d); 
//		buffer_default.push(Link(i)); 
	}
}

bool Execution::finished(Stack avoid) const
{
	assert(avoid.get_depth() == done.get_depth());

	if (targets.empty())
		assert(done.get_depth() == 0);
	else {
		assert(done.get_depth() == targets.front().type.get_depth());
		assert(done.get_depth() == targets.back().type.get_depth());
	}

	Flags to_do_aggregate= 0;
	
	for (unsigned j= 0;  j <= done.get_depth();  ++j) {
		to_do_aggregate |= ~done.get(j) & ~avoid.get(j); 
	}

	return (to_do_aggregate & ((1 << C_TRANSITIVE) - 1)) == 0; 
}

bool Execution::finished() const 
{
	if (targets.empty())
		assert(done.get_depth() == 0);
	else 
		assert(done.get_depth() == targets.front().type.get_depth());

	Flags to_do_aggregate= 0;
	
	for (unsigned j= 0;  j <= done.get_depth();  ++j) {
		to_do_aggregate |= ~done.get(j); 
	}

	return (to_do_aggregate & ((1 << C_TRANSITIVE) - 1)) == 0; 
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
	
	for (auto i= Execution::executions_by_pid.begin();
	     i != Execution::executions_by_pid.end();  ++i) {

		const pid_t pid= i->first;

		Job::kill(pid); 
	}

	int count_terminated= 0;

	for (auto i= Execution::executions_by_pid.begin();
	     i != Execution::executions_by_pid.end();  ++i) {

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
	for (auto &i:  Execution::executions_by_pid) {
		i.second->print_as_job(); 
	}
}

bool Execution::find_cycle(const Execution *const parent, 
			   const Execution *const child,
			   const Link &link)
{
	/* Happens when the parent is the root execution */ 
	if (parent->param_rule == nullptr)
		return false;
		
	/* Happens with files that should be there and have no rule */ 
	if (child->param_rule == nullptr)
		return false; 

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
		if (next->param_rule == nullptr)
			continue;

		path.push_back(next); 

		bool found= find_cycle(path, child, link);
		if (found)
			return true;

		path.pop_back(); 
	}
	
	return false; 
}

bool Execution::remove_if_existing(bool output) 
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

		execution= new Execution(target, link, parent);  

		assert(execution->parents.size() == 1); 
	}

	if (find_cycle(parent, execution, link)) {
		parent->raise(ERROR_LOGICAL);
		return nullptr;
	}

	execution->initialize(link.avoid); 

	return execution;
}

void Execution::read_dynamic_dependency(Stack avoid,
					shared_ptr <Dependency> dependency_this)
{
	assert(dynamic_pointer_cast <Dynamic_Dependency> (dependency_this)); 

	Target target= dependency_this->get_single_target().unparametrized(); 

	assert(target.type.is_dynamic());

	assert(avoid.get_depth() == target.type.get_depth()); 

	try {
		const string filename= target.name; 
		vector <shared_ptr <Dependency> > dependencies;

		Flags flags= dynamic_pointer_cast <Dynamic_Dependency>
			(dependency_this)->dependency->get_flags();

		if (! (flags & (F_NEWLINE_SEPARATED | F_ZERO_SEPARATED))) {

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

			/* We use getdelim() for parsing.  A more
			 * optimized way would be via mmap()+strchr(), but
			 * why the complexity?  */
			
			const char c= (flags & F_NEWLINE_SEPARATED) ? '\n' : '\0';
			/* The delimiter */ 

			const char c_printed=
				(flags & F_NEWLINE_SEPARATED) ? 'n' : '0';
			
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
					/* Should not happen, so abort parse */ 
					assert(false); 
					break;
				} 

				/* There may or may not be a terminating
				 * \n.  getdelim(3) will include it if it is
				 * present, but the file may not have
				 * one.  */ 

				if (lineptr[len - 1] == c) {
					--len; 
				}

				/* An empty line: This corresponds to an
				 * empty filename, and thus we treat is
				 * as a syntax error.  */ 
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
					(make_shared <Direct_Dependency>
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

		for (auto &j:  dependencies) {

			/* Check that it is unparametrized */ 
			if (! j->is_unparametrized()) {
				shared_ptr <Dependency> dep= j;
				while (dynamic_pointer_cast <Dynamic_Dependency> (dep)) {
					shared_ptr <Dynamic_Dependency> dep2= 
						dynamic_pointer_cast <Dynamic_Dependency> (dep);
					dep= dep2->dependency; 
				}
				dynamic_pointer_cast <Direct_Dependency> (dep)
					->place_param_target.place_name.places[0] <<
					fmt("dynamic dependency %s must not contain "
					    "parametrized dependencies",
					    target.format_word());
				Target target_base= target;
				target_base.type= target.type.get_base();
				print_traces(fmt("%s is declared here", 
						 target_base.format_word())); 
				raise(ERROR_LOGICAL);
				continue; 
			}

			/* Check that there is no multiply-dynamic variable dependency */ 
			if (j->has_flags(F_VARIABLE) && 
			    target.type.is_dynamic() && target.type != Type::DYNAMIC_FILE) {
				
				/* Only direct dependencies can have the F_VARIABLE flag set */ 
				assert(dynamic_pointer_cast <Direct_Dependency> (j));

				shared_ptr <Direct_Dependency> dep= 
					dynamic_pointer_cast <Direct_Dependency> (j);

				bool quotes= false;
				string s= dep->place_param_target.format(0, quotes);

				j->get_place() <<
					fmt("variable dependency %s$[%s%s%s]%s must not appear",
					    Color::word,
					    quotes ? "'" : "",
					    s,
					    quotes ? "'" : "",
					    Color::end
					    );
				print_traces(fmt("within multiply-dynamic dependency %s", 
						 target.format_word())); 
				raise(ERROR_LOGICAL);
				continue; 
			}

			/* Add the found dependencies, with one less dynamic level
			 * than the current target.  */

			shared_ptr <Dependency> dependency(j);

			vector <shared_ptr <Dynamic_Dependency> > vec;
			shared_ptr <Dependency> p= dependency_this;

			while (dynamic_pointer_cast <Dynamic_Dependency> (p)) {
				shared_ptr <Dynamic_Dependency> dynamic_dependency= 
					dynamic_pointer_cast <Dynamic_Dependency> (p);
				vec.resize(vec.size() + 1);
				vec[vec.size() - 1]= dynamic_dependency;
				p= dynamic_dependency->dependency;   
			}

			Stack avoid_this= avoid;
			assert(vec.size() == avoid_this.get_depth());
			avoid_this.pop(); 
			dependency->add_flags(avoid_this.get_lowest()); 
			if (dependency->get_place_flag(I_PERSISTENT).empty())
				dependency->set_place_flag
					(I_PERSISTENT,
					 vec[target.type.get_depth() - 1]
					 ->get_place_flag(I_PERSISTENT)); 
			if (dependency->get_place_flag(I_OPTIONAL).empty())
				dependency->set_place_flag
					(I_OPTIONAL,
					 vec[target.type.get_depth() - 1]
					 ->get_place_flag(I_OPTIONAL)); 
			if (dependency->get_place_flag(I_TRIVIAL).empty())
				dependency->set_place_flag
					(I_TRIVIAL,
					 vec[target.type.get_depth() - 1]
					 ->get_place_flag(I_TRIVIAL)); 

			for (Type k= target.type - 1;  k.is_dynamic();  --k) {
				avoid_this.pop(); 
				Flags flags_level= avoid_this.get_lowest(); 
				dependency= make_shared <Dynamic_Dependency> 
					(flags_level, dependency); 
				dependency->set_place_flag
					(I_PERSISTENT,
					 vec[k.get_depth() - 1]->get_place_flag(I_PERSISTENT)); 
				dependency->set_place_flag
					(I_OPTIONAL,
					 vec[k.get_depth() - 1]->get_place_flag(I_OPTIONAL)); 
				dependency->set_place_flag
					(I_TRIVIAL,
					 vec[k.get_depth() - 1]->get_place_flag(I_TRIVIAL)); 
			}

			assert(avoid_this.get_depth() == 0); 

			push_default(dependency); 
//			buffer_default.push(Link(dependency));

		}
				
	} catch (int e) {
		/* We catch not only the errors raised in this function,
		 * but also the errors raised in the parser.  */
		raise(e); 
	}
}

void Execution::warn_future_file(struct stat *buf, 
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
	if (execution->targets.empty())
		return;

	bool first= true; 

	/* If there is a rule for this target, show the message with the
	 * rule's trace, otherwise show the message with the first
	 * dependency trace */ 
	if (execution->param_rule != nullptr && text != "") {
		execution->param_rule->place << text;
		first= false;
	}

	string text_parent= parents.begin()->second.dependency
		->get_single_target().format_word();

	while (true) {

		auto i= execution->parents.begin(); 

		if (i->first->targets.empty()) {

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
			->get_single_target().format_word();

 		const Place place= i->second.place;
		/* Set even if not output, because it may be used later
		 * for the root target  */

		/* Don't show [[A]]->A edges */
		if (i->second.flags & F_READ) {
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

void Execution::print_command() const
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

	if (!single_line || option_parallel) {
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

bool Execution::deploy(const Link &link,
		       shared_ptr <Dependency> dependency_child)
{
	if (option_debug) {
		string text_target= verbose_target();
		string text_child= dependency_child->format_out(); 
		fprintf(stderr, "DEBUG %s %s deploy %s\n",
			Verbose::padding(),
			text_target.c_str(),
			text_child.c_str());
	}

	/* Additional flags for the child are added here */ 
	Flags flags_child= dependency_child->get_flags(); 
//	Flags flags_child= link_child.flags; 
	Flags flags_child_additional= 0; 

	/* Compound dependency */
	if (dynamic_pointer_cast <Compound_Dependency> (dependency_child)) {

		/* Does not happen as the buffers do not contain
		 * compound dependencies.  */
		assert(false); 
//		shared_ptr <Compound_Dependency> compound_dependency= 
//			dynamic_pointer_cast <Compound_Dependency> (link_child.dependency);
	}

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
	assert(dynamic_pointer_cast <Direct_Dependency> (dep)); 

	shared_ptr <Direct_Dependency> direct_dependency=
		dynamic_pointer_cast <Direct_Dependency> (dep);
	assert(direct_dependency != nullptr); 
	assert(! direct_dependency->place_param_target.place_name.empty()); 

	Target target_child= direct_dependency->place_param_target.unparametrized();
	assert(target_child.type == Type::FILE || target_child.type == Type::TRANSIENT);
	assert(target_child.type.get_depth() == 0); 

	if (depth != 0) {
		assert(depth > 0);
		target_child.type += depth; 
	}

//	Stack avoid_child= link_child.avoid;
//	assert(avoid_child.get_depth() == target_child.type.get_depth()); 

	/* Carry flags over transient targets */ 
	if (! targets.empty()) {
		Target target= link.dependency->get_single_target().unparametrized();

		if (target.type == Type::TRANSIENT) { 
			flags_child_additional |= link.flags; 
//			avoid_child.add_highest(link.flags);
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
		direct_dependency->place <<
			fmt("in declaration of dependency %s", 
			    target_child.format_word());
		print_traces();
		explain_clash(); 
		raise(ERROR_LOGICAL);
		return false;
	}

	/* Either of '-p'/'-o'/'-t' does not mix with '$[' */
	if ((flags_child & F_VARIABLE) &&
	    (flags_child_additional & (F_PERSISTENT | F_OPTIONAL | F_TRIVIAL))) {

		assert(target_child.type == Type::FILE); 
		const Place &place_variable= direct_dependency->place;
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
		return false;
	}

	flags_child= flags_child_new; 

	Link link_child_new(avoid_child, 
			    flags_child, 
			    dependency_child->get_place(), 
			    dependency_child); 

	Execution *child= Execution::get_execution
		(target_child, 
		 link_child_new,
		 this);  
	if (child == nullptr) {
		/* Strong cycle was found */ 
		return false;
	}

	children.insert(child);
	
	if (child->execute(this, move(link_child_new)))
		return true;
	assert(jobs >= 0);
	if (jobs == 0)  
		return false;
			
	if (child->finished(avoid_child)) {
		unlink(this, child, 
		       link.dependency,
		       link.avoid, 
		       dependency_child,
		       avoid_child, flags_child);
	}

	return false;
}

void Execution::initialize(Stack avoid) 
{
	/* Add the special dynamic dependency, i.e., the [[A]]-A type
	 * link.  Add, as an initial dependency, the corresponding file
	 * or transient.  */
	if (! targets.empty() &&
	    targets.front().type.is_dynamic()) {

		assert(targets.size() == 1); 
		const Target &target= targets.front(); 

		Flags flags_child= avoid.get_lowest();
		
		if (target.type.is_any_file())
			flags_child |= F_READ;

		shared_ptr <Dependency> dependency_child= make_shared <Direct_Dependency>
			(flags_child,
			 Place_Param_Target(target.type.get_base(), 
					    Place_Name(target.name)));

		push_default(dependency_child); 
// 		push_default(Link(dependency_child, flags_child, Place()));
// 		buffer_default.push(Link(dependency_child, flags_child, Place()));
		/* The place of the [[A]]->A links is empty, meaning it will
		 * not be output in traces. */ 
	} 
}

void Execution::print_as_job() const
{
	pid_t pid= job.get_pid();

	string text_target= targets.front().format_src(); 

	printf("%7ld %s\n", (long) pid, text_target.c_str());
}

void Execution::write_content(const char *filename, 
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

bool Execution::read_variable(string &variable_name,
			      string &content,
			      shared_ptr <Dependency> dependency)
{
	Target target= dependency->get_single_target().unparametrized(); 

	assert(target.type == Type::FILE);

	size_t filesize;
	struct stat buf;
	string dependency_variable_name;
	
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
		dynamic_pointer_cast <Direct_Dependency> (dependency)->name; 

	variable_name= 
		dependency_variable_name == "" ?
		target.name : dependency_variable_name;

	return true;

 error_fd:
	close(fd); 
 error:
	assert(dynamic_pointer_cast <Direct_Dependency> (dependency)); 
	Target target_variable= 
		dynamic_pointer_cast <Direct_Dependency> (dependency)->place_param_target
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
			.dependency->get_single_target().format_word();
	}

	names.back()= path.back()->parents.begin()->second
		.dependency->get_single_target().format_word();
		
	for (signed i= path.size() - 1;  i >= 0;  --i) {

		/* Don't show a message for [...[A]...] -> X links */ 
		if (i != 0 &&
		    path[i - 1]->parents.at((Execution *) path[i])
		    .dependency->get_flags() & F_READ)
			continue;

		/* Same, but when [...[A]...] is at the bottom */
		if (i == 0 && link.dependency->get_flags() & F_READ) 
			continue;

		(i == 0 ? link : path[i - 1]->parents.at((Execution *) path[i]))
			.place
			<< fmt("%s%s depends on %s",
			       i == (int) (path.size() - 1) 
			       ? (path.size() == 1 
				  || (path.size() == 2 &&
				      link.dependency->get_flags() & F_READ)
				  ? "target must not depend on itself: " 
				  : "cyclic dependency: ") 
			       : "",
			       names[i],
			       i == 0
			       ? link.dependency->get_single_target().format_word()
			       : names[i - 1]);
	}

	/* If the two targets are different (but have the same rule
	 * because they match the same pattern), then output a notice to
	 * that effect */ 
	if (link.dependency->get_single_target() !=
	    path.back()->parents.begin()->second
	    .dependency->get_single_target()) {

		
		Param_Target t1= path.back()->parents.begin()->second
			.dependency->get_single_target();
		Param_Target t2= link.dependency->get_single_target();
		t1.type= t1.type.get_base();
		t2.type= t2.type.get_base(); 

		path.back()->rule->place <<
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
		execution_a->get_dynamic_depth() == execution_b->get_dynamic_depth() &&
		execution_a->param_rule == execution_b->param_rule;
}

void Execution::raise(int error_)
{
	assert(error_ >= 1 && error_ <= 3); 

	error |= error_;

	if (! option_keep_going)
		throw error;
}

bool Execution::is_dynamic() const
{
	return targets.size() != 0 && targets.front().type.is_dynamic(); 
}

void Execution::push_default(shared_ptr <Dependency> dependency)
{
	vector <shared_ptr <Dependency> > dependencies;
	Dependency::split_compound_dependencies(dependencies, dependency); 
//	assert(dependencies.size() > 0);
       
	for (const auto &d:  dependencies) {
		buffer_default.push(d);
	}
}

#endif /* ! EXECUTION_HH */
