#ifndef TARGET_HH
#define TARGET_HH

#include "flags.hh"

/* 
 * Targets are the individual "objects" of Stu.  They can be thought of
 * as the "native types" of Stu.  
 *
 * Targets can be either files or transients, and can have any level of
 * dynamicity. 
 * 
 * Targets are to be distinguished from the more general dependencies,
 * which can represent any nested expression, including concatenations,
 * flags, compound expressions, etc., while targets only represent
 * individual files or transients. 
 */

/* 
 * Glossary:
 *     * A _name_ is a filename or the name of a transient target.  They
 *       are just strings, so no special data type is used for them.
 *       There are two distinct namespaces for them (files and
 *       transients.)  They can contain any character except \0, and
 *       must not be the empty string.  
 *     * A _target_ is either file, transient target, or a dynamic^*
 *       of them.  It is represented by a name (string) and a type.  
 *     * A _parametrized_ target or name additionally can have
 *       parameters. 
 *     * Dedicated classes exist to represent these with _places_. 
 */

#if   C_WORD <= 8
typedef uint8_t  word_t;
#elif C_WORD <= 16
typedef uint16_t word_t;
#else
#	error "Invalid word size" 
#endif

/* 
 * A representation of a simple dependency, mainly used as the key in
 * the caching of Execution objects.  The difference to the Dependency
 * class is that Target objects don't store the Place objects, and don't
 * support parametrization.  Thus, Target objects are used as keys in
 * maps, etc.  Flags are included however. 
 */
class Target
{
public:
	
	explicit Target(string text_)
		/* TEXT_ is the full text field of this Target */
		:  text(text_)
	{  }

	Target(Flags flags, string name) 
	/* A plain target */
		: text(string_from_word(flags) + name)
	{
		assert((flags & ~F_TARGET_TRANSIENT) == 0); 
		assert(name.find('\0') == string::npos); /* Names do not contain \0 */
		assert(name != ""); 
	}

	Target(Flags flags, Target target)
	/* Makes the given target once more dynamic with the given
	 * flags, which must *not* contain the 'dynamic' flag.  */
		:  text(string_from_word(flags | F_TARGET_DYNAMIC) + target.text)
	{
		assert((flags & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT)) == 0);
		assert(flags <= (unsigned)(1 << C_WORD)); 
	}

	const string &get_text() const {  return text;  }
	string &get_text() {  return text;  }
	const char *get_text_c_str() const {  return text.c_str();  }

	bool is_dynamic() const {
		check(); 
		return get_word(0) & F_TARGET_DYNAMIC; 
	}

	bool is_file() const {
		check(); 
		return (get_word(0) & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT)) == 0; 
	}

	bool is_transient() const {
		check(); 
		return (get_word(0) & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT)) == F_TARGET_TRANSIENT; 
	}

	bool is_any_file() const {
		size_t i= 0;
		while (get_word(i) & F_TARGET_DYNAMIC) {
			++i;
		}
		return (get_word(i) & F_TARGET_TRANSIENT) == 0; 
	}

	bool is_any_transient() const {
		size_t i= 0;
		while (get_word(i) & F_TARGET_DYNAMIC) {
			++i;
		}
		return get_word(i) & F_TARGET_TRANSIENT; 
	}

	string format(Style style, bool &quotes) const;
	string format_out() const;
	string format_out_print_word() const;
	string format_word() const;
	string format_src() const;

	string get_name_nondynamic() const 
	/* Get the name of the target, knowing that the target is not dynamic */
	{
		check(); 
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0); 
		return text.substr(sizeof(word_t)); 
	}
	
	const char *get_name_c_str_nondynamic() const 
	/*
	 * Return a C pointer to the name of the file or transient.  The
	 * object must be non-dynamic. 
	 */
	{
		check(); 
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0); 
		return text.c_str() + sizeof(word_t); 
	}

	const char *get_name_c_str_any() const
	{
		const char *ret= text.c_str();
		while ((*(word_t *)ret) & F_TARGET_DYNAMIC)
			ret += sizeof(word_t);
		return 
			ret += sizeof(word_t); 
	}

	Flags get_front_word() const {  return get_word(0);  }
	
	word_t &get_front_word_nondynamic() 
	/* Get the front byte, given that the target is not dynamic */
	{
		check(); 
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0); 
		return *(word_t *)&text[0]; 
	}

	Flags get_front_word_nondynamic() const {
		check(); 
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0); 
		return *(word_t *)&text[0]; 
	}
	
	Flags get_word(size_t i) const 
	/* For access to any front word */
	{
		assert(text.size() > sizeof(word_t) * (i + 1)); 
		return ((word_t *)&text[0])[i]; 
	}

	bool operator== (const Target &target) const {  return text == target.text;  }
	bool operator!= (const Target &target) const {  return text != target.text;  }

	static string string_from_word(Flags flags)
	/* Return a string of length sizeof(word_t) containing the given
	 * flags  */
	{
		assert(flags <= 1 << C_WORD); 
		char ret[sizeof(word_t) + 1];
		ret[sizeof(word_t)] = '\0';
		*(word_t *)ret= (word_t)flags;
		return string(ret, sizeof(word_t)); 
	}

