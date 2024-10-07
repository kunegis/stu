#ifndef DEBUG_HH
#define DEBUG_HH

/*
 * Debug output (option -d).  This class is declared within blocks in functions
 * such as execute(), etc.  The passed Executor is valid until the end of the
 * object's lifetime.
 */

#include <vector>

#include "options.hh"
#include "show.hh"

class Debuggable
{
public:
	virtual void render(Parts &, Rendering= 0) const= 0;
	virtual ~Debuggable() { }
};

class Debug
{
public:
	Debug(const Debuggable *d) {
		padding_current += padding_text;
		debuggables.push_back(d);
	}

	~Debug() {
		padding_current.resize(padding_current.size() - strlen(padding_text));
		debuggables.pop_back();
	}

	static const char *padding() {
		return padding_current.c_str();
	}

	static void print(const Debuggable *, string text);
	/* Print a line for debug mode.  The given TEXT starts with the lower-case name of
	 * the operation being performed, followed by parameters, and not ending in a
	 * newline or period. */

private:
	static string padding_current;
	static std::vector <const Debuggable *> debuggables;
	static void print(string text_target, string text);
	static constexpr const char *padding_text= "   ";
};

void render(const Debuggable *debuggable, Parts &parts, Rendering rendering= 0);

#define DEBUG_PRINT(text) if (option_d) Debug::print(this, text)

#endif /* ! DEBUG_HH */
