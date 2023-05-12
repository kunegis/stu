#include "target.hh"

#include "canonicalize.hh"

string Target::format(Style style, Quotes *q) const
{
	string ret;
	size_t i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		assert((get_word(i) & F_TARGET_TRANSIENT) == 0);
		if (!(style & S_NO_FLAGS)) {
			ret += flags_format
				(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT));
		}
		++i;
		ret += '[';
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
	if (!(style & S_NO_FLAGS)) {
		ret += flags_format(get_word(i) & ~(F_TARGET_TRANSIENT | F_VARIABLE));
	}
	if (get_word(i) & F_TARGET_TRANSIENT) {
		ret += '@';
	}
	//	Style style2= 0;
//	if (! is_file())
//		style2 |= S_MARKERS;
//	else
//		style2 |= style;
//	style2 &= ~S_QUOTES;
//	bool quotes_inner= false;
//	bool detached= is_dynamic() || is_transient();
//	if (! detached)
//		style2 |= (style & S_QUOTES); 
//		quotes_inner= quotes;
	string s= name_format(text.substr(sizeof(word_t) * (i + 1)), style | S_INNER, q);
//	if (! detached)
//		style |= style2 & S_QUOTES;
//		quotes |= quotes_inner;
	ret += s;
	i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		++i; // TODO fold into while
		ret += ']';
	}
	return quote(ret, style, q);
}

// string Target::format_out() const
// {
// 	Style style= S_WANT_ESCAPE;
// 	if (! is_file()) {
// 		style |= S_MARKERS;
// 	}
// 	style |= S_QUOTES * is_file();
// 	string ret;
// 	size_t i= 0;
// 	while (get_word(i) & F_TARGET_DYNAMIC) {
// 		ret += flags_format(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT));
// 		++i;
// 		ret += '[';
// 	}
// 	assert(text.size() > sizeof(word_t) * (i + 1));
// 	ret += flags_format(get_word(i) & ~(F_TARGET_TRANSIENT | F_VARIABLE));
// 	if (get_word(i) & F_TARGET_TRANSIENT) {
// 		ret += '@';
// 	}
// 	string name_text = name_format(text.substr(sizeof(word_t) * (i + 1)), style);
// 	if (style & S_QUOTES)  ret += '\'';
// 	ret += name_text;
// 	if (style & S_QUOTES)  ret += '\'';
// 	i= 0;
// 	while (get_word(i) & F_TARGET_DYNAMIC) {
// 		++i;
// 		ret += ']';
// 	}
// 	return ret;
// }

// string Target::format_out_print_word() const
// {
// 	Style style= S_WANT_ESCAPE;
// 	if (! is_file()) {
// 		style |= S_MARKERS;
// 	}
// 	style |= S_QUOTES * is_file() * Color::quotes;
// //	bool quotes= is_file() ? Color::quotes : false;
// 	string ret;
// 	ret += Color::out_print_word;
// 	size_t i= 0;
// 	while (get_word(i) & F_TARGET_DYNAMIC) {
// 		ret += flags_format(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT));
// 		++i; // TODO fold into previous line
// 		ret += '[';
// 	}
// 	assert(text.size() > sizeof(word_t) * (i + 1));
// 	ret += flags_format(get_word(i) & ~(F_TARGET_TRANSIENT | F_VARIABLE));
// 	if (get_word(i) & F_TARGET_TRANSIENT) {
// 		ret += '@';
// 	}
// 	string name_text= name_format(text.substr(sizeof(word_t) * (i + 1)), style);
// 	if (style & S_QUOTES)  ret += '\'';
// 	ret += name_text;
// 	if (style & S_QUOTES)  ret += '\'';
// 	i= 0;
// 	while (get_word(i) & F_TARGET_DYNAMIC) {
// 		++i; // TODO fold into previous line
// 		ret += ']';
// 	}
// 	ret += Color::out_print_word_end;
// 	return ret;
// }

