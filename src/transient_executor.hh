#ifndef TRANSIENT_EXECUTOR_HH
#define TRANSIENT_EXECUTOR_HH

/*
 * Used for non-dynamic transients that appear in rules that have only
 * transients as targets, and have no command.
 */

class Transient_Executor
	: public Executor
{
public:
	Transient_Executor(shared_ptr <const Dep> dep_link,
			   Executor *parent,
			   shared_ptr <const Rule> rule,
			   shared_ptr <const Rule> param_rule,
			   std::map <string, string> &mapping_parameter,
			   int &error_additional);

	shared_ptr <const Rule> get_rule() const {  return rule;  }
	virtual bool want_delete() const override;
	virtual Proceed execute(shared_ptr <const Dep> dep_link) override;
	virtual bool finished(Flags flags) const override;
	virtual void render(Parts &, Rendering= 0) const override;
	virtual void notify_result(shared_ptr <const Dep> dep,
				   Executor *, Flags flags,
				   shared_ptr <const Dep> dep_source) override;
	virtual void notify_variable(const std::map <string, string> &) override;

protected:
	virtual bool optional_finished(shared_ptr <const Dep> ) override;

private:
	~Transient_Executor();

	std::vector <Hash_Dep> hash_deps;
	/* The targets to which this executor object corresponds.  All are transients.
	 * Contains at least one element. */

	shared_ptr <const Rule> rule;
	/* The instantiated file rule for this executor.  Not null. */

	Timestamp timestamp_old;
	Done done;

	std::map <string, string> mapping_parameter;
	/* Contains the parameters; is not used */

	std::map <string, string> mapping_variable;
	/* Variable assignments from variables dependencies.  This is in
	 * Transient_Executor because it may be percolated up to the parent executor. */
};

#endif /* ! TRANSIENT_EXECUTOR_HH */
