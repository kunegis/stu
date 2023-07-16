#include "debug.hh"

string Debug::padding_current= "";
std::vector <const Debuggable *> Debug::debuggables;

Debuggable::~Debuggable()  {  }

void Debug::print(const Debuggable *d, string text)
{
	if (! option_d)
		return;
	if (d == nullptr) {
		print("", text);
	} else {
		if (debuggables.size() > 0 &&
		    debuggables[debuggables.size() - 1] == d) {
			print(show(d, S_DEBUG, R_SHOW_FLAGS), text);
		} else {
			Debug debug(d);
			print(show(d, S_DEBUG, R_SHOW_FLAGS), text);
		}
	}
}

void Debug::print(string text_target,
		  string text)
{
	assert(! text.empty());
	assert(text[0] >= 'a' && text[0] <= 'z');
	assert(text[text.size() - 1] != '\n');

	if (! option_d)
		return;

	if (! text_target.empty())
		text_target += ' ';

	fprintf(stderr, "DEBUG  %s%s%s\n",
		padding(),
		text_target.c_str(),
		text.c_str());
}

void render(const Debuggable *debuggable, Parts &parts, Rendering rendering)
{
	debuggable->render(parts, rendering);
}
