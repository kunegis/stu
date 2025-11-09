#include "file_executor.hh"

#include "signal.hh"

size_t File_Executor::executors_by_pid_size= 0;
pid_t *File_Executor::executors_by_pid_key= nullptr;
File_Executor **File_Executor::executors_by_pid_value= nullptr;
std::unordered_map <string, Timestamp> File_Executor::transients;

File_Executor::File_Executor(
	shared_ptr <const Dep> dep,
	Executor *parent,
	shared_ptr <const Rule> rule_,
	shared_ptr <const Rule> param_rule_,
	std::map <string, string> &mapping_parameter_,
	int &error_additional)
	: Executor(param_rule_), timestamps_old(nullptr), filenames(nullptr),
	  rule(rule_)
{
	TRACE_FUNCTION();
	assert((param_rule_ == nullptr) == (rule_ == nullptr));

	swap(mapping_parameter, mapping_parameter_);
	Hash_Dep hash_dep_= dep->get_target();

	/* Later replaced with all targets from the rule, if a rule exists */
	Hash_Dep hash_dep_no_flags= hash_dep_;
	hash_dep_no_flags.get_front_word_nondynamic() &= F_TARGET_TRANSIENT;
	hash_deps.push_back(hash_dep_no_flags);
	executors_by_hash_dep[hash_dep_no_flags]= this;

	parents[parent]= dep;
	if (error_additional) {
		TRACE("Additional error");
		*this << "";
		done.set_all();
		parents.erase(parent);
		raise(error_additional);
		return;
	}

	if (rule == nullptr) {
		TRACE("TARGETS contains only DEPENDENCY->TARGET");
	} else {
		hash_deps.clear();
		for (auto &place_param_target: rule->place_targets) {
			hash_deps.push_back(place_param_target->unparametrized());
		}
		assert(hash_deps.size());
	}

	/* Fill EXECUTORS_BY_TARGET with all targets from the rule, not just the one given
	 * in the dependency. */
	for (const Hash_Dep &hash_dep: hash_deps) {
		executors_by_hash_dep[hash_dep]= this;
	}

	if (rule != nullptr) {
		TRACE("There is a rule for this executor");
		for (auto &d: rule->deps) {
			push(d);
		}
	} else {
		TRACE("There is no rule for this executor");

		bool rule_not_found= false;
		/* Whether to produce the "no rule to build target" error */

		assert(hash_dep_.is_file());
		check_file_target_without_rule(
			dep, hash_dep_, parent, rule_not_found);

		if (rule_not_found) {
			TRACE("Rule not found");
			assert(rule == nullptr);
			*this << fmt("no rule to build %s", show(hash_dep_));
			error_additional |= error |= ERROR_BUILD;
			raise(ERROR_BUILD);
			return;
		}
	}

	/* It is not allowed to have a dynamic of a non-transitive transient */
	if (dynamic_cast <Dynamic_Executor *> (parent) &&
	    dep->flags & F_RESULT_NOTIFY &&
	    dep->flags & F_TARGET_TRANSIENT) {

		Place place_target;
		for (auto &i: rule->place_targets) {
			if (i->place_name.unparametrized() == hash_dep_.get_name_nondynamic()) {
				place_target= i->place;
				break;
			}
		}
		assert(! place_target.empty());
		if (rule->command) {
			place_target <<
				fmt("rule for transient target %s must not have a command",
				    show(hash_dep_));
		} else {
			place_target <<
				fmt("rule for transient target %s must not have file targets",
				    show(hash_dep_));
		}
		dep->get_place() << fmt("when used as dynamic dependency of %s",
					show(parent->get_parents().begin()->second));
		*(parent->get_parents().begin()->first) << "";
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}

	/* -o and -p are not allowed on non-transitive transients */
	if (dep->flags & F_TARGET_TRANSIENT &&
	    dep->flags & (F_OPTIONAL | F_PERSISTENT)) {

		Place place_target;
		for (auto &i: rule->place_targets) {
			if (i->place_name.unparametrized() == hash_dep_.get_name_nondynamic()) {
				place_target= i->place;
				break;
			}
		}
		assert(! place_target.empty());
		unsigned ind= dep->flags & F_OPTIONAL ? I_OPTIONAL : I_PERSISTENT;
		dep->get_place() << fmt((dep->flags & F_OPTIONAL)
					? "dependency %s must not be declared as optional"
					: "dependency %s must not be declared as persistent",
					show(dep));
		dep->places[ind] <<
			fmt("using flag %s", show_prefix("-", frmt("%c", flags_chars[ind])));
		if (rule->command)
			place_target <<
				fmt("because rule for transient target %s has a command",
				    show(hash_dep_));
		else
			place_target <<
				fmt("because rule for transient target %s has file targets",
				    show(hash_dep_));
		*this << "";
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}

	parents.erase(parent);
	if (find_cycle(parent, this, dep)) {
		TRACE("Rule-level but not file-level cycle found");
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}
	parents[parent]= dep;
}

