#ifndef DYNAMIC_EXECUTOR_HH
#define DYNAMIC_EXECUTOR_HH

/*
 * This is used for all dynamic targets, regardless of whether they are
 * files, transients, or concatenations.
 *
 * If it corresponds to a (possibly multiply) dynamic transient or file,
 * it used for caching and is not deleted.  If it corresponds to a
 * concatenation, it is not cached, and is deleted when not used anymore.
 *
 * Each dynamic executor corresponds to an exact dynamic dependency,
 * taking into account all flags.  This is as opposed to file
 * executors, where multiple file dependencies share a single executor
 * object.
 *
 * The implementation:  we have exactly one Result_Executor as a child,
 * which generates the list of dependencies that we are then adding as
 * children to ourselves.
 */

class Dynamic_Executor
	:  public Executor
{
public:
	Dynamic_Executor(shared_ptr <const Dynamic_Dep> dep_,
			 Executor *parent,
			 int &error_additional);

	shared_ptr <const Dynamic_Dep> get_dep() const {  return dep;  }

	virtual bool want_delete() const override;
	virtual Proceed execute(shared_ptr <const Dep> dep_this) override;
	virtual bool finished() const override;
	virtual bool finished(Flags flags) const override;
	virtual int get_depth() const override { return dep->get_depth(); }
	virtual bool optional_finished(shared_ptr <const Dep> ) override { return false; }
	virtual void render(Parts &, Rendering= 0) const override;
	virtual void notify_variable(const std::map <string, string> &result_variable_child) override {
		result_variable.insert(result_variable_child.begin(),
				       result_variable_child.end());
	}
	virtual void notify_result(shared_ptr <const Dep> dep,
				   Executor *source,
				   Flags flags,
				   shared_ptr <const Dep> dep_source) override;

private:
	const shared_ptr <const Dynamic_Dep> dep;
	/* A dynamic of anything */

	bool is_finished;
};

#endif /* ! DYNAMIC_EXECUTOR_HH */