private:

	string text; 
	/*
	 * Linear representation of the dependency.
	 *
	 * This begins with a certain number of words (word_t, at least
	 * one), followed by the name of the target as a string.  A
	 * word_t is represented by a fixed number of characters. 
	 *
	 * A non-dynamic dependency is represented as a Type word
	 * (F_TARGET_TRANSIENT or 0) followed by the name.
	 *
	 * A dynamic is represented as a dynamic word (F_TARGET_DYNAMIC)
	 * followed by the string representation of the contained
	 * dependency. 
	 *
	 * Any of the front words may contain additional flag bits.
	 * There may be nul ('\0') bytes in the front words, but the
	 * name does not contain nul, as that is invalid in names.  The
	 * name proper (excluding front words) is non-empty. 
	 *
	 * The empty string denotes a "null" value for the type Target,
	 * or equivalently the target of the root dependency, in which
	 * case most functions should not be used.
	 */

	void check() const {
		/* The minimum length of TEXT is sizeof(word_t)+1:  One
		 * word indicating a non-dynamic target, and a text of
		 * length one.  (The text cannot be empty.)  */
#ifndef NDEBUG
		assert(text.size() > sizeof(word_t)); 
#endif /* ! NDEBUG */
	}
};

namespace std {
	template <>
	struct hash <Target>
	{
		size_t operator()(const Target &target) const {
			return hash <string> ()(target.get_text()); 
		}
	};
}

/* 
 * A parametrized name.  Each name has N >= 0 parameters.  When N > 0,
 * the name is parametrized, otherwise it is unparametrized.   
 * 
 * A name consists of (N+1) static text elements (in the variable TEXTS)
 * and N parameters, which are interleaved.  For instance when N = 2,
 * then the name is given by
 *
 *		texts[0] parameters[0] texts[1] parameters[1] texts[2].
 *
 * Parametrized names may be valid or invalid.  A parametrized name is
 * valid when all internal texts (between two parameters) are non-empty,
 * and, if N = 0, the single text is not empty. 
 *
 * A parametrized name is empty if N = 0 and the single text is empty. 
 */
class Name
{
private:

	vector <string> texts; 
	/* Length = N + 1.
	 * Only the first and last elements may be empty.  */ 

	vector <string> parameters;
	/* Length = N */ 

public:

	/* A name with zero parameters */ 
	Name(string name_)
		:  texts({name_})
	{ }

	/* Empty name */ 
	Name()
		:  texts({""})
	{ }

	bool empty() const {
		assert(texts.size() == 1 + parameters.size()); 
		return parameters.empty() && texts[0] == "";
	}

	/* Number of parameters; zero when the name is unparametrized. */ 
	size_t get_n() const {
		assert(texts.size() == 1 + parameters.size()); 
		return parameters.size(); 
	}

	bool is_parametrized() const {
		assert(texts.size() == 1 + parameters.size()); 
		return !parameters.empty(); 
	}

	const vector <string> &get_texts() const {
		return texts; 
	}

	const vector <string> &get_parameters() const {
		return parameters; 
	}

	/* Append the given PARAMETER and an empty text.  This does not
	 * check that the result is valid.  */ 
	void append_parameter(string parameter) {
		parameters.push_back(parameter); 
		texts.push_back("");
	}