File_Executor::~File_Executor()
/* Objects of this type are never deleted */
{
	unreachable();
#if 0
	/* We put this here as a reminder if this is ever activated */
	free(timestamps_old);
	if (filenames) {
		for (size_t i= 0; i < targets.size(); ++i)
			if (filenames[i])
				free(filenames[i]);
		free(filenames);
	}
#endif /* 0 */
}

void File_Executor::wait()
/* We wait for a single job to finish, and then return so that the next job can be
 * started.  It would also be possible to process as many finished jobs as possible, and
 * then return, but the current implementation prefers to first start the next job before
 * waiting for the next finished job. */
{
	Debug::print(nullptr, "wait...");

	assert(File_Executor::executors_by_pid_size);

	int status;
	const pid_t pid= Job::wait(&status);

	Debug::print(nullptr, frmt("waited for pid = %jd", (intmax_t)pid));

	timestamp_last= Timestamp::now();

	size_t index;
	bool r= executors_find(pid, index);
	if (!r) {
		should_not_happen();
		print_warning(Place(),
			frmt("the function waitpid(2) returned the invalid process ID %jd",
				(intmax_t)pid));
		return;
	}

	File_Executor *executor= executors_by_pid_value[index];
	executor->waited(pid, index, status);
	++options_jobs;
}

void File_Executor::waited(pid_t pid, size_t index, int status)
{
	assert(job.started());
	assert(job.get_pid() == pid);

	Executor::check_waited();
	done.set_all();

	{
		Signal_Blocker sb;
		executors_remove(index);
	}

	/* The file(s) may have been built, so forget that it was known to not exist */
	bits &= ~B_MISSING;

	if (job.waited(status, pid)) {
		bits |=  B_EXISTING;
		bits &= ~B_MISSING;
		/* Subsequently set to B_MISSING if at least one target file is missing */

		/* Check that the file targets were built */
		for (size_t i= 0; i < hash_deps.size(); ++i) {
			const Hash_Dep hash_dep= hash_deps[i];
			if (! hash_dep.is_file()) continue;
			check_file_was_built(hash_dep, rule->place_targets[i]);
		}
		/* In parallel mode, print "done" message */
		if (option_parallel && !option_s) {
			string text= show(hash_deps[0], S_NORMAL);
			printf("Successfully built %s\n", text.c_str());
		}
	} else {
		/* Command failed */
		string reason;
		if (WIFEXITED(status)) {
			reason= fmt("failed with exit status %s",
				show_operator(frmt("%d", WEXITSTATUS(status))));
		} else if (WIFSIGNALED(status)) {
			int sig= WTERMSIG(status);
			reason= frmt("received signal %d (%s%s%s)",
				     sig,
				     Color::highlight_on[CH_ERR],
				     strsignal(sig),
				     Color::highlight_off[CH_ERR]);
		} else {
			/* This should not happen but the standard does not exclude it */
			should_not_happen();
			reason= frmt("failed with status %s%d%s",
				     Color::highlight_on[CH_ERR],
				     status,
				     Color::highlight_off[CH_ERR]);
		}

		if (! param_rule->is_copy) {
			Hash_Dep hash_dep= parents.begin()->second->get_target();
			param_rule->command->place <<
				fmt("command for %s %s",
				    show(hash_dep), reason);
		} else {
			/* Copy rule */
			param_rule->place <<
				fmt("cp to %s %s", show(hash_deps.front()), reason);
		}

		*this << "";
		remove_if_existing(true);
		raise(ERROR_BUILD);
	}
}

bool File_Executor::finished(Flags flags) const
{
	TRACE_FUNCTION(show(hash_deps[0]));
	TRACE("flags= %s", show(flags));
	TRACE("done= %s", done.show());
	bool ret= done.is_done_from_flags(flags);
	TRACE("ret= %s", frmt("%d", ret));
	return ret;
}

void File_Executor::render(Parts &parts, Rendering rendering) const
{
	assert(hash_deps.size());
	hash_deps.front().render(parts, rendering);
}

void File_Executor::notify_variable(
	const std::map <string, string> &result_variable_child)
{
	mapping_variable.insert(
		result_variable_child.begin(),
		result_variable_child.end());
}

void print_jobs()
{
	for (size_t i= 0; i < File_Executor::executors_by_pid_size; ++i)
		File_Executor::executors_by_pid_value[i]->print_as_job();
}