// string Target::format_err() const
// {
// 	Style style= S_WANT_ESCAPE;
// 	if (! is_file()) {
// 		style |= S_MARKERS;
// 	}
// 	style |= S_QUOTES * is_file() * Color::quotes;
// //	bool quotes= is_file() ? Color::quotes : false;
// 	string ret;
// 	ret += Color::word;
// 	size_t i= 0;
// 	while (get_word(i) & F_TARGET_DYNAMIC) {
// 		++i; // TODO fold into previous line
// 		ret += '[';
// 	}
// 	assert(text.size() > sizeof(word_t) * (i + 1));
// 	if (get_word(i) & F_TARGET_TRANSIENT) {
// 		ret += '@';
// 	}
// 	string name_text= name_format(text.substr(sizeof(word_t) * (i + 1)), style);
// 	if (style & S_QUOTES)  ret += '\'';
// 	ret += name_text;
// 	if (style & S_QUOTES)  ret += '\'';
// 	i= 0;
// 	while (get_word(i) & F_TARGET_DYNAMIC) {
// 		++i; // TODO fold into previous line
// 		ret += ']';
// 	}
// 	ret += Color::end;
// 	return ret;
// }

// string Target::format_src() const
// {
// 	Style style= S_WANT_ESCAPE;
// 	if (! is_file()) {
// 		style |= S_MARKERS;
// 	}
// 	string ret;
// 	size_t i= 0;
// 	while (get_word(i) & F_TARGET_DYNAMIC) {
// 		ret += flags_format(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT));
// 		++i;
// 		ret += '[';
// 	}
// 	assert(text.size() > sizeof(word_t) * (i + 1));
// 	ret += flags_format(get_word(i) & ~F_TARGET_TRANSIENT);
// 	if (get_word(i) & F_TARGET_TRANSIENT) {
// 		ret += '@';
// 	}
// //	const char *const name= text.c_str() + sizeof(word_t) * (i + 1);
// //	bool quotes= src_need_quotes(name);
// 	style |= S_WANT_ESCAPE;
// 	string name_text= name_format(text.substr(sizeof(word_t) * (i + 1)), style);
// 	if (style & S_QUOTES)  ret += '\'';
// 	ret += name_text;
// 	if (style & S_QUOTES)  ret += '\'';
// 	i= 0;
// 	while (get_word(i) & F_TARGET_DYNAMIC) {
// 		++i;
// 		ret += ']';
// 	}
// 	return ret;
// }

void Target::canonicalize()
{
	char *b= (char *)text.c_str(), *p= b;
	while ((*(word_t *)p) & F_TARGET_DYNAMIC)
		p += sizeof(word_t);
	p += sizeof(word_t);
	p= canonicalize_string(A_BEGIN | A_END, p);
	text.resize(p - b);
}

string Name::instantiate(const map <string, string> &mapping) const
/*
 * This function must take into account the special rules.
 *
 * Special rule (a) does not need to be handled, (i.e., we keep the starting './')
 */
{
	assert(texts.size() == 1 + parameters.size());
	const size_t n= get_n();
	string ret= texts[0];

	for (size_t i= 0;  i < n;  ++i) {
		assert(parameters[i].size() > 0);

		/* Special rule (b) */
		if (i == 0 && texts[0].empty()
		    && mapping.at(parameters[0]) == "/"
		    && texts[1].size() != 0 && texts[1][0] == '/') {
			/* Do nothing */
		}
		else
			ret += mapping.at(parameters[i]);

		ret += texts[i + 1];
	}

	return ret;
}

bool Name::match(const string name,
		 map <string, string> &mapping,
		 vector <size_t> &anchoring,
		 int &priority) const
