#ifndef ROOT_EXECUTOR_HH
#define ROOT_EXECUTOR_HH

class Root_Executor
	:  public Executor
{
public:
	explicit Root_Executor(const std::vector <shared_ptr <const Dep> > &dep);
	virtual bool want_delete() const override { return true; }
	virtual Proceed execute(shared_ptr <const Dep> dep_link) override;
	virtual bool finished() const override;
	virtual bool finished(Flags flags) const override;

	virtual void render(Parts &parts, Rendering= 0) const override {
		parts.append_operator("ROOT");
	}

protected:
	virtual bool optional_finished(shared_ptr <const Dep> ) override { return false; }

private:
	bool is_finished;
};

#endif /* ! ROOT_EXECUTOR */
