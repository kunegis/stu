#ifndef TRANSIENT_EXECUTOR_HH
#define TRANSIENT_EXECUTOR_HH

/*
 * Used for non-dynamic transients that appear in rules that have only
 * transients as targets, and have no command.  If at least one file
 * target or a command is present in the rule, File_Executor is used.
 */

class Transient_Executor
	:  public Executor
{
public:
	Transient_Executor(shared_ptr <const Dep> dep_link,
			   Executor *parent,
			   shared_ptr <const Rule> rule,
			   shared_ptr <const Rule> param_rule,
			   map <string, string> &mapping_parameter,
			   int &error_additional);

	shared_ptr <const Rule> get_rule() const {  return rule;  }
	virtual bool want_delete() const {  return false;  }
	virtual Proceed execute(shared_ptr <const Dep> dep_this);
	virtual bool finished() const;
	virtual bool finished(Flags flags) const;
	virtual string format_src() const;
	virtual void notify_result(shared_ptr <const Dep> dep,
				   Executor *, Flags flags,
				   shared_ptr <const Dep> dep_source);
	virtual void notify_variable(const map <string, string> &result_variable_child) {
		result_variable.insert(result_variable_child.begin(),
				       result_variable_child.end());
	}

protected:
	virtual int get_depth() const {  return 0;  }
	virtual bool optional_finished(shared_ptr <const Dep> ) {  return false;  }

private:
	~Transient_Executor();

	vector <Target> targets;
	/* The targets to which this executor object corresponds.  All
	 * are transients.  Contains at least one element.  */

	shared_ptr <const Rule> rule;
	/* The instantiated file rule for this executor.  Not null. */

	Timestamp timestamp_old;
	bool is_finished;

	map <string, string> mapping_parameter;
	/* Contains the parameters; is not used */

	map <string, string> mapping_variable;
	/* Variable assignments from variables dependencies.  This is in
	 * Transient_Executor because it may be percolated up to the
	 * parent executor.  */
};

#endif /* ! TRANSIENT_EXECUTOR_HH */