bool File_Executor::remove_if_existing(bool output)
{
	/* [ASYNC-SIGNAL-SAFE] We use only async signal-safe functions here, if OUTPUT is
	 * false */

	if (option_K)
		return false;

	/* Whether anything was removed */
	bool removed= false;

	for (size_t i= 0; i < hash_deps.size(); ++i) {
		const char *filename= filenames[i];
		if (!filename)
			continue;
		assert_async(filename[0] != '\0');

		/* Remove the file if it exists.  If it is a symlink, only the symlink
		 * itself is removed, not the file it links to. */
		struct stat buf;
		if (stat(filename, &buf))
			continue;

		/* If the file existed before building, remove it only if it now has a
		 * newer timestamp. */
		if (! (! timestamps_old[i].defined()
				|| timestamps_old[i] < Timestamp(&buf)))
			continue;

		if (output) {
			string text_filename= ::show(filename, S_DEBUG);
			DEBUG_PRINT(fmt("remove %s", text_filename));
			print_error_reminder(fmt("removing file %s because command failed",
						 ::show(filename)));
		}
		removed= true;

		if (0 > unlink(filename)) {
			if (output) {
				rule->place << format_errno("unlink", filename);
			} else {
				write_async(2, "stu: error: unlink\n");
			}
		}
	}

	return removed;
}

void File_Executor::warn_future_file(
	struct stat *buf,
	const char *filename,
	const Place &place,
	const char *message_extra)
{
	TRACE_FUNCTION();
	TRACE("filename= '%s'", filename);
	TRACE("buf.st_mtime= %s", frmt("%jd", (intmax_t)buf->st_mtime));
	TRACE("now= %s", frmt("%jd", (intmax_t)time(nullptr)));
	Timestamp timestamp_buf= Timestamp(buf);

	if (! (timestamp_last < timestamp_buf))
		return;

	/* Update TIMESTAMP_LAST and check again, to be really-really sure */
	timestamp_last= Timestamp::now();
	if (! (timestamp_last < timestamp_buf))
		return;

	string suffix= message_extra ? string(" ") + message_extra : "";
	print_warning(place, fmt("file %s has modification time in the future%s",
				 ::show(filename), suffix));
}

void File_Executor::print_command() const
{
	constexpr size_t size_max_print_content= 20;
	if (option_s)
		return;

	if (rule->is_hardcode) {
		assert(hash_deps.size() == 1);
		string content= rule->command->command;
		bool is_printable= false;
		if (content.size() < size_max_print_content) {
			is_printable= true;
			for (const char c: content) {
				int cc= c;
				if (! (cc >= ' ' && c <= '~'))
					is_printable= false;
			}
		}
		string text= show(hash_deps.front(), S_NORMAL);
		if (is_printable) {
			string content_text= ::show(content, CH_OUT);
			printf("Creating %s: %s\n", text.c_str(), content_text.c_str());
		} else {
			printf("Creating %s\n", text.c_str());
		}
		return;
	}

	if (rule->is_copy) {
		assert(rule->place_targets.size() == 1);
		string cp_target= show(rule->place_targets[0]->place_name, S_NORMAL);
		string cp_source= show(rule->filename, S_NORMAL);
		printf("cp %s %s\n", cp_source.c_str(), cp_target.c_str());
		return;
	}

	/* We are printing a regular command */

	bool single_line= rule->command->get_lines().size() == 1;

	if (! single_line || option_parallel) {
		string text= show(hash_deps.front(), S_NORMAL);
		printf("Building %s\n", text.c_str());
		return;
	}

	if (option_x)
		return;

	bool begin= true;
	/* For single-line commands, show the variables on the same line.
	 * For multi-line commands, show them on a separate line. */

	string filename_output= rule->redirect_index < 0 ? "" :
		rule->place_targets[rule->redirect_index]
		->place_name.unparametrized();
	string filename_input= rule->filename.unparametrized();

	/* Redirections */
	if (! filename_output.empty()) {
		assert(begin);
		begin= false;
		printf(">%s", filename_output.c_str());
	}
	if (! filename_input.empty()) {
		if (! begin)
			putchar(' ');
		begin= false;
		printf("<%s", filename_input.c_str());
	}

	/* Print the parameter values (variable assignments are not printed) */
	for (auto i= mapping_parameter.begin(); i != mapping_parameter.end(); ++i) {
		string name= i->first;
		string value= i->second;
		if (! begin)
			putchar(' ');
		begin= false;
		printf("%s=%s", name.c_str(), value.c_str());
	}

	/* Colon */
	if (! begin) {
		assert(single_line);
		fputs(": ", stdout);
	}

	/* The command itself */
	for (auto &i: rule->command->get_lines()) {
		puts(i.c_str());
	}
}