	/* Append the given text to the last text element */
	void append_text(string text) {
		texts[texts.size() - 1] += text;
	}

	/* Append another parametrized name.  This function checks that
	 * the result is valid. */ 
	void append(const Name &name) {
		assert(this->texts.back() != "" ||
		       name.texts.back() != "");

		append_text(name.texts.front());

		for (size_t i= 0;  i < name.get_n();  ++i) {
			append_parameter(name.get_parameters()[i]);
			append_text(name.get_texts()[1 + i]);
		}
	}

	string &last_text() {
		return texts[texts.size() - 1];
	}

	const string &last_text() const {
		return texts[texts.size() - 1];
	}

	string instantiate(const map <string, string> &mapping) const;
	/* The name may be empty, resulting in an empty string */ 

	/* Return the unparametrized name.  The name must be unparametrized. */
	const string &unparametrized() const {
		assert(get_n() == 0);
		return texts[0]; 
	}

	bool match(string name, 
		   map <string, string> &mapping,
		   vector <size_t> &anchoring) const;
	/* Check whether NAME matches this name.  If it does, return
	 * TRUE and set MAPPING and ANCHORING accordingly. 
	 * MAPPING must be empty.  */
	
	/* No escape characters */
	string raw() const {
		assert(texts.size() == 1 + parameters.size()); 
		string ret= texts[0];
		for (size_t i= 0;  i < get_n();  ++i) {
			ret += "${";
			ret += parameters[i];
			ret += '}';
			ret += texts[1+i];
		}
		return ret; 
	}

	string format(Style style, bool &quotes) const {
		assert(texts.size() == 1 + parameters.size()); 

		string ret= name_format(texts[0], 
					style | S_MARKERS | S_NOEMPTY,
					quotes);

		for (size_t i= 0;  i < get_n();  ++i) {
			ret += "${";
			ret += name_format(parameters[i],
					   style | S_MARKERS | S_NOEMPTY,
					   quotes);
			ret += '}';
			ret += name_format
				(
				 texts[1+i],
				 style | S_MARKERS | S_NOEMPTY,
				 quotes); 
		}

		return ret; 
	}

	string format_word() const {
		bool quotes= Color::quotes;
		string s= format(0, quotes); 
		return fmt("%s%s%s%s%s",
			   Color::word,
			   quotes ? "'" : "",
			   s,
			   quotes ? "'" : "",
			   Color::end); 
	}

	string format_out() const {
		bool quotes= true;
		string s= format(0, quotes); 
		return fmt("%s%s%s",
			   quotes ? "'" : "",
			   s,
			   quotes ? "'" : "");
	}

	string format_src() const {
		bool quotes= false;
		for (const string &t:  texts)
			if (src_need_quotes(t))
				quotes= true; 
		string s= format(0, quotes); 
		return fmt("%s%s%s",
			   quotes ? "'" : "",
			   s,
			   quotes ? "'" : "");
	}

	string get_duplicate_parameter() const;
	/* Check whether there are duplicate parameters.  Return the
	 * name of the found duplicate parameter, or "" none is found.  */

	bool valid(string &param_1, string &param_2) const;
	/* Whether this is a valid name.  If it is not, fill the given
	 * parameters with the two unseparated parameters.  */ 

	bool operator == (const Name &that) const {
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

	static bool anchoring_dominates(vector <size_t> &anchoring_a,
					vector <size_t> &anchoring_b);
	/* Whether anchoring A dominates anchoring B.  The anchorings do
	 * not need to have the same number of parameters.  */
};

/* 
 * A parametrized name for which it is saved what type it represents.  Non-dynamic. 
 */ 
class Param_Target
{
public:

	Flags flags;
	/* Only file/transient target info */

	Name name; 
 
	Param_Target(Flags flags_,
		     const Name &name_)
		:  flags(flags_),
		   name(name_)
	{ 
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0); 
	}

	Param_Target(Target target)
	/* Unparametrized target.   The passed TARGET must be non-dynamic. */
		:  flags(target.get_front_word_nondynamic() & F_TARGET_TRANSIENT),
		   name(target.get_name_nondynamic())
	{ 
		assert(! target.is_dynamic()); 
	}

