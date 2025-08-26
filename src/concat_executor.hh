#ifndef CONCAT_EXECUTOR_HH
#define CONCAT_EXECUTOR_HH

/*
 * An executor representing a concatenation.  Its dependency is always a normalized
 * concatenated dependency containing (compound dependencies of) normalized dependencies,
 * whose results are concatenated as new targets added to the parent.  At least one of the
 * contained dependencies is dynamic, as otherwise the dependencies would have been
 * normalized to a non-concatenated dependency.
 *
 * Concatenated executors always have exactly one parent.  They are not cached, and they
 * are deleted when done.  Thus, they also don't need the 'done' field.  (But the parent
 * class has it.)  For the same reason, they also cannot appear in cycles.
 */

#include "executor.hh"

class Concat_Executor
	: public Executor
{
public:
	Concat_Executor(shared_ptr <const Concat_Dep> dep_,
			Executor *parent, int &error_additional);
	/* DEP_ is normalized.  See File_Executor::File_Executor() for the semantics for
	 * ERROR_ADDITIONAL. */

	~Concat_Executor()= default;

	virtual bool want_delete() const override { return true; }
	virtual Proceed execute(shared_ptr <const Dep> dep_link) override;
	virtual bool finished() const override;
	virtual bool finished(Flags flags) const override;
	virtual void render(Parts &parts, Rendering= 0) const override {
		return dep->render(parts);
	}

	virtual void notify_variable(
		const std::map <string, string> &result_variable_child) override;
	virtual void notify_result(
		shared_ptr <const Dep> dep_result, Executor *source, Flags flags,
		shared_ptr <const Dep> dep_source) override;

protected:
	virtual bool optional_finished(shared_ptr <const Dep> ) override { return false; }

private:
	typedef unsigned Stage;
	enum { ST_DYNAMIC, ST_NORMAL, ST_FINISHED };

	shared_ptr <const Concat_Dep> dep;
	/* Contains the concatenation.  This is a normalized dependency. */

	Stage stage;
	std::vector <shared_ptr <Compound_Dep> > collected;

	void launch_stage_normal();
};

#endif /* ! CONCAT_EXECUTOR_HH */
