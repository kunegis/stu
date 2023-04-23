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

	virtual bool want_delete() const;
	virtual Proceed execute(shared_ptr <const Dep> dep_this);
	virtual bool finished() const;
	virtual bool finished(Flags flags) const;
	virtual int get_depth() const {  return dep->get_depth();  }
	virtual bool optional_finished(shared_ptr <const Dep> ) {  return false;  }
	virtual string format_src() const;
	virtual void notify_variable(const map <string, string> &result_variable_child) {
		result_variable.insert(result_variable_child.begin(),
				       result_variable_child.end());
	}
	virtual void notify_result(shared_ptr <const Dep> dep,
				   Executor *source,
				   Flags flags,
				   shared_ptr <const Dep> dep_source);

private:
	const shared_ptr <const Dynamic_Dep> dep;
	/* A dynamic of anything */

	bool is_finished;
};

#endif /* ! DYNAMIC_EXECUTOR_HH */