	Target instantiate(const map <string, string> &mapping) const {
		return Target(flags, name.instantiate(mapping)); 
	}

	string format_word() const {

		Style style= 0;
		if (flags & F_TARGET_TRANSIENT)
			style |= S_MARKERS;

		bool quotes2= 
			(flags == 0
			 ? Color::quotes
			 : 0);

		string text= name.format(style, quotes2); 
		
		return fmt("%s%s%s%s%s%s", 
			   Color::word, 
			   flags ? "@" : "",
			   quotes2 ? "'" : "",
			   text,
			   quotes2 ? "'" : "",
			   Color::end);
	}

	Target unparametrized() const 
	/* The corresponding unparametrized target.  This target must
	 * have zero parameters.  */ 
	{
		return Target(flags, name.unparametrized());
	}

	bool operator == (const Param_Target &that) const {
		return this->flags == that.flags &&
			this->name == that.name; 
	}

	bool operator != (const Param_Target &that) const {
		return ! (*this == that);
	}
};

/*
 * A parametrized name annotated with places. 
 */
class Place_Name
	:  public Name
{
public:

	Place place;
	/* Place of the name as a whole */ 

	vector <Place> places;
	/* Length = N (number of parameters). 
	 * The places of the individual parameters.  */

	Place_Name() 
		/* Empty parametrized name, and empty place */
		:  Name(),
		   place()
	{ }

	Place_Name(string name)
		/* Unparametrized, with empty place */ 
		:  Name(name)
	{ 
		/* PLACES remains empty */ 
	}
	
	Place_Name(string name, const Place &_place) 
		/* Unparametrized, with explicit place */
		:  Name(name), place(_place)
	{
		assert(! place.empty()); 
	}

	const vector <Place> &get_places() const {
		return places;
	}

	void append_parameter(string parameter,
			      const Place &place_parameter) 
	/* Append the given PARAMETER and an empty text */ 
	{
		Name::append_parameter(parameter); 
		places.push_back(place_parameter);
	}

	shared_ptr <Place_Name> instantiate(const map <string, string> &mapping) const 
	/* In the returned object, the PLACES vector is empty */ 
	{
		string name= Name::instantiate(mapping);
		return make_shared <Place_Name> (name, place);
	}
};

/* 
 * A target that is parametrized and contains places.  Non-dynamic. 
 */
class Place_Param_Target
{
public:

	Flags flags;
	/* Only F_TARGET_TRANSIENT is used */ 

	Place_Name place_name;

	Place place;
	/* The place of the target as a whole.  The PLACE_NAME
	 * variable additionally contains a place for the name itself,
	 * as well as for individual parameters.  */ 

	Place_Param_Target(Flags flags_,
			   const Place_Name &place_name_)
		:  flags(flags_),
		   place_name(place_name_),
		   place(place_name_.place)
	{ 
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0); 
	}

	Place_Param_Target(Flags flags_,
			   const Place_Name &place_name_,
			   const Place &place_)
		:  flags(flags_),
		   place_name(place_name_),
		   place(place_)
	{ 
		assert((flags_ & ~F_TARGET_TRANSIENT) == 0); 
	}

	Place_Param_Target(const Place_Param_Target &that)
		:  flags(that.flags),
		   place_name(that.place_name),
		   place(that.place)
	{  }

	/* Compares only the content, not the place. */ 
	bool operator == (const Place_Param_Target &that) const {
		return this->flags == that.flags && 
			this->place_name == that.place_name; 
	}

	string format(Style style, bool &quotes) const {
		Target target(flags, place_name.raw()); 
		return target.format(style, quotes); 
	}
	
	string format_word() const {
		Target target(flags, place_name.raw());
		return target.format_word(); 
	}
	
	string format_out() const {
		Target target(flags, place_name.raw());
		return target.format_out(); 
	}

	string format_src() const {
		Target target(flags, place_name.raw());
		return target.format_src(); 
	}

	shared_ptr <Place_Param_Target> 
	instantiate(const map <string, string> &mapping) const {
		return make_shared <Place_Param_Target> 
			(flags, *place_name.instantiate(mapping), place); 
	}

	Target unparametrized() const {
		return Target(flags, place_name.unparametrized()); 
	}

	Param_Target get_param_target() const {
		return Param_Target(flags, place_name); 
	}
};

