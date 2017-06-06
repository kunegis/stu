#ifndef TARGET_HH
#define TARGET_HH

#include "type.hh"
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
 *     * A _name_ is a filename or the name of a transient target.  They are
 *       just strings, so no special data type for it.  There are two
 *       distinct namespaces for them. 
 *     * A _target_ is either file, transient target, or a dynamic
 *       variant of them.  It is represented by a name (string) and a
 *       type.  
 *     * A _parametrized_ target or name additionally can have
 *       parameters. 
 *     * Dedicated classes exist to represent these with _places_. 
 */

/* 
 * The basic object in Stu:  a file, a variable, or a dynamic version of these.  This
 * consists of a name together with a type.  This class is not
 * parametrized, and does not contain a place object.  It is used as
 * keys in maps. 
 */ 
// TODO deprecate:  This was only used for indexing Execution objects,
// and is now replaced by Target2. 
class Target
{
public:

	Type type;
	string name;

	Target(Type type_)
		:  type(type_),
		   name("")
		{ }

	Target(Type type_, string name_)
		:  type(type_),
		   name(name_)
		{ }

	string format(Style style, bool &quotes) const {

		quotes= false;

		Style style2= 0;
		if (type != Type::FILE)
			style2 |= S_MARKERS;
		else 
			style2 |= style; 

		bool quotes2= false;

		string text= name_format(name, style2, quotes2); 

		quotes= false;
		
		return fmt("%s%s%s%s%s%s", 
			   string(type.get_depth(), '['),
			   type.is_any_transient() ? "@" : "",
			   quotes2 ? "'" : "",
			   text,
			   quotes2 ? "'" : "",
			   string(type.get_depth(), ']'));
	}

	string format_word() const {

		Style style= 0;
		if (type != Type::FILE)
			style |= S_MARKERS;

		bool quotes= 
			(type == Type::FILE
			 ? Color::quotes
			 : false);

		string text= name_format(name, style, quotes); 
		
		return fmt("%s%s%s%s%s%s%s%s", 
			   Color::word, 
			   string(type.get_depth(), '['),
			   type.is_any_transient() ? "@" : "",
			   quotes ? "'" : "",
			   text,
			   quotes ? "'" : "",
			   string(type.get_depth(), ']'),
			   Color::end);
	}

	string format_out_print_word() const {

		Style style= 0;
		if (type != Type::FILE)
			style |= S_MARKERS;

		bool quotes= 
			(type == Type::FILE
			 ? Color::quotes_out
			 : 0);

		string text= name_format(name, style, quotes); 
		
		return fmt("%s%s%s%s%s%s%s%s", 
			   Color::out_print_word, 
			   string(type.get_depth(), '['),
			   type.is_any_transient() ? "@" : "",
			   quotes ? "'" : "",
			   text,
			   quotes ? "'" : "",
			   string(type.get_depth(), ']'),
			   Color::out_print_word_end);
	}

	string format_out() const {

		Style style= 0;
		if (type != Type::FILE)
			style |= S_MARKERS;

		bool quotes= type == Type::FILE;
		
		string text= name_format(name, style, quotes); 

		return fmt("%s%s%s%s%s%s", 
			   string(type.get_depth(), '['),
			   type.is_any_transient() ? "@" : "",
			   quotes ? "'" : "",
			   text,
			   quotes ? "'" : "",
			   string(type.get_depth(), ']')); 
	}

	string format_src() const {

		Style style= 0;
		if (type != Type::FILE)
			style |= S_MARKERS;

		bool quotes= src_need_quotes(name); 
		
		string text= name_format(name, style, quotes); 

		return fmt("%s%s%s%s%s%s", 
			   string(type.get_depth(), '['),
			   type.is_any_transient() ? "@" : "",
			   quotes ? "'" : "",
			   text,
			   quotes ? "'" : "",
			   string(type.get_depth(), ']')); 
	}

	bool operator== (const Target &target) const {
		return this->type == target.type &&
			this->name == target.name;
	}
	
