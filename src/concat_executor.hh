#ifndef CONCAT_EXECUTOR_HH
#define CONCAT_EXECUTOR_HH

/*
 * An executor representing a concatenation.  Its dependency is
 * always a normalized concatenated dependency containing (compound
 * dependencies of) normalized dependencies, whose results are
 * concatenated as new targets added to the parent.  At least one of the
 * contained dependencies is dynamic, as otherwise the dependencies
 * would have been normalized to a non-concatenated dependency.
 *
 * Concatenated executors always have exactly one parent.  They are not
 * cached, and they are deleted when done.  Thus, they also don't need
 * the 'done' field.  (But the parent class has it.)
 */

#include "executor.hh"

class Concat_Executor
	:  public Executor
{
public:
	Concat_Executor(shared_ptr <const Concat_Dep> dep_,
			Executor *parent,
			int &error_additional);
	/* DEP_ is normalized.  See File_Executor::File_Executor() for
	 * the semantics for ERROR_ADDITIONAL.  */

	~Concat_Executor()= default;

	virtual int get_depth() const  {  return -1;  }
	virtual bool want_delete() const  {  return true;  }
	virtual Proceed execute(shared_ptr <const Dep> dep_this);
	virtual bool finished() const;
	virtual bool finished(Flags flags) const;
	virtual void render(Parts &parts, Rendering= 0) const {
		return dep->render(parts);
	}

	virtual void notify_variable(const map <string, string> &result_variable_child) {
		result_variable.insert(result_variable_child.begin(),
				       result_variable_child.end());
	}
	virtual void notify_result(shared_ptr <const Dep> dep,
				   Executor *source,
				   Flags flags,
				   shared_ptr <const Dep> dep_source);

protected:
	virtual bool optional_finished(shared_ptr <const Dep> ) {  return false;  }

private:
	typedef unsigned Stage;
	enum { S_DYNAMIC, S_NORMAL, S_FINISHED };

	shared_ptr <const Concat_Dep> dep;
	/* Contains the concatenation.  This is a normalized dependency. */

	Stage stage;
	vector <shared_ptr <Compound_Dep> > collected;

	void launch_stage_1();
};

#endif /* ! CONCAT_EXECUTOR_HH */
