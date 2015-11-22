#ifndef EXECUTION_HH
#define EXECUTION_HH

/* Code for executing the building process itself.  This is by
 * far the longest source code file in Stu.  Each file or phony target
 * is represented at runtime by one Execution object.  All Execution objects
 * are allocated with new Execution(...), and are never deleted, as the
 * information contained in them needs to be cached.  All Execution objects
 * are also stored in the map called "executions_by_target" by their
 * target.  All currently active Execution objects form a rooted acyclic
 * graph.  Note that it is not a tree in the general case; executions
 * may have multiple parents.  But all nodes are reachable from the root
 * node.   
 */

#include <queue>
#include <map> 
#include <unordered_set>

#include "process.hh"
#include "timestamp.hh"

/* Information about one parent--child link. 
 */ 
class Link
{
private:
	void check() const {
		avoid.check();
		if (avoid.get_k() == 0) {
			assert(avoid.get_lowest() == (flags & ((1 << F_COUNT) - 1)));
		}
	}

public:

	/* The length is one plus the dynamicity (number of dynamic
	 * indirections).  Small indices indicate the links lower in the
	 * hierarchy. 
	 * This variable only needs to hold the EXISTENCE and OPTIONAL
	 * bits (i.e., transitive bits). 
	 */
	Stack avoid;

	/* Flags that are valid for this dependency */ 
	Flags flags;

	/* The place of the declaration of the dependency */ 
	Place place; 

	shared_ptr <Dependency> dependency;

	Link() { }

	Link(Stack avoid_,
	     Flags flags_,
	     Place place_,
	     shared_ptr <Dependency> dependency_)
		:  avoid(avoid_),
		   flags(flags_),
		   place(place_),
		   dependency(dependency_)
	{ 
		check(); 
	}

	Link(shared_ptr <Dependency> dependency_,
	     Flags flags_,
	     const Place &place_)
		:  avoid(dependency_),
		   flags(flags_),
		   place(place_),
		   dependency(dependency_)
	{ 
		check();
	}

	/* The flags of the Dependency_Info only contain the top-level flags
	 * of the dependency if this is a direct dependency. 
	 */ 
	Link(shared_ptr <Dependency> dependency_)
		:  avoid(dependency_),
		   flags(dynamic_pointer_cast <Dynamic_Dependency> (dependency_)
			 ? (dependency_->get_flags() & ~((1 << F_COUNT) - 1))
			 : dependency_->get_flags()),
		   place(dependency_->get_place()),
		   dependency(dependency_)
	{ 
		check(); 
	}

	void add(Stack avoid_, Flags flags_) {
		assert(avoid.get_k() == avoid_.get_k());
		avoid.add(avoid_);
		flags |= flags_;
		check(); 
	}
};

class Execution
{
public:

	/* Target to build */ 
	const Target target;

	/* The instantiated file rule for this execution.  NULL when there
	 * is no rule for this file (this happens for instance when a
	 * source code file is given as a dependency, or when this is a
	 * complex dependency).  Individual dynamic dependencies do have
	 * rules, in order for cycles to be detected. 
	 */ 
	shared_ptr <Rule> rule;

	/* The file/phony rule from which this execution was derived.  This is
	 * only used to detect strong cycles.  To manage the dependencies, the
	 * instantiated general rule is used.  NULL if and only if RULE is
	 * NULL. 
	 */ 
	shared_ptr <Rule> param_rule;

	/* Currently running executions.  Allocated with new.  Contains
	 * both dependency-subs and dynamic-subs.  
	 */
	unordered_set <Execution *> children;

	/* The parent executions */ 
	unordered_map <Execution *, Link> parents; 

	/* The process used to build this file */ 
	Process process;
	
	/* Dependencies that have not yet begun to be built.
	 * Initialized with all dependencies, and emptied over time when
	 * things are built, and filled over time when dynamic dependencies
	 * are worked on. 
	 * Entries are not necessarily unique.  The traces within the
	 * dependencies refer to the declaration of the dependencies,
	 * not to the rules of the dependencies. 
	 */ 
	queue <Link> buffer;

	/* The buffer for trivial dependencies.  They are only started
	 * if, after (potentially) starting all non-trivial
	 * dependencies, the target must be rebuilt anyway. 
	 */
	queue <Link> buffer_trivial;

	/* Info about the target before it is built.  Only valid once the
	 * process was started.  Used for checking whether a file was
	 * rebuild to decide whether to remove it after a command failed or
	 * was interrupted.  The field .tv_sec is LONG_MAX when the file did
	 * not exist, or the target is not a file
	 */ 
	Timestamp timestamp_old; 

	/* Variable assignments for when the command is run.  Filled over
	 * time with the various types of variables that are possible.
	 * Note:  we use an ordered map in order to be able to output all
	 * variable assignments alphabetically; otherwise the order does not
	 * matter. 
	 */
	map <string, string> mapping_parameter; 

	/* Variable assignments to be printed when the command is executed. 
	 */
	map <string, string> mapping_variable; 

	/* Error status of this target.  The value is propagated (using '|')
	 * to the parent.  Values correspond to constants defined in
	 * error.hh; zero denotes the absence of an error. 
	 */ 
	int error;

	/* Whether this target needs to be built.  When a
	 * target is finished, this value is propagated to the parent
	 * executions (except when the F_EXISTENCE flag is set). 
	 */ 
	bool need_build;

	/* Whether we performed the check in execute().  (Only for FILE
	 * targets). 
	 */ 
	bool checked;

	/* What parts of this target have been done. Each bit represents
	 * one task done.  The depth K is equal to the dynamicity
	 * for dynamic targets, and to zero for non-dynamic targets. 
	 */
	Stack done;

	/* Latest timestamp of a (direct or indirect) file dependency that
	 * was not rebuilt.  (Files that were not rebuilt are not
	 * considered, since they make the target be rebuilt anyway.)
	 * The function execute() also changes this to consider the file
	 * itself.  This final timestamp is then carried over to the parent
	 * executions.   
	 */
	Timestamp timestamp; 

	/* Whether the file is know to exist.  
	 * -1 = no 
	 * 0  = unknown or not a T_FILE
	 * +1 = yes
	 */
	int exists;
	
	/* File, phony and dynamic targets.  
	 */ 
	Execution(Target target_,
		  const Link &,
		  Execution *parent);

	/* Empty execution.  DEPENDENCIES don't have to be unique.
	 */
	Execution(const vector <shared_ptr <Dependency> > &dependencies_); 

	~Execution(); 

	/* Whether the execution is finished working for the PARENT */ 
	bool finished(Stack avoid) const; 

	/* Whether the execution is completely finished */ 
	bool finished() const;

