#ifndef DEBUG_HH
#define DEBUG_HH

/* 
 * Helper class for debug output (option -d).  Provides indentation.  During the
 * lifetime of an object, padding is increased by one step.  This class is
 * declared within blocks in functions such as execute(), etc.  The passed
 * Execution is valid until the end of the object's lifetime.  
 */

class Debuggable
{
public:
	virtual string format_src() const= 0;
	virtual ~Debuggable();
};

class Debug
{
public:
	Debug(const Debuggable *d) 
	{
		padding_current += "   ";
		debuggables.push_back(d); 
	}

	~Debug() 
	{
		padding_current.resize(padding_current.size() - 3);
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
	static vector <const Debuggable *> debuggables; 

	static void print(string text_target, string text);
};

string Debug::padding_current= "";
vector <const Debuggable *> Debug::debuggables; 

Debuggable::~Debuggable()  {  }

void Debug::print(const Debuggable *d, string text) 
{
	if (d == nullptr) {
		print("", text);
	} else {
		if (debuggables.size() > 0 &&
		    debuggables[debuggables.size() - 1] == d) {
			print(d->format_src(), text); 
		} else {
			Debug debug(d);
			print(d->format_src(), text); 
		}
	}
}

void Debug::print(string text_target,
		  string text)
{
	assert(! text.empty());
	assert(text[0] >= 'a' && text[0] <= 'z'); 
	assert(text[text.size() - 1] != '\n');

	if (! option_debug) 
		return;

	if (! text_target.empty())
		text_target += ' ';

	fprintf(stderr, "DEBUG  %s%s%s\n",
		padding(),
		text_target.c_str(),
		text.c_str()); 
}

#endif /* ! DEBUG_HH */
