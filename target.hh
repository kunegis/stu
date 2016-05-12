#ifndef TARGET_HH
#define TARGET_HH

/* Data types for representing filenames, targets, etc. 
 */

#include <unordered_set>

/* Glossary:
 *     * A _name_ is a filename of the name of a transient target.  They are
 *       just strings, so no special data type for it
 *     * A _target_ is either file or a transient, or a dynamic file.  It is
 *       represented by a name (string) and a type (integer). 
 *     * A _parametrized_ target or name additionally can have
 *       parameters
 *     * Dedicated classes exist to represent these with _places_. 
 */

#define STU_STRING_ESCAPE_CHARACTERS "\"\' \t\n\v\f\r\a\b\\\?"
#define STU_STRING_ESCAPE_CHARACTERS_STRICT "\"\'\t\n\v\f\r\a\b\\\?"

string format_name(string name) 
{
	string ret(2 + 2 * name.size(), '\0');
	char *const q_begin= (char *) ret.c_str(), *q= q_begin; 
	*q++= '\'';
	for (const char *p= name.c_str();  *p;  ++p) {
		if (strchr(STU_STRING_ESCAPE_CHARACTERS_STRICT, *p)) {
			*q++= '\\';
			switch (*p) {
			default:  assert(0);
			case '\a':  *q++= 'a';  break;
			case '\b':  *q++= 'b';  break;
			case '\f':  *q++= 'f';  break;
			case '\n':  *q++= 'n';  break;
			case '\r':  *q++= 'r';  break;
			case '\t':  *q++= 't';  break;
			case '\v':  *q++= 'v';  break;
			case '\\':  *q++= '\\'; break;
			case '\'':  *q++= '\''; break;
			case '\"':  *q++= '\"'; break;
			case '\?':  *q++= '?';  break;
			}
		} else {
			*q++= *p;
		}
	}
	*q++= '\'';
	ret.resize(q - q_begin); 
	return ret; 
}

string format_name_mid(string name)
{
	if (name.find_first_of(STU_STRING_ESCAPE_CHARACTERS) != string::npos) 
		return format_name(name); 
	else
		return name;
}

class Target;

class Type
{
private:  

	/* >= T_ROOT */
	int value;

	friend class Target; 
	friend class std::hash <Target> ;

	/* Only used for hashing */
	int get_value() const {
		return value; 
	}

	/* Only used by containers */
	bool operator < (const Type &type) const {
		return this->value < type.value; 
	}

	enum: int {
		/* A transient target */ 
		T_TRANSIENT     = 0,
	
		/* A file in the file system; this entry has to come before
		 * T_DYNAMIC because it counts also as a dynamic dependency of
		 * depth zero. */
		T_FILE          = 1,

		/* A dynamic transient target -- only used for the Target object of
		 * executions */
		T_DYNAMIC_TRANSIENT = 2,

		/* A dynamic target -- only used for the Target object of executions */   
		T_DYNAMIC_FILE  = 3

		/* Larger values denote multiply dynamic targets.  They are only
		 * used as the target of Execution objects.  Therefore, T_DYNAMIC is
		 * always last in this enum. */
	
		/* Note:  all dynamic targets are files, and therefore T_FILE can be
		 * thought of as a dynamic target of depth zero. */
	};

	Type(int value_)
		:  value(value_)
	{
		assert(value >= 0);
	}

public:

	static const Type TRANSIENT, FILE, DYNAMIC_TRANSIENT, DYNAMIC_FILE;

	bool is_dynamic() const {
		return value >= T_DYNAMIC_TRANSIENT;
	}

	unsigned get_dynamic_depth() const {
		assert(value >= 0);
		return (value - T_TRANSIENT) >> 1;
	}

	bool is_any_transient() const {
		assert(value >= 0); 
		return ! (value & 1);
	}

	bool is_any_file() const {
		assert(value >= 0); 
		return value & 1;
	}

	Type get_base() const {
		assert(value >= 0); 
		return Type(value & 1); 
	}

	bool operator == (const Type &type) const {
		return this->value == type.value;
	}

