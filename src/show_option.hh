#ifndef SHOW_OPTION_HH
#define SHOW_OPTION_HH

class Option_View
{
public:
	Option_View(char c_): c(c_) {}
private:
	friend void render(Option_View, Parts &, Rendering);
	char c;
};

void render(Option_View, Parts &, Rendering= 0);

#endif /* ! SHOW_OPTION_HH */