/*
 * Rule:  Each parameter must match at least one character.
 *
 * This algorithm uses one pass without backtracking or recursion.
 * Therefore, there are no "deadly" patterns that can make it hang,
 * which is a common source of errors for naive trivial implementations
 * of regular expression matching.
 *
 * This implementation takes into account the special rules described in
 * the manpage.  Each special rule is referred to by a letter (a, b, c,
 * etc.)
 */
{
	assert(mapping.size() == 0);
	priority= 0;
	map <string, string> ret;
	const size_t n= get_n();

	if (name.empty()) {
		return n == 0 && texts[0].empty();
	}

	anchoring.resize(2 * n);

	/*
	 * Special rules
	 */

	/* ./$A... is equivalent to $A... where $A must not begin in a
	 * slash */
	bool special_a= n != 0 && texts[0] == "./";
	if (special_a)
		priority= 1;

	/* $A/bbb matches /bbb with $A set to / */
	bool special_b_potential= n != 0
		&& texts[0].empty() && texts[1].size() != 0 && texts[1][0] == '/';

	bool special_c= false;  /* We are in the second pass for Special Rule (c) */

 restart:
	const char *const p_begin= name.c_str();
	const char *p= p_begin;
	const char *const p_end= name.c_str() + name.size();

	/* Match first text */
	if (! special_a) {
		size_t k= texts[0].size();
		if ((size_t)(p_end - p) <= k) {
			goto failed;
		}
		/* Note:  K can be zero here, in which case memcmp()
		 * always returns zero, i.e., a match.  */
		if (memcmp(p, texts[0].c_str(), k)) {
			goto failed;
		}
		p += k;
		anchoring[0]= k;
	} else {
		/* In this case, the text contains './', but should be
		 * treated as the empty string */
		assert(texts[0] == "./");
		anchoring[0]= 0;
	}

	/* I goes over all parameters */
	for (size_t i= 0;  i < n;  ++i) {
		size_t length_min= 1;
		/* Minimal length of the matching parameter */

		if (special_b_potential && i == 0)
			length_min= 0;

		if (special_c && i == 0) {
			ret[parameters[i]]= ".";
			anchoring[2*i + 1]= p_end - p_begin; /* The anchoring has zero length */
			if (p + (texts.at(i+1).size() - 1) > p_end)
				goto failed;
			if (memcmp(p, texts.at(i+1).c_str() + 1, texts.at(i+1).size() - 1))
				goto failed;
			p += texts.at(i+1).size() - 1;
			continue;
		}
		if (i == n - 1) {
			/* For the last segment, texts[n-1] must match the end of the input string */
			size_t size_last= texts[n].size();
			const char *last= texts[n].c_str();
			/* Minimal length of matched text */
			if (p_end - p < (ssize_t) length_min + (ssize_t) size_last)
				goto failed;
			if (memcmp(p_end - size_last, last, size_last))
				goto failed;
			string matched= string(p, p_end - p - size_last);
			if (matched.empty()) {
				assert(special_b_potential);
				priority= 1;
				matched= "/";
			}
			ret[parameters[i]]= matched;
			anchoring[2*i + 1]= p_end - size_last - p_begin;
			assert(ret.at(parameters[i]).size() >= length_min);
		} else {
			/* Intermediate texts must not be empty, i.e.,
			 * two parameters cannot be unseparated */
			assert(texts[i+1].size() != 0);
			const char *q= strstr(p+length_min, texts[i+1].c_str());
			if (q == nullptr)
				goto failed;
			assert(q >= p + length_min);
			anchoring[i * 2 + 1]= q - p_begin;
			string matched= string(p, q-p);
			assert(matched.size() >= length_min);
			if (special_a) {
				assert(matched.size() > 0);
				if (i == 0 && matched[0] == '/')
					goto failed;
			}
			if (matched.empty()) {
				assert(special_b_potential);
				priority= 1;
				matched= "/";
			}
			ret[parameters[i]]= matched;
			p= q + texts[i+1].size();
			anchoring[i * 2 + 2]= p - p_begin;
		}
	}

	/* There is a match */
	swap(mapping, ret);
	assert(anchoring.size() == 2 * n);
	return true;

 failed:
	if (special_c)
		return false;

	/*
	 * Special rule (c):  $A/bbb matches bbb with $A set to .
	 *
	 * For special rule (c), we must do two passes:
	 *   (1) Normal pass, matching the given pattern
	 *   (2) Special (c) pass
	 *
	 * We can't just test wether the pattern matches as rule (c), because
	 * there are patterns that match both with and without rule (c).
	 *
	 * Example:  Does $X/bbb/$Y match 'bbb/bbb/bbb' as
	 *   (1) $X = . and $Y = bbb/bbb , or
	 *   (2) $X = bbb and $Y = bbb ?
	 * Answer:  It's (2), because that's not the special rule.
	 */

	if (n != 0 && texts[0].empty() && texts[1].size() != 0 && texts[1][0] == '/') {
		special_c= true;
		priority= -1;
		goto restart;
	} else {
		return false;
	}
}