	bool operator == (int value_) const {
		return this->value == value_; 
	}

	bool operator != (const Type &type) const {
		return this->value != type.value;
	}

	bool operator != (int value_) const {
		return this->value != value_; 
	}

	int operator - (const Type &type) const {
		assert(this->value >= T_TRANSIENT);
		assert(type .value >= T_TRANSIENT);

		/* We can only subtract compatible types */ 
		assert(((this->value ^ type.value) & 1) == 0);

		return (this->value - type.value) / 2; 
	}

	Type operator - (int diff) const {
		assert(value >= 0); 
		assert(value - 2 * diff >= 0); 
		return Type(value - 2 * diff);
	}

	Type operator ++ () {
		assert(value >= 0);
		value += 2;
		return *this; 
	}

	Type operator -- () {
		assert(value >= T_DYNAMIC_TRANSIENT);
		value -= 2;
		return *this;
	}

	Type &operator += (int diff) {
		assert(value >= 0);
		value += 2 * diff;
		assert(value >= 0);
		return *this;
	}
};

const Type Type::TRANSIENT(Type::T_TRANSIENT);
const Type Type::FILE(Type::T_FILE);
const Type Type::DYNAMIC_TRANSIENT(Type::T_DYNAMIC_TRANSIENT);
const Type Type::DYNAMIC_FILE(Type::T_DYNAMIC_FILE);