string Target::format(Style style, bool &quotes) const
{
	Style style2= 0;
	if (! is_file()) {
		style2 |= S_MARKERS;
	} else {
		style2 |= style; 
	}
	string ret; 
	size_t i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		assert((get_word(i) & F_TARGET_TRANSIENT) == 0); 
		if (!(style & S_NOFLAGS)) {
			ret += flags_format(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT)); 
		}
		++i;
		ret += '[';
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
	if (!(style & S_NOFLAGS)) {
		ret += flags_format(get_word(i) & ~(F_TARGET_TRANSIENT | F_VARIABLE)); 
	}
	if (get_word(i) & F_TARGET_TRANSIENT) {
		ret += '@'; 
	}
	bool quotes_inner= false;
	bool detached= is_dynamic() || is_transient(); 
	if (! detached)
		quotes_inner= quotes; 
	string s= name_format(text.substr(sizeof(word_t) * (i + 1)), style2, quotes_inner); 
	if (! detached)
		quotes |= quotes_inner; 
	ret += s; 
	i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		++i;
		ret += ']';
	}
	return ret; 
}

string Target::format_out() const
{
	Style style= 0;
	if (! is_file()) {
		style |= S_MARKERS;
	}
	bool quotes= is_file();
	string ret; 
	size_t i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		ret += flags_format(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT));
		++i;
		ret += '[';
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
	ret += flags_format(get_word(i) & ~(F_TARGET_TRANSIENT | F_VARIABLE)); 
	if (get_word(i) & F_TARGET_TRANSIENT) {
		ret += '@'; 
	}
	string name_text = name_format(text.substr(sizeof(word_t) * (i + 1)), style, quotes); 
	if (quotes)  ret += '\'';
	ret += name_text; 
	if (quotes)  ret += '\'';
	i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		++i;
		ret += ']';
	}
	return ret; 
}

string Target::format_out_print_word() const
{
	Style style= 0;
	if (! is_file()) {
		style |= S_MARKERS;
	}
	bool quotes= is_file() ? Color::quotes : false;
	string ret; 
	ret += Color::out_print_word; 
	size_t i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		ret += flags_format(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT));
		++i;
		ret += '[';
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
	ret += flags_format(get_word(i) & ~(F_TARGET_TRANSIENT | F_VARIABLE)); 
	if (get_word(i) & F_TARGET_TRANSIENT) {
		ret += '@'; 
	}
	string name_text= name_format(text.substr(sizeof(word_t) * (i + 1)), style, quotes); 
	if (quotes)  ret += '\'';
	ret += name_text; 
	if (quotes)  ret += '\'';
	i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		++i;
		ret += ']';
	}
	ret += Color::out_print_word_end;
	return ret; 
}

string Target::format_word() const
{
	Style style= 0;
	if (! is_file()) {
		style |= S_MARKERS;
	}
	bool quotes= is_file() ? Color::quotes : false;
	string ret; 
	ret += Color::word; 
	size_t i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		++i;
		ret += '[';
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
	if (get_word(i) & F_TARGET_TRANSIENT) {
		ret += '@'; 
	}
	string name_text= name_format(text.substr(sizeof(word_t) * (i + 1)), style, quotes); 
	if (quotes)  ret += '\'';
	ret += name_text;
	if (quotes)  ret += '\'';
	i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		++i;
		ret += ']';
	}
	ret += Color::end;
	return ret; 
}

string Target::format_src() const
{
	Style style= 0;
	if (! is_file()) {
		style |= S_MARKERS;
	}
	string ret; 
	size_t i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		ret += flags_format(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT));
		++i;
		ret += '[';
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
	ret += flags_format(get_word(i) & ~F_TARGET_TRANSIENT); 
	if (get_word(i) & F_TARGET_TRANSIENT) {
		ret += '@'; 
	}
	const char *const name= text.c_str() + sizeof(word_t) * (i + 1);
	bool quotes= src_need_quotes(name); 
	string name_text= name_format(text.substr(sizeof(word_t) * (i + 1)), style, quotes); 
	if (quotes)  ret += '\'';
	ret += name_text; 
	if (quotes)  ret += '\'';
	i= 0;
	while (get_word(i) & F_TARGET_DYNAMIC) {
		++i;
		ret += ']';
	}
	return ret; 
}

