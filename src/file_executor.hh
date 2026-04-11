#ifndef FILE_EXECUTOR_HH
#define FILE_EXECUTOR_HH

/*
 * Each non-dynamic file target is represented at run time by one File_Executor object.
 * Each File_Executor object may correspond to multiple files or phonies, when a rule
 * has multiple targets.  Phonies are only represented by a File_Executor when they
 * appear as targets of rules that have at least one file target, or when the rule has a
 * command.  Otherwise, Transitive_Executor is used for them.
 *
 * This is the only Executor subclass that actually starts jobs -- all other Executor
 * subclasses only delegate their tasks to child executors.
 */

class File_Executor
	: public Executor
{
public:
	File_Executor(
		shared_ptr <const Dep> dep_link,
		Executor *parent,
		shared_ptr <const Rule> rule,
		shared_ptr <const Rule> param_rule,
		std::map <string, string> &mapping_parameter_,
		int &error_additional);
	/* ERROR_ADDITIONAL indicates whether an error will be thrown after the
	 * call. (Because an error can only be thrown after the executor has been
	 * connected to a parent, which is not done in the constructor.  The parent is
	 * then connected to this iff ERROR_ADDITIONAL is zero after the call. */

	void read_variable(shared_ptr <const Dep> dep);
	/* Read the content of the file into a string as the variable value.  THIS is the
	 * variable executor.  Write the result into THIS's RESULT_VARIABLE. */

	shared_ptr <const Rule> get_rule() const { return rule; }

	bool remove_if_existing(bool output);
	/* Remove all file targets of this executor object if they exist.  If OUTPUT is
	 * true, output a corresponding message.  Return whether the file was removed.  If
	 * OUTPUT is false, only do async signal-safe things. */
	void print_as_job() const;
	/* Print a line to stdout for a running job, as output of SIGUSR1.
	 * Is currently running. */

	virtual bool want_delete() const override { return false; }
	virtual Proceed execute(shared_ptr <const Dep> dep_link) override;
	virtual bool finished(Flags flags) const override;
	virtual void notify_variable(const std::map <string, string> &) override;

	static void wait();
	/* Wait for next job to finish and finish it.  Do not start anything new. */

#ifndef NDEBUG
	virtual void render(Parts &, Rendering= 0) const override;
#endif /* ! NDEBUG */

protected:
	virtual bool optional_finished(shared_ptr <const Dep> dep_link) override;

private:
	friend class Executor;

	std::vector <Hash_Dep> hash_deps;
	/* The targets to which this executor object corresponds.  Never empty.  All
	 * targets are non-dynamic, i.e., only plain files and phonies are included.
	 * Does not include flags (except F_PHONY). */

	Timestamp *timestamps_old;
	/* Timestamp of each file target, before the command is executed.  Only valid once
	 * the job was started.  The indexes correspond to those in TARGETS.  Non-file
	 * indexes are uninitialized.  Used for checking whether a file was rebuild to
	 * decide whether to remove it after a command failed or was interrupted.  This is
	 * UNDEFINED when the file did not exist, or no target is a file.  Allocated with
	 * malloc().  Length equals that of TARGETS. */

	char **filenames;
	/* The actual filename for every file target.  Null for phonies.  Both the
	 * array and each filename is allocated with malloc().  If used, it has the same
	 * length as TARGETS.  Only set when we are actually starting the jobs.  Cannot be
	 * a C++ container because we access it from async-signal safe functions.  Used to
	 * delete partially-built files. */

	shared_ptr <const Rule> rule;
	/* The instantiated file rule.  Null when there is no rule for this file.
	 * Individual dynamic dependencies do have rules, in order for cycles to be
	 * detected.  Null if and only if PARAM_RULE is null. */

	Job job;

	std::map <string, string> mapping_parameter;
	/* Variable assignments from parameters for when the command is run */

	std::map <string, string> mapping_variable;
	/* Variable assignments from variables dependencies */

	Done done;

	~File_Executor();

	void waited(pid_t pid, size_t index, int status);
	/* Called after the job was waited for.  The PID is only passed
	 * for checking that it is correct.  INDEX is the index within
	 * EXECUTORS_BY_PID_*. */

	void warn_future_file(struct stat *buf, const char *filename,
			      const Place &place,
			      const char *message_extra= nullptr);
	/* Warn when the file has a modification time in the future.
	 * MESSAGE_EXTRA may be null to not show an extra message. */

	void print_command() const;

	bool check_file_target(
		const Hash_Dep &target,
		size_t index,
		shared_ptr <const Dep> dep_link,
		bool no_execution);
	/* Return whether we are done */

	void write_content(const char *filename, const Command &command);
	void check_file_was_built(Hash_Dep hash_dep, const Place &place);
	void check_file_target_without_rule(
		shared_ptr <const Dep> dep,
		Hash_Dep &hash_dep,
		Executor *parent,
		bool &rule_not_found);
	bool start(
		shared_ptr <const Dep> dep_link,
		pid_t &pid,
		const std::map <string, string> &mapping);
	/* Return value is TRUE on error */

	static std::unordered_map <string, Timestamp> phonies;
	/* The timestamps for phony targets.  This container plays the role of the file
	 * system for phony targets, holding their timestamps, and remembering whether
	 * they have been executed.  Note that if a rule has both file targets and phony
	 * targets, and all file targets are up to date and the phony targets have all
	 * their dependencies up to date, then the command is not executed, even though it
	 * was never executed in the current invocation of Stu. In that case, the phony
	 * targets are never inserted in this map.  */
};

#endif /* ! FILE_EXECUTOR_HH */