/* 
 * The basic object in Stu:  a file, a variable or a transient target.  This
 * consists of a name together with a type.  This class is not
 * parametrized, and does not contain a place object.  It is used as
 * keys in maps. 
 */ 
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

	/* Used in output of Stu, i.e., mainly in error messages.  */ 
	string format() const {

		if (type.is_any_transient()) {
			return "@" + (string(type.get_dynamic_depth(), '[') 
				      + format_name_mid(name) 
				      + string(type.get_dynamic_depth(), ']'));
		} else if (type == Type::FILE) {
			return format_name(name); 
		} else {
			assert(type.is_any_file()); 
			assert(type.is_dynamic()); 
			return string(type.get_dynamic_depth(), '[') 
				+ format_name_mid(name) 
				+ string(type.get_dynamic_depth(), ']');
		}
	}

	string format_bare() const {

		return 
			string(type - Type::FILE, '[') 
			+ (type.is_any_transient() ? "@" : "")
			+ name
			+ string(type - Type::FILE, ']');
	}

	string format_mid() const {

		return 
			string(type - Type::FILE, '[') 
			+ (type.is_any_transient() ? "@" : "")
			+ format_name_mid(name) 
			+ string(type - Type::FILE, ']'); 
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

namespace std {

	template <>
	struct hash <Target>
	{
		size_t operator()(const Target &target) const {
			return
				hash <string> ()(target.name)
				^ target.type.get_value();
		}
	};

}

/* A parametrized name.  Each name has N >= 0 parameters.  When N > 0,
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
class Param_Name
{
private:

	/* Length = N + 1.
	 * Only the first and last elements may be empty.  
	 */ 
	vector <string> texts; 

	/* Length = N.  
	 */ 
	vector <string> parameters;

public:

	/* A name with zero parameters */ 
	Param_Name(string name_)
		:  texts({name_})
	{ }

	/* Empty name */ 
	Param_Name()
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

	const vector <string> &get_texts() const {
		return texts; 
	}

	const vector <string> &get_parameters() const {
		return parameters; 
	}

	/* Append the given PARAMETER and an empty text.  This does not
	 * check that the result is valid. 
	 */ 
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
	void append(const Param_Name &param_name) {
		assert(this->texts.back() != "" ||
		       param_name.texts.back() != "");

		append_text(param_name.texts.front());

		for (unsigned i= 0;  i < param_name.get_n();  ++i) {
			append_parameter(param_name.get_parameters()[i]);
			append_text(param_name.get_texts()[1 + i]);
		}
	}

	string &last_text() {
		return texts[texts.size() - 1];
	}

	const string &last_text() const {
		return texts[texts.size() - 1];
	}

	/* The name may be empty, resulting in an empty string */ 
	string instantiate(const map <string, string> &mapping) const;

	/* Return the unparametrized name.  The name must be unparametrized. 
	 */
	const string &unparametrized() const {
		assert(get_n() == 0);
		return texts[0]; 
	}

	/* Check whether NAME matches this name.  If it does, return
	 * TRUE and set MAPPING and ANCHORING accordingly. 
	 */
	bool match(string name, 
		   map <string, string> &mapping,
		   vector <int> &anchoring);
	
	/* The canonical string representation; unquoted */ 
	string format_bare() const {
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

	string format_mid() const {
		assert(texts.size() == 1 + parameters.size()); 
		string ret= format_name_mid(texts[0]);
		for (unsigned i= 0;  i < get_n();  ++i) {
			ret += "${";
			ret += parameters[i];
			ret += '}';
			ret += format_name_mid(texts[1+i]);
		}
		return ret; 
	}

	string format() const {
		return fmt("'%s'", format_bare()); 
	}

	/* Return whether there are duplicate parameter names.
	 * If TRUE, write the first found into PARAMETER. 
	 */
	bool has_duplicate_parameters(string &parameter) const;

	/* Whether this is a valid name */
	bool valid() const;

	bool operator == (const Param_Name &that) const {
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

	/* Whether anchoring A dominates anchoring B. 
	 * The anchorings do not need to have the same number of parameters. 
	 */
	static bool anchoring_dominates(vector <int> &anchoring_a,
					vector <int> &anchoring_b);
};

/* A parametrized name for which it is saved what type it represents.  
 */ 
class Param_Target
{
public:

	Type type;
	Param_Name param_name; 
 
	Param_Target(Type type_,
		     const Param_Name &param_name_)
		:  type(type_),
		   param_name(param_name_)
	{ }

	/* Unparametrized target */
	Param_Target(const Target &target)
		:  type(target.type),
		   param_name(target.name)
	{ }

	Target instantiate(const map <string, string> &mapping) const {
		return Target(type, param_name.instantiate(mapping)); 
	}

	string format() const {
		return Target(type, param_name.format_bare()).format(); 
	}

	/* The corresponding unparametrized target.  This target must
	 * have zero parameters. */ 
	Target unparametrized() const {
		return Target(type, param_name.unparametrized());
	}

	bool operator == (const Param_Target &that) const {
		return this->type == that.type &&
			this->param_name == that.param_name; 
	}

	bool operator != (const Param_Target &that) const {
		return ! (*this == that);
	}
};

class Place_Param_Name
	:  public Param_Name
{
public:

	/* Place of the name as a whole */ 
	Place place;

	/* length = n (number of parameters). 
	 * The places of the individual parameters.  
	 */
	vector <Place> places;

	/* Empty parametrized name, and empty place */
	Place_Param_Name() 
		:  Param_Name(),
		   place()
	{ }

	/* A name with zero parameters, and hence no places.  No place as a
	 * whole. */ 
	Place_Param_Name(string name)
		:  Param_Name(name)
	{ 
		/* PLACES remains empty */ 
	}

	Place_Param_Name(string name, const Place &_place) 
		:  Param_Name(name), place(_place)
	{ }

	const vector <Place> &get_places() const {
		return places;
	}

	/* Append the given PARAMETER and an empty text */ 
	void append_parameter(string parameter,
			      const Place &place_parameter) {
		Param_Name::append_parameter(parameter); 
		places.push_back(place_parameter);
	}

	shared_ptr <Place_Param_Name> instantiate(const map <string, string> &mapping) const {
		/* In the returned object, the PLACES vector is empty */ 
		string name= Param_Name::instantiate(mapping);
		return make_shared <Place_Param_Name> (name);
	}
};

/* A target that is parametrized and contains places 
 */
class Place_Param_Target
{
public:

	Type type; 

	Place_Param_Name place_param_name;

	/* The place of the target as a whole.  The PLACE_PARAM_NAME
	 * variable additionally contains places for each parameter. */ 
	Place place;

	Place_Param_Target(Type type_,
			   const Place_Param_Name &place_param_name_)
		:  type(type_),
		   place_param_name(place_param_name_),
		   place(place_param_name_.place)
	{ }

	Place_Param_Target(Type type_,
			   const Place_Param_Name &place_param_name_,
			   const Place &place_)
		:  type(type_),
		   place_param_name(place_param_name_),
		   place(place_)
	{ }

	string format() const {
		return Target(type, place_param_name.format_bare()).format(); 
	}

	string format_mid() const {
		return Target(type, place_param_name.format_bare()).format_mid(); 
	}

	shared_ptr <Place_Param_Target> 
	instantiate(const map <string, string> &mapping) const {
		return make_shared <Place_Param_Target> 
			(type, *place_param_name.instantiate(mapping)); 
	}

	Target unparametrized() const {
		return Target(type, place_param_name.unparametrized()); 
	}

	Param_Target get_param_target() const {
		return Param_Target(type, place_param_name); 
	}
};

string Param_Name::instantiate(const map <string, string> &mapping) const
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

bool Param_Name::match(const string name, 
		       map <string, string> &mapping,
		       vector <int> &anchoring)
{
	/* Rules:
	 *  - Each parameter must match at least one character. 
	 */

	/* The algorithm uses one pass without backtracking or
	 * recursion.  Therefore, there are no "deadly" patterns that
	 * can make it hang, as it is the case for trivial
	 * implementation of regular expression matching. 
	 */

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

			if (p_end - p < 1 + (int) size_last) 
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

	mapping.insert(ret.begin(), ret.end());

	assert(anchoring.size() == 2 * n); 

	return true;
}

bool Param_Name::has_duplicate_parameters(string &parameter) const
{
	unordered_set <string> seen;

	for (auto &i:  parameters) {
		if (seen.count(i)) {
			parameter= i; 
			return true;
		}
		seen.insert(i); 
	}
	
	return false;
}

bool Param_Name::valid() const 
{
	if (empty())  
		return false;

	for (unsigned i= 1;  i + 1 < get_n() + 1;  ++i) {
		if (texts[i] == "")
			return false;
	}

	return true;
}

bool Param_Name::anchoring_dominates(vector <int> &anchoring_a,
				     vector <int> &anchoring_b)
{
 	/* (A) dominates (B) when every character in a parameter in (A)
	 * is also in a parameter in (B) and at least one character is
	 * not parametrized in (A) but in (B). */

	assert(anchoring_a.size() % 2 == 0);
	assert(anchoring_b.size() % 2 == 0);

	const unsigned n_a= anchoring_a.size() / 2;
	const unsigned n_b= anchoring_b.size() / 2;

	/* Set to TRUE when characters are found that are parametrized
	 * in (B) but not in (A). */
	bool dominate= false;

	/* The following code returns with FALSE when a character is
	 * found that is parametrized in (A) but not in (B). 
	 */

	unsigned i_b= 0; 
	for (unsigned i_a= 0;  i_a < n_a;  ++i_a) {

		/* Parameter in (B) ends before that in (A) begins */ 
		while (i_b < n_b && anchoring_b[2 * i_b + 1] <= anchoring_a[2 * i_a]) {
			++ i_b;
			dominate= true; 
		}

		/* The current parameter in (A) cannot be matched
		 * anymore */ 
		if (i_b == n_b)
			return false;

		/* The parameter in (A) begins before that in (B) */
		if (anchoring_a[2 * i_a] < anchoring_b[2 * i_b])
			return false;

		/* The parameter in (A) begins after that in (B) */ 
		if (anchoring_a[2 * i_a] > anchoring_b[2 * i_b])
			dominate= true;

		/* The parameter in (A) ends after that in (B) */
		if (anchoring_a[2 * i_a + 1] > anchoring_b[2 * i_b + 1])
			return false;

		/* The parameter in (A) ends before that in (B) */ 
		if (anchoring_a[2 * i_a + 1] < anchoring_b[2 * i_b + 1])
			dominate= true;
	}

	/* The are unmatched parameters left in (B) */ 
	if (i_b < n_b)
		dominate= true;

	return dominate;
}

#endif /* ! TARGET_HH */