string Name::instantiate(const map <string, string> &mapping) const
{
	assert(texts.size() == 1 + parameters.size()); 

	const int n= get_n(); 

	string ret= texts[0];

	for (int i= 0;  i < n;  ++i) {
		assert(parameters[i].size() > 0); 
		ret += mapping.at(parameters[i]);
		ret += texts[i + 1];
	}

	return ret; 
}

bool Name::match(const string name, 
		 map <string, string> &mapping,
		 vector <size_t> &anchoring) const
{
	/* 
	 * Rules:
	 *  - Each parameter must match at least one character. 
	 */

	/* This algorithm uses one pass without backtracking or
	 * recursion.  Therefore, there are no "deadly" patterns that
	 * can make it hang, which is a common source of errors for
	 * naive trivial implementations of regular expression
	 * matching.  */

	assert(mapping.size() == 0); 

	map <string, string> ret;

	const size_t n= get_n(); 

	if (name == "") {
		return n == 0 && texts[0] == ""; 
	}

	anchoring.resize(2 * n);

	const char *const p_begin= name.c_str();
	const char *p= p_begin;
	const char *const p_end= name.c_str() + name.size(); 

	int k= texts[0].size(); 

	if (p_end - p <= k) {
		return false;
	}

	if (memcmp(p, texts[0].c_str(), k)) {
		return false;
	}

	p += k; 

	anchoring[0]= k;

	for (size_t i= 0;  i < n;  ++i) {

		if (i == n - 1) {
			/* For the last segment, the texts[n-1] must
			 * match the end of the input string */ 

			size_t size_last= texts[n].size();

			if (p_end - p < 1 + (ssize_t) size_last) 
				return false;

			if (memcmp(p_end - size_last, texts[n].c_str(), size_last))
				return false;

			ret[parameters[i]]= string(p, p_end - p - size_last); 
			anchoring[2*i + 1]= p_end - size_last - p_begin;

			assert(ret.at(parameters[i]).size() > 0); 

		} else {
			/* Intermediate texts must not be empty, i.e.,
			 * two parameters cannot be unseparated */ 
			assert(texts[i+1].size() != 0); 

			const char *q= strstr(p+1, texts[i+1].c_str());

			if (q == nullptr) 
				return false;

			assert(q > p);
			
			anchoring[i * 2 + 1]= q - p_begin; 

			ret[parameters[i]]= string(p, q-p);
			p= q + texts[i+1].size();

			anchoring[i * 2 + 2]= p - p_begin; 
		}
	}

	swap(mapping, ret); 

	assert(anchoring.size() == 2 * n); 

	return true;
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
		if (texts[i] == "") {
			param_1= parameters[i-1];
			param_2= parameters[i];
			return false;
		}
	}

	return true;
}

bool Name::anchoring_dominates(vector <size_t> &anchoring_a,
			       vector <size_t> &anchoring_b)
{
 	/* (A) dominates (B) when every character in a parameter in (A)
	 * is also in a parameter in (B) and at least one character is
	 * not parametrized in (A) but in (B). */

	/* CORRESPONDING TEST: anchoring */ 

	assert(anchoring_a.size() % 2 == 0);
	assert(anchoring_b.size() % 2 == 0);

	const size_t k_a= anchoring_a.size();
	const size_t k_b= anchoring_b.size();

	bool dominate= false;
	size_t p= 0; /* Position in the string */ 
	size_t i= 0; /* Index in (A) */ 
	size_t j= 0; /* Index in (B) */ 

	for (;;) {
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
		if (i == k_a && j == k_b)
			return dominate; 
		else if (i < k_a && j == k_b)
			p= anchoring_a[i];
		else if (j < k_b && i == k_a)
			p= anchoring_b[j]; 
		else if (i < k_a && j < k_b) 
			p= min(anchoring_a[i], anchoring_b[j]); 
	}
}

#endif /* ! TARGET_HH */