	bool operator< (const Target &target) const {
		return this->type < target.type ||
			(this->type == target.type && this->name < target.name);
	}
};

/* A hash function for Target objects, because they are used as keys in
 * containers.  */
namespace std {
	template <>
	struct hash <Target>
	{
		size_t operator()(const Target &target) const {
			return
				hash <string> ()(target.name)
				^ target.type.get_value_for_hash();
		}
	};
}

/* 
 * A representation of single dependency, as well as dyn^* of single
 * dependencies, mainly used as the key in the caching of Execution
 * objects. 
 */
// TODO rename 'Target' after class Target is removed 
class Target2
{
public:

	Target2(Type type, string name) 
	/* Non-dynamic */
		: text(((char) type.get_value()) + name)
	{
		assert(type.is_file() || type.is_transient()); 
	}

	const string &get_text() const {  return text;  }

	bool is_dynamic() const {
		assert(text.size() >= 2); 
		return (text.at(0) & F_DYNAMIC_TARGET) != 0; 
	}

	bool is_file() const {
		assert(text.size() >= 2); 
		return (text.at(0) & (F_DYNAMIC_TARGET | Type::T_MASK_FILE_TRANSIENT)) == Type::T_FILE; 
	}

	bool is_transient() const {
		assert(text.size() >= 2); 
		return (text.at(0) & (F_DYNAMIC_TARGET | Type::T_MASK_FILE_TRANSIENT)) == Type::T_TRANSIENT;
	}

	string format_out() const;
	string format_word() const;
	string format_src() const;

	string get_nondynamic_name() const 
	{
		assert(text.size() >= 2);
		assert((text.at(0) & F_DYNAMIC_TARGET) == 0); 
		return text.substr(1); 
	}
	
	const char *get_nondynamic_name_c_str() const 
	/*
	 * Return a C pointer to the name of the file or transient.  The
	 * object must be non-dynamic. 
	 */
	{
		assert(text.size() >= 2);
		assert((text.at(0) & F_DYNAMIC_TARGET) == 0); 
		return text.c_str() + 1; 
	}

	unsigned get_front_byte() const {  return text.at(0);  }

	bool operator== (const Target2 target2) const {  return text == target2.text;  }

private:

	string text; 
	/*
	 * Linear representation of the dependency.
	 *
	 * A non-dynamic dependency is represented as a Type byte (\002 or \004)
	 * followed by the name. 
	 *
	 * A dynamic is represented as a dynamic byte (\001 & flags),
	 * following by the string representation of the contained
	 * dependency. 
	 */
};