string Name::get_duplicate_parameter() const
{
	vector <string> seen;
	for (auto &parameter:  parameters) {
		for (const auto &parameter_seen:  seen) {
			if (parameter_seen == parameter) {
				return parameter;
			}
		}
		seen.push_back(parameter);
	}
	return "";
}

bool Name::valid(string &param_1, string &param_2) const
{
	if (empty())
		return false;

	for (size_t i= 1;  i + 1 < get_n() + 1;  ++i) {
		if (texts[i].empty()) {
			param_1= parameters[i-1];
			param_2= parameters[i];
			return false;
		}
	}

	return true;
}

bool Name::anchoring_dominates(vector <size_t> &anchoring_a,
			       vector <size_t> &anchoring_b,
			       int priority_a, int priority_b)
/*
 * (A) dominates (B) when every character in a parameter in (A) is also
 * in a parameter in (B) and at least one character is not parametrized
 * in (A) but in (B).
 *
 * If the anchorings are equal, the special bit dominates no special
 * bit.
 */
{
	assert(anchoring_a.size() % 2 == 0);
	assert(anchoring_b.size() % 2 == 0);

	const size_t k_a= anchoring_a.size();
	const size_t k_b= anchoring_b.size();
	bool dominate= false;
	size_t p= 0; /* Position in the string */
	size_t i= 0; /* Index in (A) */
	size_t j= 0; /* Index in (B) */

	while (true) {
		if (i < k_a && p == anchoring_a[i])  ++i;
		if (j < k_b && p == anchoring_b[j])  ++j;

		assert(i == k_a || p <= anchoring_a[i]);
		assert(j == k_b || p <= anchoring_b[j]);

		/* A character is parametrized in (B) but not in (A) */
		if ((i % 2) == 0 && (j % 2) != 0)
			dominate= true;

		/* A character is parametrized in (A) but not in (B) */
		else if ((i % 2) != 0 && (j % 2) == 0)
			return false;

		/* End or increment */
		if (i == k_a && j == k_b) {
			if (dominate) {
				return true;
			} else {
				/* The anchorings are equal (up to
				 * PRIORITY) */
				assert(anchoring_a == anchoring_b);
				return priority_a > priority_b;
			}
		}

		else if (i < k_a && j == k_b)
			p= anchoring_a[i];
		else if (j < k_b && i == k_a)
			p= anchoring_b[j];
		else if (i < k_a && j < k_b)
			p= min(anchoring_a[i], anchoring_b[j]);
	}
}

string Name::format(Style style, Quotes *q) const
{
	assert(texts.size() == 1 + parameters.size());
 restart:
	bool quotes_initial= q && q->is();
	string ret= name_format(texts[0], style | S_INNER, q);
	for (size_t i= 0;  i < get_n();  ++i) {
//		style_this= style | S_MARKERS | S_NO_EMPTY;
		ret += "${";
		ret += name_format(parameters[i], style | S_INNER, q);
		ret += '}';
		ret += name_format(texts[1+i], style | S_INNER, q);
		if (quotes_initial != (q && q->is()))
//		if ((style_this & S_QUOTES) != quotes_initial)
//		if (quotes != quotes_initial)
			goto restart;
	}
	return ret;
}

// string Name::format_raw() const
// {
// 	assert(texts.size() == 1 + parameters.size());
// 	string ret= texts[0];
// 	for (size_t i= 0;  i < get_n();  ++i) {
// 		ret += "${";
// 		ret += parameters[i];
// 		ret += '}';
// 		ret += texts[1+i];
// 	}
// 	return ret;
// }