Proceed File_Executor::execute(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION(show_trace(dep_link));
	Debug debug(this);

	std::map <string, string> mapping;
	bool no_execution;
	char **filenames_new;
	Timestamp *timestamps_old_new;

	assert(! job.started() || children.empty());

	Proceed proceed_A= execute_phase_A(dep_link), proceed_B;
	TRACE("proceed_A= %s", show(proceed_A));
	assert(is_valid(proceed_A));
	if (proceed_A & (P_WAIT | P_CALL_AGAIN)) {
		return proceed_A;
	}
	assert(proceed_A == P_FINISHED);
	if (error) {
		done |= Done::from_flags(dep_link->flags);
		return P_FINISHED;
	}

	if (finished(dep_link->flags)) {
		return P_FINISHED;
	}

	/* Job has already been started */
	if (job.started_or_waited()) {
		return P_WAIT;
	}

	/* The file must now be built */

	assert(! hash_deps.empty());
	assert(! hash_deps.front().is_dynamic());
	assert(! hash_deps.back().is_dynamic());
	assert(get_buffer_A().empty());
	assert(children.empty());
	assert(error == 0);

	/*
	 * Check whether executor has to be built
	 */
	cov_tag("File_Executor::execute");
	timestamps_old_new= (Timestamp *)
		realloc(timestamps_old, sizeof(timestamps_old[0]) * hash_deps.size());
	if (!timestamps_old_new) {
		print_errno("realloc");
		error_exit();
	}
	timestamps_old= timestamps_old_new;
	for (size_t i= 0; i < hash_deps.size(); ++i)
		timestamps_old[i]= Timestamp::UNDEFINED;
	cov_tag("File_Executor::execute::1");
	filenames_new= (char **)realloc(filenames,
		sizeof(filenames[0]) * hash_deps.size());
	if (!filenames_new) {
		print_errno("realloc");
		error_exit();
	}
	filenames= filenames_new;
	for (size_t i= 0; i < hash_deps.size(); ++i) {
		TRACE("hash_deps[i]= %s", show(hash_deps[i]));
		if (hash_deps[i].is_file()) {
			TRACE("Is file");
			filenames[i]= strdup(hash_deps[i].get_name_c_str_nondynamic());
			if (!filenames[i]) {
				print_errno("strdup");
				error_exit();
			}
		} else {
			filenames[i]= nullptr;
		}
	}

	/* A target for which no execution has to be done */
	no_execution= rule != nullptr && rule->command == nullptr && ! rule->is_copy;
	TRACE("no_execution= %s", frmt("%d", no_execution));

	if (! (bits & B_CHECKED)) {
		bits |= B_CHECKED;
		bits |= B_EXISTING;
		bits &= ~B_MISSING;
		/* Now, set to B_MISSING when a file is found not to exist */

		for (size_t i= 0; i < hash_deps.size(); ++i) {
			const Hash_Dep &hash_dep= hash_deps[i];
			if (! hash_dep.is_file())
				continue;
			TRACE("Check filename= '%s'", hash_dep.get_name_c_str_nondynamic());

			/* We save the return value of stat() and handle errors later */
			struct stat buf;
			int ret_stat= stat(hash_dep.get_name_c_str_nondynamic(), &buf);
			int errno_stat= errno;
			TRACE("stat ret= %s, errno= %s", frmt("%d", ret_stat), frmt("%d", errno_stat));

			/* Warn when file has timestamp in the future */
			if (ret_stat == 0) {
				/* File exists */
				Timestamp timestamp_file= Timestamp(&buf);
				timestamps_old[i]= timestamp_file;
				if (! (dep_link->flags & F_PERSISTENT))
					warn_future_file
						(&buf,
						 hash_dep.get_name_c_str_nondynamic(),
						 rule == nullptr
						 ? parents.begin()->second->get_place()
						 : rule->place_targets[i]->place);
				/* EXISTS is not changed */
			} else {
				bits |= B_MISSING;
				bits &= ~B_EXISTING;
			}

			if (! (bits & B_NEED_BUILD)
			    && ret_stat == 0
			    && timestamp.defined() && timestamps_old[i] < timestamp
			    && ! no_execution)
			{
				bits |= B_NEED_BUILD;
			}

			if (ret_stat == 0) {
				assert(timestamps_old[i].defined());
				if (timestamp.defined() &&
				    timestamps_old[i] < timestamp &&
				    no_execution)
				{
					print_warning
						(rule->place_targets[i]->place,
						 fmt("file target %s which has no command is older than its dependency",
						     show(hash_dep)));
				}
			}

			if (! (bits & B_NEED_BUILD) && ret_stat != 0 && errno_stat == ENOENT) {
				/* File does not exist */
				if (! (dep_link->flags & F_OPTIONAL)) {
					/* Non-optional dependency */
					bits |= B_NEED_BUILD;
				} else {
					unreachable();
					/* In this case, execute_phase_A() will already
					 * have returned P_FINISHED. */
				}
			}

			if (ret_stat != 0 && errno_stat != ENOENT) {
				TRACE("stat() returned an actual error");
				rule->place_targets[i]->place
					<< format_errno("stat",
						hash_dep.get_name_c_str_nondynamic());
				raise(ERROR_BUILD);
				done |= Done::from_flags(dep_link->flags);
				return P_FINISHED;
			}

			/* File does not exist, all its dependencies are up to
			 * date, and the file has no commands: that's an error */
			if (ret_stat != 0 && no_execution) {
				TRACE("File doesn't exist, all dependencies are up to date, and file has no command");
				assert(errno == ENOENT);
				if (rule->deps.size()) {
					*this <<
						fmt("expected the file without command %s to exist because all its dependencies are up to date, but it does not",
						    show(hash_dep));
					explain_file_without_command_with_dependencies();
				} else {
					rule->place_targets[i]->place
						<< fmt("expected the file without command and without dependencies %s to exist, but it does not",
						       show(hash_dep));
					*this << "";
					explain_file_without_command_without_dependencies();
				}
				done |= Done::from_flags(dep_link->flags);
				raise(ERROR_BUILD);
				return P_FINISHED;
			}
		}
		/* We cannot update TIMESTAMP within the loop above
		 * because we need to compare each TIMESTAMP_OLD with
		 * the previous value of TIMESTAMP. */
		for (size_t i= 0; i < hash_deps.size(); ++i) {
			if (timestamps_old[i].defined() &&
			    (! timestamp.defined() || timestamp < timestamps_old[i])) {
				timestamp= timestamps_old[i];
			}
		}
	}

	if (! (bits & B_NEED_BUILD)) {
		bool has_file= false; /* One of the targets is a file */
		for (const Hash_Dep &hash_dep: hash_deps) {
			if (hash_dep.is_file()) {
				has_file= true;
				break;
			}
		}
		for (const Hash_Dep &hash_dep: hash_deps) {
			if (! hash_dep.is_transient())
				continue;
			if (transients.count(hash_dep.get_name_nondynamic()) == 0) {
				/* Transient was not yet executed */
				if (! no_execution && ! has_file) {
					bits |= B_NEED_BUILD;
				}
				break;
			}
		}
	}

	if (! (bits & B_NEED_BUILD)) {
		TRACE("No need to build");
		done |= Done::from_flags_trivial_and_nontrivial(dep_link->flags);
		return P_FINISHED;
	}

	/* We now know that the command must be run, or that there is no command. */

	/* Second pass to execute also all trivial targets */
	proceed_B= execute_phase_B(dep_link);
	assert(is_valid(proceed_B));
	if (proceed_B & (P_WAIT | P_CALL_AGAIN)) {
		return proceed_B;
	}
	assert(children.empty());

	if (no_execution) {
		/* A target without a command:  Nothing to do anymore */
		done |= Done::from_flags(dep_link->flags);
		return P_FINISHED;
	}

	/* The command must be run (or the file created, etc.) now */

	if (option_q) {
		print_error_silenceable("Targets are not up to date");
		exit(ERROR_BUILD);
	}

	out_message_done= true;

	assert(options_jobs > 0);

	/* For hardcoded rules (i.e., static content), we don't need to start a
	 * job, and therefore this is executed even if JOBS is zero. */
	if (rule->is_hardcode) {
		assert(hash_deps.size() == 1);
		assert(hash_deps.front().is_file());
		TRACE("%s", "Create content");
		DEBUG_PRINT("create_content");
		print_command();
		write_content(hash_deps.front().get_name_c_str_nondynamic(),
			*(rule->command));
		done.set_all();
		return P_FINISHED;
	}

	/* We have to start a job now */
	print_command();
	for (const Hash_Dep &hash_dep: hash_deps) {
		if (! hash_dep.is_transient())
			continue;
		Timestamp timestamp_now= Timestamp::now();
		assert(timestamp_now.defined());
		assert(transients.count(hash_dep.get_name_nondynamic()) == 0);
		transients[hash_dep.get_name_nondynamic()]= timestamp_now;
	}
	if (rule->redirect_index >= 0)
		assert(! (rule->place_targets[rule->redirect_index]->flags & F_TARGET_TRANSIENT));
	assert(options_jobs > 0);

	/* Key/value pairs for all environment variables of the job.  Variables override
	 * parameters.  Note about C++ map::insert():  The insert function is a no-op if a
	 * key is already present.  Thus, we insert variables first (because they have
	 * priority). */
	mapping.insert(mapping_variable.begin(), mapping_variable.end());
	mapping.insert(mapping_parameter.begin(), mapping_parameter.end());
	mapping_parameter.clear();
	mapping_variable.clear();

	pid_t pid;
	size_t index; /* In EXECUTORS_BY_PID_* */
	{
		/* Block signals from the time the process is started,
		 * to after we have entered it in the map.  Note:  if we
		 * only blocked signals during the time we update
		 * EXECUTORS_BY_PID_*, there would be a race condition
		 * in which the job would failed to be clean up. */
		Signal_Blocker sb;

		if (start(dep_link, pid, mapping))
			return P_FINISHED;
		TRACE("pid= %s", frmt("%jd", (intmax_t)pid));
		assert(pid != 0 && pid != 1);
		DEBUG_PRINT(frmt("execute: pid = %jd", (intmax_t) pid));

		if (pid < 0) {
			/* Starting the job failed */
			*this << fmt("error executing command for %s",
				     show(hash_deps.front()));
			raise(ERROR_BUILD);
			done |= Done::from_flags(dep_link->flags);
			return P_FINISHED;
		}

		executors_add(pid, index, this);
	}

	assert(executors_by_pid_value[index]->job.started());
	assert(pid == executors_by_pid_value[index]->job.get_pid());
	--options_jobs;
	assert(options_jobs >= 0);

	Proceed proceed_ret= P_WAIT;
	if (order == Order::RANDOM && options_jobs > 0)
		proceed_ret |= P_CALL_AGAIN;
	TRACE("proceed_ret= %s", show(proceed_ret));
	return proceed_ret;
}