namespace std {
	template <>
	struct hash <Target2>
	{
		size_t operator()(const Target2 &target) const {
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
	unsigned get_n() const {
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

		for (unsigned i= 0;  i < name.get_n();  ++i) {
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
		   vector <unsigned> &anchoring);
	/* Check whether NAME matches this name.  If it does, return
	 * TRUE and set MAPPING and ANCHORING accordingly. 
	 * MAPPING must be empty.  */
	
	/* No escape characters */
	string raw() const {
		assert(texts.size() == 1 + parameters.size()); 
		string ret= texts[0];
		for (unsigned i= 0;  i < get_n();  ++i) {
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

		for (unsigned i= 0;  i < get_n();  ++i) {
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
		for (unsigned i= 0;  i < get_n();  ++i) {
			if (this->parameters[i] != that.parameters[i])
				return false;
			if (this->texts[i] != that.texts[i])
				return false;
		}
		if (this->texts[get_n()] != that.texts[get_n()])
			return false;
		return true;
	}

	static bool anchoring_dominates(vector <unsigned> &anchoring_a,
					vector <unsigned> &anchoring_b);
	/* Whether anchoring A dominates anchoring B.  The anchorings do
	 * not need to have the same number of parameters.  */
};

/* 
 * A parametrized name for which it is saved what type it represents.  Non-dynamic. 
 */ 
class Param_Target
{
public:

	Type type;
	/* Non-dynamic */ 

	Name name; 
 
	Param_Target(Type type_,
		     const Name &name_)
		:  type(type_),
		   name(name_)
	{ }

	/* Unparametrized target */
	Param_Target(const Target &target)
		:  type(target.type),
		   name(target.name)
	{ }

	Target instantiate(const map <string, string> &mapping) const {
		return Target(type, name.instantiate(mapping)); 
	}

	string format_word() const {

		Style style= 0;
		if (type != Type::FILE)
			style |= S_MARKERS;

		bool quotes2= 
			(type == Type::FILE
			 ? Color::quotes
			 : 0);

		string text= name.format(style, quotes2); 
		
		return fmt("%s%s%s%s%s%s%s%s", 
			   Color::word, 
			   string(type.get_depth(), '['),
			   type.is_any_transient() ? "@" : "",
			   quotes2 ? "'" : "",
			   text,
			   quotes2 ? "'" : "",
			   string(type.get_depth(), ']'),
			   Color::end);
	}

	/* The corresponding unparametrized target.  This target must
	 * have zero parameters. */ 
	Target unparametrized() const {
		return Target(type, name.unparametrized());
	}

	bool operator == (const Param_Target &that) const {
		return this->type == that.type &&
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

	/* Empty parametrized name, and empty place */
	Place_Name() 
		:  Name(),
		   place()
	{ }

	/* Unparametrized, with empty place */ 
	Place_Name(string name)
		:  Name(name)
	{ 
		/* PLACES remains empty */ 
	}
	
	/* Unparametrized, with explicit place */
	Place_Name(string name, const Place &_place) 
		:  Name(name), place(_place)
	{
		assert(! place.empty()); 
	}

	const vector <Place> &get_places() const {
		return places;
	}

	/* Append the given PARAMETER and an empty text */ 
	void append_parameter(string parameter,
			      const Place &place_parameter) {
		Name::append_parameter(parameter); 
		places.push_back(place_parameter);
	}

	shared_ptr <Place_Name> instantiate(const map <string, string> &mapping) const {
		/* In the returned object, the PLACES vector is empty */ 
		string name= Name::instantiate(mapping);
		return make_shared <Place_Name> (name);
	}
};

/* 
 * A target that is parametrized and contains places.  Non-dynamic. 
 */
class Place_Param_Target
{
public:

	Type type; 
	/* Non-dynamic */ 

	Place_Name place_name;

	Place place;
	/* The place of the target as a whole.  The PLACE_NAME
	 * variable additionally contains a place for the name itself,
	 * as well as for individual parameters.  */ 

	Place_Param_Target(Type type_,
			   const Place_Name &place_name_)
		:  type(type_),
		   place_name(place_name_),
		   place(place_name_.place)
	{ }

	Place_Param_Target(Type type_,
			   const Place_Name &place_name_,
			   const Place &place_)
		:  type(type_),
		   place_name(place_name_),
		   place(place_)
	{ }

	/* Compares only the content, not the place. */ 
	bool operator == (const Place_Param_Target &that) const {
		return this->type == that.type &&
			this->place_name == that.place_name; 
	}

	string format(Style style, bool &need_quotes) const {
		Target target(type, place_name.raw());
		return target.format(style, need_quotes); 
	}
	
	string format_word() const {
		Target target(type, place_name.raw());
		return target.format_word(); 
	}
	
	string format_out() const {
		Target target(type, place_name.raw());
		return target.format_out(); 
	}

	shared_ptr <Place_Param_Target> 
	instantiate(const map <string, string> &mapping) const {
		return make_shared <Place_Param_Target> 
			(type, *place_name.instantiate(mapping), place); 
	}

	Target2 unparametrized() const {
		return Target2(type, place_name.unparametrized()); 
	}

	Param_Target get_param_target() const {
		return Param_Target(type, place_name); 
	}
};

string Target2::format_out() const
{
	Style style= 0;
	if (! is_file()) {
//	if (type != Type::FILE) {
		style |= S_MARKERS;
	}
	bool quotes= is_file();
	string ret; 
	size_t i= 0;
	while (text.at(i) & F_DYNAMIC_TARGET) {
		ret += flags_format(text.at(i) & ~F_DYNAMIC_TARGET);
		++i;
		ret += '[';
	}
	assert(text.size() > i + 1);
	ret += flags_format(text.at(i)); 
	if ((text.at(i) & Type::T_MASK_FILE_TRANSIENT) == Type::T_TRANSIENT) {
		ret += '@'; 
	}
	if (quotes)  ret += '\'';
	ret += name_format(text.substr(i+1), style, quotes); 
	if (quotes)  ret += '\'';
	i= 0;
	while (text.at(i) & F_DYNAMIC_TARGET) {
		++i;
		ret += ']';
	}
	return ret; 
}

string Target2::format_word() const
{
	Style style= 0;
	if (! is_file()) {
		style |= S_MARKERS;
	}
	bool quotes= is_file() ? Color::quotes : false;
	string ret; 
	ret += Color::word; 
	size_t i= 0;
	while (text.at(i) & F_DYNAMIC_TARGET) {
		ret += flags_format(text.at(i) & ~F_DYNAMIC_TARGET);
		++i;
		ret += '[';
	}
	assert(text.size() > i + 1);
	ret += flags_format(text.at(i)); 
	if ((text.at(i) & Type::T_MASK_FILE_TRANSIENT) == Type::T_TRANSIENT) {
		ret += '@'; 
	}
	if (quotes)  ret += '\'';
	ret += name_format(text.substr(i+1), style, quotes); 
	if (quotes)  ret += '\'';
	i= 0;
	while (text.at(i) & F_DYNAMIC_TARGET) {
		++i;
		ret += ']';
	}
	ret += Color::end;
	return ret; 
}

string Target2::format_src() const
{
	Style style= 0;
	if (! is_file()) {
		style |= S_MARKERS;
	}
	string ret; 
	size_t i= 0;
	while (text.at(i) & F_DYNAMIC_TARGET) {
		ret += flags_format(text.at(i) & ~F_DYNAMIC_TARGET);
		++i;
		ret += '[';
	}
	assert(text.size() > i + 1);
	ret += flags_format(text.at(i)); 
	if ((text.at(i) & Type::T_MASK_FILE_TRANSIENT) == Type::T_TRANSIENT) {
		ret += '@'; 
	}
	const char *const name= text.c_str() + i + 1;
	bool quotes= src_need_quotes(name); 
	if (quotes)  ret += '\'';
	ret += name_format(text.substr(i+1), style, quotes); 
	if (quotes)  ret += '\'';
	i= 0;
	while (text.at(i) & F_DYNAMIC_TARGET) {
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
		 vector <unsigned> &anchoring)
{
	/* 
	 * Rules:
	 *  - Each parameter must match at least one character. 
	 */

	/* This algorithm uses one pass without backtracking or
	 * recursion.  Therefore, there are no "deadly" patterns that
	 * can make it hang, as is the case for a trivial implementation
	 * or regular expression matching.  */

	assert(mapping.size() == 0); 

	map <string, string> ret;

	const unsigned n= get_n(); 

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

	for (unsigned i= 0;  i < n;  ++i) {

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

	for (unsigned i= 1;  i + 1 < get_n() + 1;  ++i) {
		if (texts[i] == "") {
			param_1= parameters[i-1];
			param_2= parameters[i];
			return false;
		}
	}

	return true;
}

bool Name::anchoring_dominates(vector <unsigned> &anchoring_a,
			       vector <unsigned> &anchoring_b)
{
 	/* (A) dominates (B) when every character in a parameter in (A)
	 * is also in a parameter in (B) and at least one character is
	 * not parametrized in (A) but in (B). */

	/* CORRESPONDING TEST: anchoring */ 

	assert(anchoring_a.size() % 2 == 0);
	assert(anchoring_b.size() % 2 == 0);

	const unsigned k_a= anchoring_a.size();
	const unsigned k_b= anchoring_b.size();

	bool dominate= false;
	unsigned p= 0; /* Position in the string */ 
	unsigned i= 0; /* Index in (A) */ 
	unsigned j= 0; /* Index in (B) */ 

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
