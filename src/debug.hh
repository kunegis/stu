#ifndef DEBUG_HH
#define DEBUG_HH

/*
 * Debug output (option -d).  Provides indentation.  During the lifetime of an
 * object, padding is increased by one step.  This class is declared within
 * blocks in functions such as execute(), etc.  The passed Executor is valid
 * until the end of the object's lifetime.
 */

class Debuggable
{
public:
	virtual void render(Parts &, Rendering= 0) const= 0;
	virtual ~Debuggable();
};

#define DEBUG_PADDING_TEXT "   "

class Debug
{
public:
	Debug(const Debuggable *d) {
		padding_current += DEBUG_PADDING_TEXT;
		debuggables.push_back(d);
	}

	~Debug() {
		padding_current.resize(padding_current.size() - strlen(DEBUG_PADDING_TEXT));
		debuggables.pop_back();
	}

	static const char *padding() {
		return padding_current.c_str();
	}

	static void print(const Debuggable *, string text);
	/* Print a line for debug mode.  The given TEXT starts with the
	 * lower-case name of the operation being performed, followed by
	 * parameters, and not ending in a newline or period.  */

private:
	static string padding_current;
	static std::vector <const Debuggable *> debuggables;
	static void print(string text_target, string text);
};

void render(const Debuggable *debuggable, Parts &parts, Rendering rendering= 0)
{
	debuggable->render(parts, rendering);
}

#define DEBUG_PRINT(text) if (option_d)  Debug::print(this, text)

#endif /* ! DEBUG_HH */