void File_Executor::print_as_job() const
{
	pid_t pid= job.get_pid();
	string text_target= show(hash_deps.front(), S_NORMAL);
	printf("%9jd %s\n", (intmax_t) pid, text_target.c_str());
}

void File_Executor::write_content(
	const char *filename,
	const Command &command)
{
	TRACE_FUNCTION();
	FILE *file= fopen(filename, "w");

	if (file == nullptr) {
		rule->place << format_errno("fopen", filename);
		goto error;
	}

	for (const string &line: command.get_lines()) {
		TRACE("line= '%s'", line);
		if (fwrite(line.c_str(), line.size(), 1, file) != 1) {
			TRACE("fwrite failed");
			assert(ferror(file));
			fclose(file);
			rule->place << format_errno("fwrite", filename);
			goto remove;
		}
		if (EOF == fputc('\n', file)) {
			fclose(file);
			rule->place << format_errno("putc", filename);
			goto remove;
		}
	}

	if (0 != fclose(file)) {
		rule->place << format_errno("fclose", filename);
		command.get_place() << fmt("error creating %s", ::show(filename));
		goto remove;
	}

	bits |= B_EXISTING;
	bits &= ~B_MISSING;
	return;

 remove:
	if (remove(filename)) {
		TRACE("remove failed");
		rule->place << format_errno("remove", filename);
	}
 error:
	raise(ERROR_BUILD);
	bits &= ~B_EXISTING;
	bits |= B_MISSING;
}

