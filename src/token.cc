#include "token.hh"

Command::Command(string command_,
		 const Place &place_,
		 const Place &place_start_,
		 Environment environment_)
	:  Token(environment_),
	   command(command_),
	   place(place_),
	   place_start(place_start_)
{  }

const std::vector <string> &
Command::get_lines() const
/* This code parses the command string into lines ready for output.
 * Most of the code is for making the output pretty.
 *
 * We only output a command when it has a single line, but the following
 * code also handles the case of multiline commands.  We keep it because
 * we may need it in the future.  */
{
	if (lines != nullptr)
		return *lines;
	lines= std::unique_ptr <std::vector <string> > (new std::vector <string> ());
	const char *p= command.c_str();
	const char *p_end= p + command.size();

	/* Split into lines */
	while (p < p_end) {
		const char *q= p;
		while (p < p_end && *p != '\n')  ++p;
		string line(q, p - q);
		if (line.size()) {
			/* Discard lines consisting only of whitespace */
			bool keep= false;
			for (size_t i= 0; i < line.size(); ++i) {
				if (! isspace(line[i]))
					keep= true;
			}
			if (keep)
				lines->push_back(line);
		}
		if (p < p_end) {
			assert(*p == '\n');
			++p;
		}
	}

	/* Remove initial whitespace common to all lines */
	while (lines->size()) {
		char begin= (*lines)[0][0];
		if ((begin & 0x80) || ! isspace(begin))  break;
		bool equal= true;
		for (auto &i: *lines) {
			assert(i.size());
			if (i[0] != begin)  equal= false;
		}
		if (! equal)  break;
		for (size_t i= 0; i < lines->size(); ) {
			string &line= (*lines)[i];
			assert(line.size());
			line.erase(0, 1);
			if (line.empty())
				lines->erase(lines->begin() + i);
			else
				++i;
		}
	}

	/* Remove whitespace at end of lines */
	for (string &line: *lines) {
		size_t l= line.size();
		while (l != 0 && isspace(line[l - 1]))  --l;
		line.resize(l);
	}

	return *lines;
}

void Operator::render(Parts &parts, Rendering) const
{
	parts.append_operator(string(1, op));
}

void Flag_Token::render(Parts &parts, Rendering) const
{
	parts.append_operator(frmt("-%c", flag));
}
