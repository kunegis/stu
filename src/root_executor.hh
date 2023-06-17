#ifndef ROOT_EXECUTOR_HH
#define ROOT_EXECUTOR_HH

class Root_Executor
	:  public Executor
{
public:
	explicit Root_Executor(const vector <shared_ptr <const Dep> > &dep);
	virtual bool want_delete() const  {  return true;  }
	virtual Proceed execute(shared_ptr <const Dep> dep_this);
	virtual bool finished() const;
	virtual bool finished(Flags flags) const;

	virtual void render(Parts &parts, Rendering= 0) const {
		parts.append_operator_unquotable("ROOT");
	}

protected:
	virtual int get_depth() const  {  return -1;  }
	virtual bool optional_finished(shared_ptr <const Dep> )  {  return false;  }

private:
	bool is_finished;
};

#endif /* ! ROOT_EXECUTOR */