	/* Append DEPENDENCIES to the dependency queue, adding TARGET and
	 * PLACE to the stack of each */ 
	void append_dependencies(const vector <Link> &dependencies);

	/* Read dynamic dependencies from a file.  Can only be called for
	 * dynamic targets.  Called for the parent of a dynamic--file link. 
	 */ 
	void read_dynamics(Stack avoid, shared_ptr <Dependency> dependency_parent);

	/* Remove the targe file if it exists.  If OUTPUT is true, output a
	 * corresponding error message.  Return whether the file was
	 * removed.  
	 */
	bool remove_if_existing(bool output); 

	/* Add the dependency to the dependency queue and return TRUE, or
	 * return FALSE when it was already there which the same or weaker
	 * dependency type.   
	 */
	void add_dependency(const Link &dependency);

	/* Start the next K processes.  Return the new value of K, i.e. the
	 * number of processes left to start.  This will also terminate
	 * processes when they don't need to be run anymore, and thus it can
	 * be called with K = 0 just to terminate processes that need to be
	 * terminated.  The passed FLAG is the ORed combination
	 * of all FLAGs up the dependency chain.  
	 * DEPENDENCY is the dependency linking the two executions.  
	 */
	// TODO Make K be a global variable. 
 	int execute(Execution *parent, 
		    int k, 
		    const Link &link);

	/* Called after the process was waited for.  The PID is only passed
	 * for checking that it is correct. 
	 */
	void waited(int pid, int status); 

	/* Print full trace for the execution.  First the message is
	 * printed, then all traces for it starting at this execution. 
	 * TEXT may be "" to not print any additional message. 
	 */ 
	void print_traces(string text= "") const;

	/* Warn when the file has a modification time in the future */ 
	void warn_future_file(struct stat *buf);

	/* Note:  the top-level flags of LINK.DEPENDENCY may be modified. 
	 */
	void deploy_dependency(int &k, 
			       const Link &link,
			       const Link &link_child);

	/* Initialize the Execution object.  Used for dynamic dependencies.
	 * Called from get_execution() before the object is connected to a
	 * new parent. */ 
	void initialize(Stack avoid);

	/* Print a command and its associated variable assignments,
	 * according to the selected verbosity level.  
	 * FILENAME_OUTPUT and FILENAME_INPUT are "" when not used. 
	 */ 
	void print_command(); 

	/* The currently running executions by process IDs */ 
	static unordered_map <pid_t, Execution *> executions_by_pid;

	/* All Execution objects.  Execution objects are never deleted.  This serves
	 * as a caching mechanism.   
	 */
	static unordered_map <Target, Execution *> executions_by_target;

	/* The timestamps for phonies.  This container serves as the
	 * "file system" for phonies, holding their timestamps. 
	 */
	static unordered_map <string, Timestamp> phonies;

	/* The timepoint of the last time wait() returned.  No file in the
	 * filesystem should be newer than this. 
	 */ 
	static Timestamp timestamp_last; 

	/* Set once before calling execute_main().  Unchanging during the whole call
	 * to execute_main(). 
	 */ 
	static Rule_Set rule_set; 

	/* Whether any process was started */ 
	static bool worked;

	/* Propagate information from the subexecution to the execution, and
	 * then delete the child execution.  The child execution is
	 * however not deleted as it is kept for caching. 
	 */
	static void unlink_execution(Execution *const parent, 
				     Execution *const child,
				     shared_ptr <Dependency> dependency_parent,
				     Stack avoid_parent,
				     Stack avoid_child,
				     Flags flags_child); 

	/* Get an existing execution or create a new one.
	 * Return NULL when a strong cycle was found; return the execution
	 * otherwise.  PLACE is the place of where the dependency was
	 * declared.    
	 */ 
	static Execution *get_execution(const Target &target, 
					const Link &link,
					Execution *parent); 

	/* Main execution loop.  Execute K processes in parallel. 
	 * Return the same error code as in Execution::error. 
	 */
	static int execute_main(const vector <Target> &targets, 
				const vector <Place> &places,
				int k);

	/* Wait for next process to finish and finish it.  Do not start anything
	 * new.  
	 */ 
	static void execute_wait();

	/* Find a cycle between CHILD and one of its parent executions.  This
	 * is the main entry point of the two find_cycle() functions. 
	 * A cycle is defined not in terms of
	 * filenames, but in terms of general rules, i.e., it is an error if
	 * the file "a.gz" depends on the file "a.gz.gz", when both of them
	 * came from the general rule for "$NAME.gz".  This is to make sure
	 * we don't get into infite recursion such as with:
	 *
	 * $NAME.gz:  $NAME.gz.gz { ... }
	 */ 
	static bool find_cycle(const Execution *const parent, 
			       const Execution *const child); 

	/* The helper function for find_cycle().  TRACES contains the list
	 * of traces connected CHILD to EXECUTION. 
	 */
	static bool find_cycle(const Execution *const parent, 
			       const Execution *const child,
			       vector <Trace> &traces);

	static string cycle_string(const Execution *execution);

	/* Return NULL when no trace should be given */ 
	static shared_ptr <Trace> cycle_trace(const Execution *child,
					      const Execution *parent);

};

unordered_map <pid_t, Execution *> Execution::executions_by_pid;
unordered_map <Target, Execution *> Execution::executions_by_target;
unordered_map <string, Timestamp> Execution::phonies;
Timestamp Execution::timestamp_last;
Rule_Set Execution::rule_set; 

bool Execution::worked= false;

void Execution::execute_wait() 
{
	assert(Execution::executions_by_pid.size()); 

	int status;
	pid_t pid= Process::wait_do(&status); 

	timestamp_last= Timestamp::now(); 

	Execution *const execution= executions_by_pid.at(pid); 

	execution->waited(pid, status); 
}

