#ifndef TARGET_HH
#define TARGET_HH

#include "frmt.hh"

/* The type of targets in Stu.  Each target is of exactly one type.
 */

/* Functions named text() returned a optionally quoted and printable
 * representation of the target.  These are inserted directly into error
 * messages (without adding quotes).  text_mid() is used when brackets
 * of any form are added around.  text_bare() returns the always unquoted
 * string.  
 */

string format_name_mid(string name)
{
	// TODO escape these properly in the output
	if (name.find_first_of("\"\' \t\n\v\f\r") != string::npos)
		return fmt("'%s'", name);
	else
		return name;
}

string format_name(string name) 
{
	return fmt("'%s'", name); 
}

/* Declared as int so arithmetic can be performed on it */ 
typedef int Type; 
enum {

	/* Top-level target, which contains the individual targets given
	 * on the command line as dependencies.  Does not appear as
	 * dependencies in Stu files. */ 
	T_ROOT,

	/* A phony target */ 
	T_PHONY,
	
	/* A file in the file system; this entry has to come before
	 * T_DYNAMIC because it counts also as a dynamic dependency of
	 * depth zero. */
	T_FILE,

	/* A dynamic target -- only used for the Target object of executions */   
	T_DYNAMIC

	/* Larger values denote multiply dynamic targets.  They are only
	 * used as the target of Execution objects.  Therefore, T_DYNAMIC is
	 * always last in this enum. */
	
	/* Note:  all dynamic targets are files, and therefore T_FILE can be
	 * thought of as a dynamic target of depth zero. */
};

/* The dynamic depth of the target type.  The number of dynamic
 * indirections for dynamic targets, zero for all other targets. */ 
unsigned dynamic_depth(Type type) {
	assert(type >= T_ROOT); 
	if (type < T_DYNAMIC)  
		return 0;
	else
		return type - T_FILE; 
}

/* 
 * The basic object in Stu:  a file, a variable or a phony.  This
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

		assert(type >= T_ROOT);

		if (type == T_ROOT) {
			return "ROOT"; 
		} else if (type == T_PHONY) {
			return fmt("@%s", format_name_mid(name));  
		} else {
			return type >= T_DYNAMIC
				? (string(type - T_FILE, '[') 
				   + format_name_mid(name) 
				   + string(type - T_FILE, ']'))
				: format_name(name); 
		}
	}

	string format_bare() const {
		assert(type >= T_PHONY);

		if (type == T_PHONY) {
			return fmt("@%s", name);  
		} else {
			return type >= T_DYNAMIC
				? (string(type - T_FILE, '[') 
				   + name
				   + string(type - T_FILE, ']'))
				: name; 
		}
	}

	string format_mid() const {
		assert(type >= T_PHONY);

		if (type == T_PHONY) {
			return fmt("@%s", format_name_mid(name));  
		} else {
			return type >= T_DYNAMIC
				? (string(type - T_FILE, '[') 
				   + format_name_mid(name) 
				   + string(type - T_FILE, ']'))
				: format_name_mid(name); 
		}
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
				(hash<string>()(target.name))
				^ (hash<int>()(target.type) << 1);
		}
	};

}

/* A parametrized name.  Each name has N >= 0  
 * parameters.  When N > 0, the name is parametrized, otherwise it is
 * unparametrized.  
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
 * A parametrized name is empty if N = 0 and the single text is empty
 * (which makes it invalid).
 */
class Param_Name
{
private:

	/* Length = N + 1.
	 * Only the first and last elements can be empty.  
	 */ 
	vector <string> texts; 

	/* Length = N.  
	 * Must all be distinct. 
	 */ 
	vector <string> parameters;

public:

	/* A name with zero parameters */ 
	Param_Name(string name_)
		:  texts({name_})
	{ }

	/* Empty */ 
	Param_Name()
		:  texts({""})
	{ }

	/* A name is empty when N=0, and when texts[0] is the empty
	 * string.  Such names cannot result from parsing an input file,
	 * but they are allowed in this class, and used internally. 
	 */ 
	bool empty() const {
		assert(texts.size() == 1 + parameters.size()); 
		return parameters.empty() && texts[0] == "";
	}

	const vector <string> &get_texts() const {
		return texts; 
	}

	const vector <string> &get_parameters() const {
		return parameters; 
	}

	/* Append the given PARAMETER and an empty text */ 
	void append_parameter(string parameter) {
		parameters.push_back(parameter); 
		texts.push_back("");
	}

	string &last_text() {
		return texts[texts.size() - 1];
	}

	/* The name may be empty, resulting in an empty string */ 
	string instantiate(const map <string, string> &mapping) const;

	/* Return the unparametrized name.  The name must be unparametrized. 
	 */
	string unparametrized() const {
		assert(get_n() == 0);
		return texts.at(0); 
	}

	/* Number of parameters; zero when the name is unparametrized. */ 
	unsigned get_n() const {
		assert(texts.size() == 1 + parameters.size()); 

		return parameters.size(); 
	}

	/* Check whether NAME matches this name.  If it does, return
	 * TRUE and write the mapping into MAPPING. 
	 *
	 * Rules for matching:  each parameter must match at last one
	 * character. 
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

	/* Return whether there are duplicate parameter names.
	 * If TRUE, write the first found into PARAMETER. 
	 */
	bool has_duplicate_parameters(string &parameter) const;

	/* Whether this is a valid name */
	bool valid() const;

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
 
	/* The root target */
	Param_Target(Type type_)
		:  type(type_),
		   param_name("")
		{
			assert(type_ == T_ROOT);
			/* Note:  PARAM_NAME is invalid */ 
		}

	Param_Target(Type type_,
				 shared_ptr <Param_Name> param_name_)
		:  type(type_),
		   param_name(*param_name_)
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
		return shared_ptr <Place_Param_Name> (new Place_Param_Name(name));
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


	Place_Param_Target()
		:  type(T_ROOT),
		   place_param_name()
	{ }

	Place_Param_Target(Type type_,
			   const Place_Param_Name &place_param_name_)
		:  type(type_),
		   place_param_name(place_param_name_)
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
};

string Param_Name::instantiate(const map <string, string> &mapping) const
{
	assert(texts.size() == 1 + parameters.size()); 

	const int n= get_n(); 

	string ret= texts.at(0);

	for (int i= 0;  i < n;  ++i) {
		assert(parameters.at(i).size() > 0); 
		ret += mapping.at(parameters.at(i));
		ret += texts.at(i + 1);
	}

	return ret; 
}

bool Param_Name::match(const string name, 
		       map <string, string> &mapping,
		       vector <int> &anchoring)
{
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

			ret[parameters.at(i)]= string(p, p_end - p - size_last); 
			anchoring[2*i + 1]= p_end - size_last - p_begin;

			assert(ret.at(parameters.at(i)).size() > 0); 

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

	for (auto i= parameters.begin();  i != parameters.end();  ++i) {
		if (seen.count(*i)) {
			parameter= *i; 
			return true;
		}
		seen.insert(*i); 
	}
	
	return false;
}

bool Param_Name::valid() const 
{
	if (empty())  
		return false;

	for (unsigned i= 1;  i + 1 < get_n() + 1;  ++i) {
		if (texts.at(i) == "")
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