void File_Executor::read_variable(shared_ptr <const Dep> dep)
{
	TRACE_FUNCTION();
	TRACE("dep= %s", show(dep));
	TRACE("result_variable.size()= %s", frmt("%zu", result_variable.size()));
	TRACE("error= %s", frmt("%d", error));
	TRACE("bits= %s", show_bits(bits));
	DEBUG_PRINT(fmt("read_variable %s", show(dep, S_DEBUG, R_SHOW_FLAGS)));
	assert(to <Plain_Dep> (dep));
	if (! result_variable.empty())
		return;
	if (error)
		return;

	Hash_Dep hash_dep= dep->get_target();
	assert(! hash_dep.is_dynamic());
	const char *filename= hash_dep.get_name_c_str_nondynamic();
	size_t filesize;
	struct stat buf;
	string dependency_variable_name;
	string content;
	constexpr const char *whitespace= " \n\t\f\r\v";

	FILE *file= fopen(filename, "r");
	if (!file) {
		if (errno == ENOENT) {
			Hash_Dep hash_dep_variable=
				to <Plain_Dep> (dep)->place_target.unparametrized();
			if (rule == nullptr) {
				dep->get_place() <<
					fmt("file %s was up to date but cannot be found now",
						show(hash_dep_variable));
			} else {
				for (auto const &place_param_target: rule->place_targets) {
					if (place_param_target->unparametrized()
						== hash_dep_variable) {
						place_param_target->place <<
							fmt("generated file %s was built but cannot be found now",
								show(*place_param_target));
						break;
					}
				}
			}
			*this << "";
		} else {
			print_errno("fopen", filename);
			*this << "";
		}
		goto error;
	}
	if (fstat(fileno(file), &buf) < 0) {
		print_errno("fstat", filename);
		*this << "";
		goto error_close;
	}

	filesize= buf.st_size;
	content.resize(filesize);
	if (filesize && fread(content.data(), filesize, 1, file) != 1) {
		print_errno("fread", filename);
		*this << "";
		goto error_close;
	}

	if (fclose(file)) {
		print_errno("fclose", filename);
		*this << "";
		goto error;
	}

	/* Remove space at beginning and end of the content. The characters are those used
	 * by isspace() in the C locale. */
	content.erase(0, content.find_first_not_of(whitespace));
	content.erase(content.find_last_not_of(whitespace) + 1);

	/* The variable name */
	dependency_variable_name= to <Plain_Dep> (dep)->variable_name;
	{
		string variable_name=
			dependency_variable_name.empty() ?
			hash_dep.get_name_nondynamic() : dependency_variable_name;
		result_variable[variable_name]= content;
	}
	return;

 error_close:
	fclose(file);
 error:
	raise(ERROR_BUILD);
}