int Execution::execute(Execution *parent, 
		       int k, 
		       const Link &link)
{
	assert(k >= 0); 
	assert(link.avoid.get_k() == dynamic_depth(target.type)); 
	assert(done.get_k() == dynamic_depth(target.type));
	if (dynamic_depth(target.type) == 0) {
		assert(link.avoid.get_lowest() == (link.flags & ((1 << F_COUNT) - 1))); 
	}
	done.check();

	if (finished(link.avoid))
		return k;

	/* 
	 * Continue the already-active child executions 
	 */  

	/* Since unlink_execution() may change execution->children,
	 * we must first copy it over locally, and then iterate
	 * through it */ 

	unordered_set <Execution *> executions_children_copy= children;

	for (auto i= executions_children_copy.begin();
	     i != executions_children_copy.end(); ++i) {

		Execution *child= *i;
		
		assert(child != NULL);

		Stack avoid_child= child->parents.at(this).avoid;
		Flags flags_child= child->parents.at(this).flags;

		if (target.type == T_PHONY) { 
			flags_child |= link.flags; 
		}
		
		Link link_child(avoid_child, flags_child, child->parents.at(this).place,
						child->parents.at(this).dependency);

		k= child->execute(this, k, link_child);
		assert(k >= 0);
		if (k == 0)  
			return k; 

		if (child->finished(avoid_child)) {
			unlink_execution(this, child, 
							 link.dependency,
							 link.avoid, 
							 avoid_child, flags_child); 
		}
	}

	if (error) 
		assert(option_continue); 

	/* Should children even be started?  Check whether this is an
	 * optional dependency and if it is, return when the file does not
	 * exist.  
	 */
	if ((link.flags & F_OPTIONAL) && target.type == T_FILE) {
		struct stat buf;
		int ret_stat= stat(target.name.c_str(), &buf);
		if (ret_stat < 0) {
			if (errno != ENOENT) {
				perror(target.name.c_str());
				if (option_continue) {
					error |= ERROR_BUILD;
					done.add_neg(link.avoid); 
					exists= -1;
					return k;
				} else {
					exit(ERROR_SYSTEM); 
				}
			}
			done.add_highest_neg(link.avoid.get_highest()); 
			return k;
		} else {
			assert(ret_stat == 0);
			exists= +1;
		}
	}

	assert(done.get_k() == dynamic_depth(target.type));

	if (error) 
		assert(option_continue); 

	/* 
	 * Deploy non-trivial dependencies
	 */ 
	while (buffer.size()) {
		Link link_child= buffer.front(); 
		buffer.pop(); 
		if (link_child.flags & F_TRIVIAL) {
			buffer_trivial.push(link_child); 
		} else {
			deploy_dependency(k, link, link_child);
			if (k == 0)
				return k;
		}
	} 
	assert(buffer.empty()); 

	/* Some dependencies are still running */ 
	if (children.size())
		return k;

	/* There was an error in a child */ 
	if (error != 0) {
		assert(option_continue == true); 
		done.add_neg(link.avoid);
		return k;
	}


	/* Rule does not have a command.  This includes the case of dynamic
	 * executions, even though for dynamic executions the RULE variable
	 * is set (to detect cycles). */ 
	if ((target.type == T_PHONY && ! (rule != NULL && rule->command != NULL))
		|| target.type == T_EMPTY
		|| target.type >= T_DYNAMIC) {

		done.add_neg(link.avoid);
		return k;
	}

	/* Process has already been started */ 
	if (process.started_or_waited()) {
		return k;
	}

	/* Build the file itself */ 
	assert(k > 0); 
	assert(target.type == T_FILE || target.type == T_PHONY); 
	assert(buffer.empty()); 
	assert(children.empty()); 
	assert(error == 0);

	if (verbosity >= VERBOSITY_VERBOSE) {
		string text= target.text();
		fprintf(stderr, "Building %s\n", 
				text.c_str()); 
		fprintf(stderr, "\tneed_build = %u\n", need_build);
	}

	/*
	 * Check whether execution has to be built
	 */

	/* Check existence of file */
	struct stat buf;
	int ret_stat; 
	timestamp_old= Timestamp::UNDEFINED;

	const bool no_command= rule != NULL && rule->command == NULL;

	if (! checked && target.type == T_FILE) {

		checked= true; 

		/* We save the return value of stat() and handle errors later */ 
		ret_stat= stat(target.name.c_str(), &buf);

		/* Warn when file has timestamp in the future */ 
		if (ret_stat == 0) { 
			/* File exists */ 
			timestamp_old= Timestamp(&buf);
			if (parent == NULL || ! (link.flags & F_EXISTENCE)) {
				warn_future_file(&buf);
			}
			exists= +1; 
		} else {
			exists= -1;
		}
 
		if (! need_build) { 
			if (ret_stat == 0) {
				/* File exists. Check whether it has to be rebuilt
				 * because of more up to date dependencies */ 

				if (timestamp.defined() && timestamp_old.older_than(timestamp)) {
					if (verbosity >= VERBOSITY_VERBOSE) {
						fprintf(stderr, "\trebuilding because dependencies are newer\n"); 
						string text_timestamp= timestamp.format();
						string text_timestamp_old= timestamp_old.format(); 
						fprintf(stderr, "\ttimestamp of dependency = %s\n",
								text_timestamp.c_str()); 
						fprintf(stderr, "\ttimestamp of '%s' = %s\n",
								target.name.c_str(), 
								text_timestamp_old.c_str()); 
					}
					if (no_command) {
						print_warning(fmt("File target '%s' which has no command "
								  "is older than its dependency",
								  target.name));
					} else {
						need_build= true;
					}

				} else {
					timestamp= timestamp_old;
				}
			} else {
				/* Note:  Rule may be NULL here for optional
				 * dependencies that do not exist and do not have a
				 * rule */

				if (errno == ENOENT) {
					/* File does not exist */

					if (! (link.flags & F_OPTIONAL)) {
						if (verbosity >= VERBOSITY_VERBOSE) {
							fprintf(stderr, "\trebuilding because file does not exist\n"); 
						}
						need_build= true; 
					} else {
						/* Optional dependency:  don't create the file;
						 * it will then not exist when the parent is
						 * called. 
						 */ 
						if (verbosity >= VERBOSITY_VERBOSE) {
							fprintf(stderr, 
									"\tis an optional dependency that is not present\n"); 
						}
						done.add_one_neg(F_OPTIONAL); 
						return k;
					}
				} else {
					/* stat() returned an actual error,
					 * e.g. permission denied:  fail */
					perror(target.name.c_str());
					if (! option_continue)
						exit(ERROR_SYSTEM); 
					error |= ERROR_BUILD;
					done.add_one_neg(link.avoid); 
					return k;
				}
			}
		}

		/* File does not exist, all its dependencies are up to
		 * date, and the file has no commands: that's an error. */  
		if (0 != ret_stat && no_command) {

			/* Case has already been checked, and an
			 * exception thrown */ 
			assert(errno == ENOENT); 

			if (rule->dependencies.size()) {
				print_traces
					(fmt("file without command '%s' does not exist, "
					     "although all its dependencies are up to date", 
					     target.name)); 
			} else {
				print_traces
					(fmt("file without command and without dependencies "
						 "'%s' does not exist",
						 target.name)); 
			}
			error |= ERROR_BUILD;
			done.add_one_neg(link.avoid); 
			if (! option_continue) 
				throw error;  
			return k;
		}		
	}

	if (! need_build && target.type == T_PHONY) {
		if (! phonies.count(target.name)) {
			/* Phony was not yet executed */ 
			need_build= true; 
			if (verbosity >= VERBOSITY_VERBOSE) {
				fprintf(stderr, "\trunning because this phony was not yet executed\n");
			}
		}
	}

	if (! need_build) {
		done.add_neg(link.avoid);
		return k;
	}

	/*
	 * The command must be run now. 
	 */

	/* A target without a command */ 
	if (no_command) {
		if (verbosity >= VERBOSITY_VERBOSE) {
			fprintf(stderr, "\ttarget without command\n");
		}
		
		if (buffer_trivial.size() != 0) {
			const Link &link_child= buffer_trivial.front();
			assert(link_child.dependency->get_flags() & F_TRIVIAL); 
			link_child.dependency->get_place_trivial() <<
				fmt("target without command %s cannot have trivial dependencies",
				    target.text()); 
			error |= ERROR_LOGICAL;
			done.add_neg(link.avoid); 
			if (! option_continue) 
				throw error;  
			return k;
		}

		done.add_neg(link.avoid); 
		return k;
	}

	/*
	 * Deploy trivial dependencies
	 */
	while (buffer_trivial.size()) {
		Link link_child= buffer_trivial.front(); 
		buffer_trivial.pop(); 
		deploy_dependency(k, link, link_child);
		if (k == 0)
			return k;
	} 
	assert(buffer_trivial.empty()); 

	worked= true; 
	
	if (target.type == T_PHONY) {
		Timestamp timestamp_now= Timestamp::now(); 
		assert(timestamp_now.defined()); 
		phonies[target.name]= timestamp_now; 
	}

	/* Output the command */ 
	if (verbosity >= VERBOSITY_VERBOSE) {
		fprintf(stderr, "\tstarting\n"); 
	}
	if (rule->redirect_output)
		assert(rule->place_param_target.type == T_FILE); 
	print_command();

	/* Start the process */ 
	assert(k >= 1); 

	map <string, string> mapping;
	mapping.insert(mapping_parameter.begin(), mapping_parameter.end());
	mapping.insert(mapping_variable.begin(), mapping_variable.end());
	mapping_parameter.clear();
	mapping_variable.clear(); 
	const pid_t pid= process.start
		(rule->command->command, 
		 mapping,
		 rule->redirect_output 
		 ? rule->place_param_target.place_param_name.unparametrized() : "",
		 rule->filename_input.unparametrized(),
		 rule->command->place); 
	if (pid < 0) {
		print_traces(fmt("error executing command for %s", target.text())); 
		if (! option_continue) 
			exit(ERROR_SYSTEM); 
		error |= ERROR_BUILD;
		done.add_neg(link.avoid); 
		return k;
	}
	executions_by_pid[pid]= this;
	assert(executions_by_pid.at(pid)->process.started()); 
	assert(pid == executions_by_pid.at(pid)->process.get_pid()); 
	--k;
	assert(k >= 0);

	return k;
}