// string Name::format_err() const
// {
// 	//	bool quotes= Color::quotes;
// 	Style style= S_WANT_ESCAPE;
// 	string s= format(style);
// 	return fmt("%s%s%s%s%s",
// 		   Color::word,
// 		   style & S_QUOTES ? "'" : "",
// 		   s,
// 		   style & S_QUOTES ? "'" : "",
// 		   Color::end);
// }

// string Param_Target::format_err() const
// {
// 	Style style= S_WANT_ESCAPE;
// 	if (flags & F_TARGET_TRANSIENT)
// 		style |= S_MARKERS;
// 	style |= S_QUOTES * (flags == 0) * Color::quotes;
// //	bool quotes2= (flags == 0 ? Color::quotes : 0);
// 	string text= name.format(style);
// 	return fmt("%s%s%s%s%s%s",
// 		   Color::word,
// 		   flags ? "@" : "",
// 		   style & S_QUOTES ? "'" : "",
// 		   text,
// 		   style & S_QUOTES ? "'" : "",
// 		   Color::end);
// }

// string Name::format_out() const
// {
// //	bool quotes= true;
// 	Style style= S_WANT_ESCAPE;
// 	string s= format(style);
// 	return fmt("%s%s%s",
// 		   style & S_QUOTES ? "'" : "",
// 		   s,
// 		   style & S_QUOTES ? "'" : "");
// }

string Name::format_glob() const
{
	Style style= S_SRC;
	Quotes q(style);
	string ret= texts[0];
	for (size_t i= 0;  i < get_n();  ++i) {
		ret += "*";
		Style style_inner= S_INNER | S_NO_EMPTY * !empty(); 
		ret += name_format(texts[i+1], style_inner, &q);
	}
	return quote(ret, style, &q);
}

// string Name::format_src() const
// {
// 	Style style= S_WANT_ESCAPE;
// //	bool quotes= false;
// //	for (const string &t:  texts)
// //		if (src_need_quotes(t))
// //			quotes= true;
// 	string s= format(style);
// 	return fmt("%s%s%s",
// 		   style & S_QUOTES ? "'" : "",
// 		   s,
// 		   style & S_QUOTES ? "'" : "");
// }

void Name::canonicalize()
{
	for (size_t i= 0;  i <= get_n();  ++i) {
                char *const p= (char *) texts[i].c_str();
		Canon_Flags canon_flags= 0;
		if (i == 0)
			canon_flags |= A_BEGIN;
		if (i == get_n())
			canon_flags |= A_END;
		const char *q= canonicalize_string(canon_flags, p);
		texts[i].resize(q - p);
	}
}

bool Name::operator==(const Name &that) const
{
	if (this->get_n() != that.get_n())
		return false;
	for (size_t i= 0;  i < get_n();  ++i) {
		if (this->parameters[i] != that.parameters[i])
			return false;
		if (this->texts[i] != that.texts[i])
			return false;
	}
	if (this->texts[get_n()] != that.texts[get_n()])
		return false;
	return true;
}

void Name::append(const Name &name)
{
	assert(this->texts.back() != "" ||
	       name.texts.back() != "");
	append_text(name.texts.front());
	for (size_t i= 0;  i < name.get_n();  ++i) {
		append_parameter(name.get_parameters()[i]);
		append_text(name.get_texts()[1 + i]);
	}
}

string Place_Param_Target::format(Style style, Quotes *q) const {
	string s= place_name.format(style | S_INNER, q);
	if (flags & F_TARGET_TRANSIENT)
		s= '@' + s;
	return quote(s, style, q);
//	...;
//	Target target(flags, place_name.format_raw());
//	return target.format(style, q);
}

void Place_Param_Target::canonicalize()
{
	place_name.canonicalize();
}

shared_ptr <const Place_Param_Target>
canonicalize(shared_ptr <const Place_Param_Target> place_param_target)
{
	shared_ptr <Place_Param_Target> ret=
		Place_Param_Target::clone(place_param_target);
	ret->canonicalize();
	return ret;
}