bool File_Executor::optional_finished(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION();
	TRACE("dep_link= %s", show_trace(dep_link));

	if ((dep_link->flags & F_OPTIONAL) && to <Plain_Dep> (dep_link)
		&& ! (to <Plain_Dep> (dep_link)->place_target.flags & F_TARGET_TRANSIENT))
	{
		TRACE("Is optional file target");

		if (bits & B_MISSING) {
			TRACE("We already know a file to be missing");
			done |= Done::from_flags(dep_link->flags);
			return true;
		}
		TRACE("File not known to be missing");

		const char *name= to <Plain_Dep> (dep_link)
			->place_target.place_name.unparametrized().c_str();
		TRACE("name= '%s'", name);

		struct stat buf;
		int ret_stat= stat(name, &buf);
		TRACE("ret_stat= %s", frmt("%d", ret_stat));
		if (ret_stat < 0) {
			TRACE("Stat failed");
			bits |= B_MISSING;
			bits &= ~B_EXISTING;
			if (errno != ENOENT) {
				TRACE("Stat failed with error other than ENOENT");
				to <Plain_Dep> (dep_link)->place_target.place <<
					format_errno("stat", name);
				raise(ERROR_BUILD);
				done |= Done::from_flags(dep_link->flags);
				return true;
			}
			TRACE("File is missing; consider it done");
			done |= Done::from_flags(dep_link->flags);
			return true;
		} else {
			TRACE("File exists");
			assert(ret_stat == 0);
			bits |= B_EXISTING;
			bits &= ~B_MISSING;
		}
	}

	return false;
}

void File_Executor::executors_add(pid_t pid, size_t &index, File_Executor *executor)
{
	assert(Signal_Blocker::is_blocked());
	assert(!executors_by_pid_key == !executors_by_pid_value);

	if (!executors_by_pid_key) {
		/* This is executed just once, before we have executed any job, and
		 * therefore JOBS is the value passed via -j (or its default value 1), and
		 * thus we can allocate arrays of that size once and for all. */
		if ((uintmax_t)SIZE_MAX / sizeof(*executors_by_pid_key) < (uintmax_t)options_jobs ||
			(uintmax_t)SIZE_MAX / sizeof(*executors_by_pid_value) < (uintmax_t)options_jobs)
		{
			happens_only_on_certain_platforms();
			/* This can only happen when long is at least as large as size_t,
			 * which is not the case on any commonly used platform, but is
			 * allowed by ISO C++ and POSIX. */
			errno= ENOMEM;
			print_errno(frmt("Value too large for option -j, maximum value is %ju",
					(uintmax_t)SIZE_MAX / std::max(sizeof(*executors_by_pid_key), sizeof(*executors_by_pid_value))));
			error_exit();
		}
		cov_tag("File_Executors::executors_add");
		executors_by_pid_key  = (pid_t *)
			malloc(options_jobs * sizeof(*executors_by_pid_key));
		executors_by_pid_value= (File_Executor **)
			malloc(options_jobs * sizeof(*executors_by_pid_value));
		if (!executors_by_pid_key || !executors_by_pid_value) {
			print_errno("malloc");
			error_exit();
		}
	}

	size_t index_new= executors_by_pid_size++;
	while (index_new && executors_by_pid_key[index_new - 1] > pid) {
		happens_only_on_certain_platforms();
		/* These lines are only executed if a PID returned by fork() is smaller
		 * than a previous PID returned by fork().  This is not the case on the OS
		 * we use for testing.  We tried mockind PID-related syscalls, but
		 * execve() called getpid() with different value of PRELOAD.cc's static
		 * variables, making it impossible to share a PID translation table.  As a
		 * result, we mark this as not tested. */
		executors_by_pid_key[index_new]= executors_by_pid_key[index_new - 1];
		executors_by_pid_value[index_new]= executors_by_pid_value[index_new - 1];
		--index_new;
	}
	executors_by_pid_key[index_new]= pid;
	executors_by_pid_value[index_new]= executor;
	index= index_new;
}

bool File_Executor::executors_find(pid_t pid, size_t &index)
{
	size_t mi= 0, ma= executors_by_pid_size - 1;
	/* Both are inclusive */

	assert(mi <= ma);
	while (mi < ma) {
		size_t ne= mi + (ma - mi + 1) / 2;
		assert(ne <= ma);
		if (executors_by_pid_key[ne] == pid) {
			mi= ma= ne;
			break;
		}
		if (executors_by_pid_key[ne] < pid) {
			mi= ne + 1;
		} else {
			ma= ne - 1;
		}
	}
	if (mi > ma || mi == SIZE_MAX) {
		/* No File_Executor is registered for the PID that just finished.  Should
		 * not happen, but since the PID value came from outside this process, we
		 * better handle this case gracefully, i.e., do nothing. */
		should_not_happen();
		print_warning(Place(),
			      frmt("the function waitpid(2) returned the invalid process ID %jd",
				   (intmax_t)pid));
		return false;
	}

	assert(mi == ma);
	index= mi;
	assert(index < executors_by_pid_size);
	assert(executors_by_pid_key[index] == pid);
	return true;
}