void Execution::waited(int pid, int status) 
{
	assert(process.started()); 
	assert(process.get_pid() == pid); 
	assert(buffer.empty()); 
	assert(children.size() == 0); 

	assert(done.get_k() == 0);
	done.add_one_neg(0); 

	executions_by_pid.erase(pid); 

	if (process.waited(status, pid)) {
		/* Command was successful */ 

		/* For file targets, check that the file was built */ 
		if (target.type == T_FILE) {

			struct stat buf;

			if (0 == stat(target.name.c_str(), &buf)) {
				exists= +1;
				/* Check that file was not created with modification
				 * time in the future */  
				warn_future_file(&buf); 
				/* Check that file is not older that Stu
				 * startup */ 
				Timestamp timestamp_file(&buf);
				if (timestamp_file.older_than(Timestamp::startup)) {

					/* Check whether the file is actually a symlink, in
					 * which case we ignore that error */ 
					if (0 > lstat(target.name.c_str(), &buf)) {
						perror(target.name.c_str()); 
						error |= ERROR_BUILD;
						if (! option_continue)
							throw error; 
					}
					if (! S_ISLNK(buf.st_mode)) {
						error |= ERROR_BUILD;
						rule->place <<
							fmt("timestamp of file '%s' "
							    "after execution of its command is older than %s startup", 
							    target.name, dollar_zero);  
						print_info(fmt("Timestamp of %s is %s",
							       target.text(), timestamp_file.format()));
						print_info(fmt("Startup timestamp is %s",
							       Timestamp::startup.format())); 
						print_traces();
						if (! option_continue)
							throw error; 
					}
				}
			} else {
				error |= ERROR_BUILD; 
				rule->command->place <<
					fmt("file '%s' was not built by command", 
					    target.name); 
				print_traces();
				exists= -1;
				if (! option_continue)
					throw error; 
			}
		}

	} else {
		/* Command failed */ 

		error |= ERROR_BUILD; 

		string reason;
		if (WIFEXITED(status)) {
			reason= frmt("failed with exit code %d", WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			int sig= WTERMSIG(status);
			reason= frmt("received signal %s", strsignal(sig));
		} else {
			/* This should not happen but the standard does not exclude
			 * it */ 
			reason= frmt("failed with status code %d", status); 
		}

		param_rule->command->place <<
			fmt("command for %s %s", target.text(), reason); 
		print_traces(); 

		remove_if_existing(true); 

		if (! option_continue)
			throw error; 
	}
}

int Execution::execute_main(const vector <Target> &targets, 
			    const vector <Place> &places, 
			    int k)
{
	assert(k >= 0);
	assert(targets.size() == places.size()); 

	timestamp_last= Timestamp::now(); 

	vector <shared_ptr <Dependency> > dependencies;
	
	for (unsigned i= 0;  i != targets.size();  ++i) {
		dependencies.push_back
			(
			 shared_ptr <Dependency>
			 (
			  new Direct_Dependency
			  (0, Place_Param_Target
			   (targets.at(i).type, 
				Place_Param_Name(targets.at(i).name, places.at(i))
				))
			  )
		   ); 
	}

	Execution *execution_root= new Execution(dependencies); 
	int error= 0; 

	try {
		while (! execution_root->finished()) {

			Link link(Stack(), (Flags)0, Place(), shared_ptr <Dependency> ());

			execution_root->execute
			    (nullptr, 
			     k - Execution::executions_by_pid.size(), link);

			if (executions_by_pid.size()) {
				execute_wait();
			}
		}

		assert(execution_root->finished()); 

		bool success= (execution_root->error == 0);

		if (! option_continue)
			assert(success); 

		if (worked && verbosity == VERBOSITY_SHORT) {
			puts("Done");
		}
	
		if (success && ! worked) {
			puts("Nothing to be done"); 
		}

		if (! success && option_continue) {
			fprintf(stderr, "%s: *** Targets not remade because of errors\n", 
					dollar_zero); 
		}

		error= execution_root->error; 
	} 

	/* A build error is only thrown when options_continue is
	 * not set */ 
	catch (int e) {

		assert(! option_continue); 
		assert(e >= 1 && e <= 3); 

		error= e; 
		
		/* Terminate all processes */ 
		if (executions_by_pid.size()) {
			print_error("Terminating all running processes"); 
			process_terminate_all();
		}
	}

	return error;
}

void Execution::unlink_execution(Execution *const parent, 
				 Execution *const child,
				 shared_ptr <Dependency> dependency_parent,
				 Stack avoid_parent,
				 Stack avoid_child,
				 Flags flags_child)
{
	(void) avoid_child;

	assert(parent != child); 
	assert(child->finished(avoid_child)); 

	if (! option_continue)  
		assert(child->error == 0); 

	/*
	 * Propagations
	 */

	/* Propagate dynamic dependencies */ 
	if (flags_child & F_DYNAMIC) {
		assert(child->target.type == T_FILE); 
		assert(T_FILE < parent->target.type);
		assert(parent->target.name == child->target.name); 
		assert(child->done.get_k() == 0); 
		
		bool do_read= true;

		if (child->error != 0) {
			do_read= false;
		}

		/* Don't read the dependencies when the target was optional and
		 * was not built */
		// TODO we may need to use stat() here in case child->exists is
		// zero. 
		else if (flags_child & F_OPTIONAL) {
			if (child->exists != +1) {
				do_read= false;
			}
		}

		if (do_read) {
			parent->read_dynamics(avoid_parent, dependency_parent); 
		}
	}

	/* Propagate timestamp.  Note:  When the parent execution has
	 * filename == "", this is unneccesary */ 
	if (! (flags_child & F_EXISTENCE) && 
		! (flags_child & F_DYNAMIC)) {
		if (child->timestamp.defined()) {
			if (! parent->timestamp.defined()) {
				parent->timestamp= child->timestamp;
			} else {
				if (parent->timestamp.older_than(child->timestamp)) {
					parent->timestamp= child->timestamp; 
				}
			}
		}
	}

	/* Propagate variable dependencies */
	if (flags_child & F_VARIABLE) {

		assert(child->target.type == T_FILE);
		string filename= child->target.name;
		int fd;
		string content;
		size_t filesize;

		struct stat buf;
		fd= open(filename.c_str(), O_RDONLY);
		if (fd < 0) {
			goto error;
		}
		if (0 > fstat(fd, &buf)) {
			goto error_fd;
		}

		filesize= buf.st_size;
		content.resize(filesize);
		if ((ssize_t)filesize != read(fd, (void *) content.c_str(), filesize)) {
			goto error_fd;
		}

		if (0 > close(fd))  
			goto error;

		/* Remove space at beginning and end.  The characters are
		 * exactly those used by isspace() in the C locale. 
		 */ 
		content.erase(0, content.find_first_not_of(" \n\t\f\r\v")); 
		content.erase(content.find_last_not_of(" \n\t\f\r\v") + 1);  

		parent->mapping_variable[filename]= content;

		if (0) {
		error_fd:
			close(fd); 
		error:
			parent->error |= ERROR_BUILD;
			child->param_rule->place_param_target.place <<
				fmt("Generated file '%s' for variable dependency was made "
					"but cannot be found now", 
					filename);
			parent->param_rule->place_param_target.place <<
				"in variable dependency used here";
			if (! option_continue)
				throw parent->error; 
		}
	}

	/*
	 * Propagate variables over phonies without commands and dynamic
	 * targets
	 */
	if (child->target.type >= T_DYNAMIC ||
		(child->target.type == T_PHONY &&
		 child->rule->command == NULL)) {
		parent->mapping_variable.insert
			(child->mapping_variable.begin(), child->mapping_variable.end()); 
	}

	/* 
	 * Propagate attributes
	 */ 

	/* Note:  propagate the flags after propagating other things, since
	 * flags can be changed by the propagations done before. 
	 */

	parent->error |= child->error; 

	if (child->need_build 
		&& ! (flags_child & F_EXISTENCE)
		&& ! (flags_child & F_DYNAMIC)) {
		if (verbosity >= VERBOSITY_VERBOSE) {
			const string text_child= child->rule->place_param_target.text();
			const string text_parent= parent->rule == NULL 
				? "<target without rule>"
				: parent->rule->place_param_target.text();
			fprintf(stderr, "Propagating need_build flag from %s to %s\n", 
					text_child.c_str(), text_parent.c_str());
		}
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
		     const Link &link,
		     Execution *parent)
	:  target(target_),
	   error(0),
	   need_build(false),
	   checked(false),
	   done(dynamic_depth(target_.type), 0),
	   timestamp(Timestamp::UNDEFINED),
	   exists(0)
{
	assert(target.type == T_PHONY || target.type >= T_FILE); 
	assert(parent != NULL); 
	assert(parents.empty()); 

	/* Fill in the rules and their parameters */ 
	if (target.type == T_FILE || target.type == T_PHONY) {
		map <string, string> mapping1; 
		rule= rule_set.get(target, param_rule, mapping1); 
		mapping_parameter= mapping1;
	} else if (target.type >= T_DYNAMIC) {
		/* We must set the rule here, so cycles in the dependency graph
		 * can be detected.  Note however that the rule of dynamic
		 * dependency executions is otherwise not used */ 
		Target target_file(T_FILE, target.name);
		map <string, string> mapping_rule; 
		rule= rule_set.get(target_file, param_rule, mapping_rule); 
	}
	assert((param_rule == NULL) == (rule == NULL)); 

	parents[parent]= link; 
	executions_by_target[target]= this;

	
	if (target.type < T_DYNAMIC && rule != NULL) {
		/* There is a rule for this execution */ 
		for (auto i= rule->dependencies.begin();
		     i != rule->dependencies.end();  ++i) {
			assert((*i)->get_place().type != Place::P_EMPTY); 
			add_dependency(Link(*i));
		}
	} else {
		/* There is no rule */ 

		/* Whether to produce the "rule not found" error */ 
		bool rule_not_found= false;

		if (target.type == T_FILE) {
			if (! (link.flags & F_OPTIONAL)) {
				/* Check that the file is present,
				 * or make it an error */ 
				struct stat buf;
				int ret_stat= stat(target.name.c_str(), &buf);
				if (0 > ret_stat) {
					if (errno != ENOENT) {
						perror(target.name.c_str()); 
						if (! option_continue)
							exit(ERROR_SYSTEM); 
					}
					/* File does not exist and there is no rule for it */ 
					error |= ERROR_BUILD;
					rule_not_found= true;
				} else {
					/* File exists:  Do nothing, and there are no
					 * dependencies to build */  
					if (parent->target.type == T_EMPTY) {
						/* Output this only for top-level targets, and
						 * therefore we don't need traces */ 
						printf("No rule for building '%s', but the file exists\n", 
							   target.name.c_str()); 
					} 
				}
			}
		} else if (target.type == T_PHONY) {
			rule_not_found= true;
		} else {
			assert(target.type >= T_DYNAMIC); 
		}
		
		if (rule_not_found) {
			assert(rule == NULL); 
			print_traces(fmt("no rule to build %s", target.text()));
			error |= ERROR_BUILD;
			if (! option_continue) 
				throw error;  
			/* Even when a rule was not found, the Execution object remains
			 * in memory */  
		}
	}

}

Execution::Execution(const vector <shared_ptr <Dependency> > &dependencies_)
	:  target(T_EMPTY),
	   error(0),
	   need_build(false),
	   checked(false),
	   exists(0)
{
	executions_by_target[target]= this;

	for (auto i= dependencies_.begin(); i != dependencies_.end(); ++i) {
		add_dependency(Link(*i)); 
	}
}

Execution::~Execution()
{
	/* Executions are never deleted (this is a caching mechanism) */ 
	assert(false); 
}

bool Execution::finished(Stack avoid) const
{
	assert(avoid.get_k() == done.get_k());
	assert(done.get_k() == dynamic_depth(target.type));

	Flags to_do_aggregate= 0;
	
	for (unsigned j= 0;  j <= done.get_k();  ++j) {
		to_do_aggregate |= ~done.get(j) & ~avoid.get(j); 
	}

	return (to_do_aggregate & ((1 << F_COUNT) - 1)) == 0; 
}

bool Execution::finished() const 
{
	assert(done.get_k() == dynamic_depth(target.type));

	Flags to_do_aggregate= 0;
	
	for (unsigned j= 0;  j <= done.get_k();  ++j) {
		to_do_aggregate |= ~done.get(j); 
	}

	return (to_do_aggregate & ((1 << F_COUNT) - 1)) == 0; 
}

void Execution::append_dependencies(const vector <Link> &dependencies_)
{
	for (auto i= dependencies_.begin();  i != dependencies_.end();  ++i) {
		add_dependency(*i); 
	}
}

void process_terminate_all() 
{
	for (auto i= Execution::executions_by_pid.begin();
	     i != Execution::executions_by_pid.end();
	     ++i) {

		const pid_t pid= i->first;
		assert(pid > 0); 

		/* Passing (-pid) to kill() kills the whole process
		 * group with PGID (pid).  Since we set each child
		 * process to have its PID as its process group ID,
		 * this kills the child and all its children
		 * (recursively), up to programs that change this PGID
		 * of processes, such as Stu and shells, which have to
		 * kill their children explicitly in their signal
		 * handlers.  */ 
		if (0 > kill(-pid, SIGTERM)) {
			if (errno == ESRCH) {
				/* The child process is a zombie.  This
				 * means the child process has already
				 * terminated but we haven't wait()ed
				 * for it yet. */ 
			} else {
				perror("kill"); 
				/* Note:  Don't call exit() yet; we want all
				 * children to be killed. */ 
			}
		}
	}

	bool single= Execution::executions_by_pid.size() == 1;

	bool terminated= false;

	for (auto i= Execution::executions_by_pid.begin();
	     i != Execution::executions_by_pid.end();
	     ++i) {

		if (i->second->remove_if_existing(single))
			terminated= true; 
	}

	if (! single && terminated)
		fprintf(stderr, "%s: *** Removing partially built files\n",
				dollar_zero); 

	/* Check that all children are terminated */ 
	for (;;) {
		int status;
		int ret= wait(&status); 
		if (ret < 0) {
			/* wait() sets errno to ECHILD when there was no
			 * child to wait for */ 
			if (errno != ECHILD) {
				perror("waitpid"); 
				exit(ERROR_SYSTEM);
			}
			
			return; 
		}
		assert(ret > 0); 
	}
}

string Execution::cycle_string(const Execution *execution)
{
	assert(execution->param_rule); 

	Target target= execution->target; 
	if (target.type >= T_DYNAMIC)
		target.type= T_FILE; 

	const Place_Param_Target &place_param_target= execution->param_rule->place_param_target;

	assert(place_param_target.type == target.type); 

	if (place_param_target.place_param_name.get_n() == 0) {
		string text= place_param_target.place_param_name.unparametrized(); 
		assert(text == target.name); 
		return target.text();
	} else {
		string t= place_param_target.place_param_name.canonical_text();
		Target o(target.type, t);
		return fmt("%s instantiated as %s", 
				   o.text(),
				   target.text()); 
	}
}

shared_ptr <Trace> Execution::cycle_trace(const Execution *child,
					  const Execution *parent)
{
	if (parent->target.type == T_EMPTY)
		return shared_ptr <Trace> ();

	if (parent->target.type == T_DYNAMIC &&
		child->target.type == T_FILE &&
		parent->target.name == child->target.name)
		return shared_ptr <Trace> (); 

	const Link &link= child->parents.at((Execution *)parent); 

	return shared_ptr <Trace> 
		(new Trace(link.place,
				   fmt("%s depends on %s", 
					   cycle_string(parent),
					   cycle_string(child)))); 
}

bool Execution::find_cycle(const Execution *const parent, 
			   const Execution *const child)
{
	/* Happens when the parent is the root execution */ 
	if (parent->param_rule == NULL)
		return false;
		
	/* Happens with files that should be there and have no rule */ 
	if (child->param_rule == NULL)
		return false; 

	vector <Trace> traces;
	shared_ptr <Trace> trace= cycle_trace(child, parent); 
	if (trace != NULL)
		traces.push_back(*trace); 

	return find_cycle(parent, child, traces);
}

bool Execution::find_cycle(const Execution *const parent, 
			   const Execution *const child,
			   vector <Trace> &traces)
{
	assert(parent);
	assert(child); 

	if (parent->target.type == child->target.type &&
		parent->param_rule == child->param_rule) {

		assert(traces.size() >= 1); 

		traces.rbegin()->message= fmt
			("%s: %s",
			 traces.size() == 1 
			 ? "target must not depend on itself"
			 : "cyclic dependency",
			 traces.rbegin()->message); 

		for (auto i= traces.rbegin();  i != traces.rend();  ++i) {
			i->print();
		}

		return true;
	} 

	for (auto i= parent->parents.begin();
		 i != parent->parents.end();  ++i) {

		const Execution *parent_parent= i->first; 
		assert(parent_parent != NULL); 
			
		if (parent_parent->param_rule == NULL)
			continue; 
		
		shared_ptr <Trace> trace_new= cycle_trace(parent, parent_parent);
		if (trace_new != NULL)
			traces.push_back(*trace_new); 
		bool found= find_cycle(parent_parent, child, traces);
		if (trace_new != NULL)
			traces.pop_back(); 

		if (found)
			return true;
	}

	return false; 
}

bool Execution::remove_if_existing(bool output) 
{
	if (target.type != T_FILE)  
		return false;

	bool removed= false;

	const char *filename= target.name.c_str();

	/* Remove the file if it exists.  If it is a symlink, only the
	 * symlink itself is removed, not the file it links to */ 

	struct stat buf;
	if (0 == stat(filename, &buf)) { 

		/* If the file existed before building, remove it only if it now
		 * has a newer timestamp. 
		 */

		if (! timestamp_old.defined() || timestamp_old.older_than(Timestamp(&buf))) {

			if (output) {
				fprintf(stderr, 
					"%s: *** Removing file '%s' because command failed\n",
					dollar_zero,
					filename); 
			}
			
			removed= true;

			if (0 > unlink(filename)) {
				perror(filename); 
			}
		}
	}

	return removed; 
}

void Execution::add_dependency(const Link &dependency) 
{
	buffer.push(dependency); 
}

Execution *Execution::get_execution(const Target &target, 
				    const Link &link,
				    Execution *parent)
{
	/* Set to the returned Execution object when one is found or created
	 */   
	Execution *execution= NULL; 

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
			/* The parent and child are not yet connected -- add the
			 * connection */ 
			execution->parents[parent]= link;
		}
		
	} else { 
		/* Create a new Execution object */ 

		execution= new Execution(target, link, parent);  

		assert(execution->parents.size() == 1); 
	}

	if (find_cycle(parent, execution)) {
		parent->error |= ERROR_LOGICAL;
		if (! option_continue) 
			throw parent->error;
		return NULL;
	}

	execution->initialize(link.avoid); 

	return execution;
}

void Execution::read_dynamics(Stack avoid,
			      shared_ptr <Dependency> dependency_parent)
{
	assert(target.type >= T_DYNAMIC);
	assert(avoid.get_k() == dynamic_depth(target.type)); 

	try {
		vector <shared_ptr <Token> > tokens;
		vector <Trace> traces;
		vector <string> filenames;
		const string filename= target.name; 
		Place place_end; 

		parse(tokens, place_end, filename, false, traces, filenames);
		auto i= tokens.begin(); 
		vector <shared_ptr <Dependency> > dependencies;

		Build build(tokens, i, place_end); 
		Place_Param_Name input; /* remains empty */ 
		Place place_input; /* remains empty */ 
		build.build_dependency_list(dependencies, input, place_input); 

		for (auto j= dependencies.begin();
			 j != dependencies.end();  ++j) {

			/* Check that it is unparametrized */ 
			if (! (*j)->is_unparametrized()) {
				shared_ptr <Dependency> dep= *j;
				while (dynamic_pointer_cast <Dynamic_Dependency> (dep)) {
					shared_ptr <Dynamic_Dependency> dep2= 
						dynamic_pointer_cast <Dynamic_Dependency> (dep);
					dep= dep2->dependency; 
				}
				dynamic_pointer_cast <Direct_Dependency> (dep)
					->place_param_target.place_param_name.places.at(0) <<
					fmt("dynamic dependency %s "
						"must not contain parametrized dependencies",
						target.text());
				Target target_file= target;
				target_file.type= T_FILE;
				print_traces(fmt("%s is declared here", 
								 target_file.text())); 
				error |= ERROR_LOGICAL; 
				if (option_continue) {
					continue; 
				} else {
					throw error; 
				}
			}

			/* Add the found dependencies, with one less dynamic level
			 * than the current target.  */

			/* If the target is multiply dynamic, we cannot add phony
			 * targets to it */ 
			if (dynamic_pointer_cast <Direct_Dependency> (*j)) {
				
				shared_ptr <Direct_Dependency> direct_dependency= 
					dynamic_pointer_cast <Direct_Dependency> (*j); 
				
				if (direct_dependency->place_param_target.type == T_PHONY) {
					if (target.type > T_DYNAMIC) {
						direct_dependency->place_param_target.place <<
							fmt("phony target %s cannot appear "
								"as dynamic dependency of %s", 
								direct_dependency->place_param_target.text(),
								target.text());
						Target target_file= target;
						target_file.type= T_FILE;
						print_traces(fmt("%s is declared here", target_file.text())); 
						error |= ERROR_LOGICAL;
						if (option_continue) {
							continue; 
						} else {
							throw error; 
						}
					}
				}
			}

			shared_ptr <Dependency> dependency(*j);

			vector <shared_ptr <Dynamic_Dependency> > vec;
			shared_ptr <Dependency> p= dependency_parent;
			while (dynamic_pointer_cast <Dynamic_Dependency> (p)) {
				shared_ptr <Dynamic_Dependency> dynamic_dependency= 
					dynamic_pointer_cast <Dynamic_Dependency> (p);
				vec.resize(vec.size() + 1);
				vec.at(vec.size() - 1)= dynamic_dependency;
				p= dynamic_dependency->dependency;   
			}

			Stack avoid_this= avoid;
			assert(vec.size() == avoid_this.get_k());
			avoid_this.pop(); 
			dependency->add_flags(avoid_this.get_lowest()); 
			if (dependency->get_place_existence().type == Place::P_EMPTY)
				dependency->set_place_existence
					(vec.at(target.type - T_DYNAMIC)->get_place_existence()); 
			if (dependency->get_place_optional().type == Place::P_EMPTY)
				dependency->set_place_optional
					(vec.at(target.type - T_DYNAMIC)->get_place_optional()); 
			for (Type k= target.type;  k > T_DYNAMIC;  --k) {
				avoid_this.pop(); 
				Flags flags_level= avoid_this.get_lowest(); 
				dependency= make_shared <Dynamic_Dependency> (flags_level, dependency); 
				dependency->set_place_existence
					(vec.at(k - T_DYNAMIC - 1)->get_place_existence()); 
				dependency->set_place_optional
					(vec.at(k - T_DYNAMIC - 1)->get_place_optional()); 
			}

			assert(avoid_this.get_k() == 0); 

			add_dependency(Link(dependency));

			/* Check that there are no input dependencies */ 
			if (! input.empty()) {
				(*j)->get_place() <<
					fmt("dynamic dependency %s must not contain input redirection", 
						target.text());
				Target target_file= target;
				target_file.type= T_FILE;
				print_traces(fmt("%s is declared here", target_file.text())); 
				error |= ERROR_LOGICAL; 
				if (option_continue) {
					continue; 
				} else {
					throw error; 
				}
			}
		}
				
		if (i != tokens.end()) {
			(*i)->get_place() << "expected dependency";
			throw ERROR_LOGICAL;
		}
	} catch (int e) {
		assert(e >= 1 && e <= 3); 
		error |= e;
		if (! option_continue) {
			throw;
		}
	}
}