void File_Executor::executors_remove(size_t index)
{
	assert(Signal_Blocker::is_blocked());
	assert(executors_by_pid_size > 0);
	assert(executors_by_pid_size >= index + 1);
	memmove(executors_by_pid_key + index,
		executors_by_pid_key + index + 1,
		sizeof(*executors_by_pid_key) * (executors_by_pid_size - index - 1));
	memmove(executors_by_pid_value + index,
		executors_by_pid_value + index + 1,
		sizeof(*executors_by_pid_value) * (executors_by_pid_size - index - 1));
	-- executors_by_pid_size;
}

void File_Executor::check_file_was_built(
	Hash_Dep hash_dep,
	shared_ptr <const Place_Target> place_target)
{
	const char *filename= hash_dep.get_name_c_str_nondynamic();
	struct stat buf;
	if (stat(filename, &buf)) {
		bits |= B_MISSING;
		bits &= ~B_EXISTING;
		place_target->place <<
			fmt("file %s was not built by command", show(hash_dep));
		*this << "";
		raise(ERROR_BUILD);
		return;
	}

	warn_future_file(&buf, filename, place_target->place, "after execution of command");

	/* Check that file is not older that Stu startup */
	Timestamp timestamp_file(&buf);
	if (! timestamp.defined() || timestamp < timestamp_file)
		timestamp= timestamp_file;
	if (timestamp_file >= Timestamp::startup) return;

	/* Check whether the file is actually a symlink, in which case we ignore that
	 * error */
	if (0 > lstat(filename, &buf)) {
		place_target->place << format_errno("lstat", filename);
		raise(ERROR_BUILD);
	}
	if (S_ISLNK(buf.st_mode))
		return;
	place_target->place
		<< fmt("timestamp of file %s after execution of its command is older than %s startup",
			show(hash_dep), dollar_zero)
		<< fmt("timestamp of %s is %s", show(hash_dep), timestamp_file.format())
		<< fmt("startup timestamp is %s", Timestamp::startup.format());
	*this << "";
	explain_startup_time();
	raise(ERROR_BUILD);
}

void File_Executor::check_file_target_without_rule(
	shared_ptr <const Dep> dep,
	Hash_Dep &hash_dep,
	Executor *parent,
	bool &rule_not_found)
{
	if (dep->flags & (F_OPTIONAL | F_TRIVIAL)) return;
	struct stat buf;
	if (stat(hash_dep.get_name_c_str_nondynamic(), &buf)) {
		if (errno != ENOENT) {
			string text= show(hash_dep);
			print_errno("stat", hash_dep.get_name_c_str_nondynamic());
			raise(ERROR_BUILD);
		}
		/* File does not exist and there is no rule for it */
		rule_not_found= true;
	} else {
		/* File exists:  Do nothing, and there are no dependencies to build */
		if (dynamic_cast <Root_Executor *> (parent)) {
			/* Output this only for top-level targets, and therefore we don't
			 * need traces */
			Style style= CH_OUT;
			print_out(fmt("No rule for building %s, but the file exists",
					show(hash_dep, style)));
			hide_out_message= true;
		}
	}
}

bool File_Executor::start(
	shared_ptr <const Dep> dep_link,
	pid_t &pid,
	const std::map <string, string> &mapping)
{
	if (rule->is_copy) {
		assert(rule->place_targets.size() == 1);
		assert(! (rule->place_targets.front()->flags & F_TARGET_TRANSIENT));
		string source= rule->filename.unparametrized();

		/* If optional copy, don't just call 'cp' and let it fail:  Look up
		 * whether the source exists in the cache */
		if (rule->deps.at(0)->flags & F_OPTIONAL) {
			Executor *executor_source_base=
				executors_by_hash_dep.at(Hash_Dep(0, source));
			assert(executor_source_base);
			File_Executor *executor_source
				= dynamic_cast <File_Executor *> (executor_source_base);
			assert(executor_source);
			if (executor_source->bits & B_MISSING) {
				/* Neither the source file nor the target file exist:  an
				 * error. */
				rule->deps.at(0)->get_place()
					<< fmt("source file %s in optional copy rule must exist",
						::show(source));
				*this << fmt("when target file %s does not exist",
					show(hash_deps.at(0)));
				explain_missing_optional_copy_source();
				raise(ERROR_BUILD);
				done |= Done::from_flags(dep_link->flags);
				return true;
			}
		}

		pid= job.start_copy(
			rule->place_targets[0]->place_name.unparametrized(), source);
	} else {
		pid= job.start(
			rule->command->command,
			mapping,
			rule->redirect_index < 0 ? "" :
			rule->place_targets[rule->redirect_index]
			->place_name.unparametrized(),
			rule->filename.unparametrized(),
			rule->command->place);
	}
	return false;
}