void Execution::warn_future_file(struct stat *buf)
{
	assert(target.type == T_FILE); 
	if (timestamp_last.older_than(Timestamp(buf))) {
		print_warning(fmt("'%s' has modification time in the future",
				  target.name.c_str()));
	}
}

void Execution::print_traces(string text) const
{	
	/* The following traverses the execution graph backwards until it
	 * finds the root. We always take the first found parent, which
	 * is an arbitrary choice, but it doesn't matter here *which*
	 * dependency path we point out as an error.  
	 */

	const Execution *execution= this; 

	assert(execution->target.type != T_EMPTY);

	bool first= true; 

	/* If there is a rule for this target, show the message with the
	 * rule's trace, otherwise show the message with the first
	 * dependency trace */ 
	if (execution->param_rule != NULL && text != "") {
		execution->param_rule->place << text;
		first= false;
	}

	string text_parent= execution->target.text(); 

	for (;;) {

		auto i= execution->parents.begin(); 

		if (i->first->target.type == T_EMPTY) {
			if (first && text != "") {
				print_error(fmt("No rule to build %s", 
						execution->target.text())); 
			}
			break; 
		}

		string text_child= text_parent; 
		text_parent= i->first->target.text(); 

		/* Don't show [[A]]->A edges */
		if (i->second.flags & F_DYNAMIC) {
			execution= i->first; 
			continue;
		}

		Place place= i->second.place;

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

void Execution::print_command()
{
	if (verbosity == VERBOSITY_SHORT) {
		string text= target.text_bare();
		puts(text.c_str()); 
		return;
	} 

	if (verbosity < VERBOSITY_SHORT)
		return; 

	/* For single-line commands, show the variables on the same line.
	 * For multi-line commands, show them on a separate line. */ 
	bool single_line= rule->command->get_lines().size() == 1;
	bool begin= true; 

	string filename_output= rule->redirect_output 
		? rule->place_param_target.place_param_name.unparametrized() : "";
	string filename_input= rule->filename_input.unparametrized(); 

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

	/* Print the parameter values.  (Variable assignments are not printed) 
	 */ 
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
	for (auto i= rule->command->get_lines().begin();  
	     i != rule->command->get_lines().end();  ++i) {
		printf("%s\n", i->c_str()); 
	}
}

void Execution::deploy_dependency(int &k,
				  const Link &link,
				  const Link &link_child)
{
	Flags flags_child= link_child.flags; 

	int dynamic_depth= 0;
	shared_ptr <Dependency> dep= link_child.dependency;
	while (dynamic_pointer_cast <Dynamic_Dependency> (dep)) {
		dep= dynamic_pointer_cast <Dynamic_Dependency> (dep)->dependency;
		++dynamic_depth;
	}

	shared_ptr <Direct_Dependency> direct_dependency=
		dynamic_pointer_cast <Direct_Dependency> (dep);
	assert(! direct_dependency->place_param_target.place_param_name.empty()); 

	Target target_child= direct_dependency->place_param_target.unparametrized();
	assert(target_child.type == T_FILE || target_child.type == T_PHONY);

	if (dynamic_depth != 0) {

		assert(dynamic_depth > 0);

		/* Phonies in dynamic dependencies are not allowed */ 
		if (target_child.type == T_PHONY) {
			error |= ERROR_LOGICAL;
			direct_dependency->place <<
				fmt("phony target %s cannot appear as dynamic dependency for target %s", 
					direct_dependency->place_param_target.text(),
					target.text());
			print_traces(); 

			if (! option_continue)
				throw error;
			return;
		}
		target_child.type += dynamic_depth; 
	}

	Stack avoid_child= link_child.avoid;

	/* Flags get carried over phonies */ 
	if (target.type == T_PHONY) { 
		flags_child |= link.flags; 
		avoid_child.add_lowest(link.flags);
		if (link.flags & F_EXISTENCE) {
			link_child.dependency->set_place_existence(link.dependency->get_place_existence()); 
		}
		if (link.flags & F_OPTIONAL) {
			link_child.dependency->set_place_optional(link.dependency->get_place_optional()); 
		}
	}
	
	/* '?' and '!' do not mix */ 
	if ((flags_child & F_EXISTENCE) && 
		(flags_child & F_OPTIONAL)) {

		error |= ERROR_LOGICAL;
		const Place &place_existence= 
			link_child.dependency->get_place_existence();
		const Place &place_optional= 
			link_child.dependency->get_place_optional();
		place_existence <<
			"declaration of existence only dependency with '!'";
		place_optional <<
			"clashes with declaration of optional dependency with '?'";
		direct_dependency->place <<
			fmt("in declaration of dependency %s", 
				target_child.text());
		print_traces();
		if (! option_continue)
			throw error;
		return;
	}

	Execution *child= Execution::get_execution
		(target_child, 
		 Link(avoid_child,
			  flags_child,
			  direct_dependency->place,
			  link_child.dependency),
		 this);  
	if (child == NULL)
		return;

	children.insert(child);

	Link link_child_new(avoid_child, flags_child, link_child.place, link_child.dependency); 

	k= child->execute(this, k, link_child_new);
	assert(k >= 0);
	if (k == 0)  
		return;
			
	if (child->finished(avoid_child)) {
		unlink_execution(this, child, 
				 link.dependency,
				 link.avoid, 
				 avoid_child, flags_child);
	}
}

void Execution::initialize(Stack avoid) 
{
	if (target.type >= T_DYNAMIC) {

		/* This is a special dynamic target.  Add, as an initial
		 * dependency, the corresponding file.  
		 */
		Flags flags_child= avoid.get_lowest() | F_DYNAMIC;

		shared_ptr <Dependency> dependency_child= make_shared <Direct_Dependency>
			(flags_child,
			 Place_Param_Target(T_FILE, Place_Param_Name(target.name)));

		add_dependency(Link(dependency_child, flags_child, Place()));
		/* The place of the [[A]]->A links is empty, meaning it will
		 * not be output in traces. */ 
	} 
}

#endif /* ! EXECUTION_HH */
