#ifndef PRESET_HH
#define PRESET_HH

/* 
 * A pre-set is a container data structure with the following properties:
 *   - It contains a multimap of strings to type T. 
 *   - It allows to check, given a string x, which of the key strings in the map
 *     is the longest prefix of x.   
 */

#include <vector>
#include <tuple>
#include <algorithm>

template <class T>
class Preset
{
public:

	class End_Iterator{};
	
	class Input_Iterator 
	{
		friend class Preset;

	public:
		void operator++() {
			/* We need only prefix ++ */
			advance(); 
			if (preset && begin1 < end1)
				++begin1;
			/* Do nothing when the iterator is at the end */
		}

		const T &operator*() {
			advance();
			assert(preset && begin1 < end1); 
			return *begin1; 
		}

		bool operator==(const End_Iterator &) {
			advance();
			return preset == nullptr; 
		}

		bool operator!=(const End_Iterator &) {
			advance();
			return preset != nullptr; 
		}

	private:
		/* When PRESET is null, this iterator has ended and the other
		 * fields have unspecified values.
		 * If PRESET is non-null, then:
		 * - PRESET is the preset in which to continue.
		 * - BEGIN1..END1 and BEGIN2..END2 are the next elements to
		 *   return.  (Either may be an empty range or both null.)
		 */
		const Preset <T> *preset;
		const T *begin1, *end1;
		const T *begin2, *end2;

		Input_Iterator(const Preset <T> *p,
			       const T *b1, const T *e1)
			:  preset(p),
			   begin1(b1), end1(e1)
		{
			if (p->chars.size() && p->chars[0].prefix[0] == '\0') {
				assert(p->chars[0].prefix == "");
				assert(p->chars[0].preset == nullptr); 
				begin2= p->chars[0].values.data();
				end2  = p->chars[0].values.data() + p->chars[0].values.size();
			} else {
				begin2= nullptr;
				end2  = nullptr; 
			}
		}

		/* Make sure the next element to return is at BEGIN1, or the
		 * iterator has ended */
		void advance();
	};

	Preset()
		:  parent(nullptr)
	{ }
	
	Preset(const Preset <T> *_parent, char _index)
		:  parent(_parent), index(_index)
	{ }
	
	void insert(string key, T value);
	/* Insert the key-value pair */
	
	Input_Iterator find(string x);
	/* Find values matching the given string X, i.e., find all values for
	 * which the key is a prefix of X.  Returned from longest to shortest
	 * match.  X must not be the empty string.  */

	End_Iterator end() const { return End_Iterator(); }

private:

	struct Entry {
		string prefix; 
		unique_ptr <Preset <T> > preset;
		vector <T> values;
	};

	const Preset <T> *parent;
	/* The parent object, or null if this is the root object */

	char index;
	/* In PARENT.  Unspecified when PARENT is null. */ 
	
	vector <Entry> chars;
	/* For each tuple, exactly one of {VALUES, PRESET} is nonempty/nonnull. 
	 * If PRESET is null, this object contains at least one value.  Otherwise,
	 * the object contains a child.  
	 * Sorted by the first character of each PREFIX, which are unique.
	 * Also, there can be a single prefix that is the empty string, which
	 * is sorted as the character '\0', i.e., always in first position.
	 * (string[0] returns '\0' when string is "".)
	 */

	void insert(string key, unique_ptr <Preset <T> > preset);

	void insert(string key, vector <T> &&values);
	/* There must not be an entry there yet starting with KEY[0].  KEY may be empty. */
	
	void develop(Entry &);
	/* Transform a VALUES-containing entry to a PRESET-containing entry */
};

template <typename T>
void Preset <T> ::insert(string key, T value)
{
	assert(! key.empty() || parent != nullptr);
	const char k= key[0];
	auto lb= lower_bound(chars.begin(),
			     chars.end(),
			     string(1, k),
			     [](const Entry &a, const string &b) -> bool {
				     return a.prefix[0] < b[0];
			     });
	const size_t i= lb - chars.begin(); 
	assert(i >= 0 && i <= chars.size()); 
	if (i < chars.size() && chars[i].prefix[0] == k) {
		/* Add to existing character */

		Entry &e= chars[i]; 

		/* Find longest common prefix of both */
		auto mm= mismatch(e.prefix.begin(), e.prefix.end(),
				  key.begin(), key.end());
		auto mm_prefix= mm.first;
		auto mm_key=    mm.second; 

		if (*mm_prefix == '\0' && *mm_key != '\0') {
			/* KEY starts with E.PREFIX:  The element can be
			 * inserted within the sub-preset.  */
			if (! e.preset) {
				develop(e);
			}
			e.preset->insert(key.substr(e.prefix.length()), value);
		} else if (*mm_prefix != '\0') {
			/* KEY does not start with E.PREFIX:  The prefix must
			 * be made shorter.  */

			/* We already know that the prefix starts with the
			 * correct character */
			assert(mm_prefix >= e.prefix.begin());

			if (! e.preset) {
				develop(e); 
			}
			unique_ptr <Preset <T> > p= make_unique <Preset <T> >
				(this, e.prefix[0]);
			p->insert(&*mm_prefix, move(e.preset));
			p->insert("", value);
			e.prefix= string(e.prefix.begin(), mm_prefix);
			e.preset= move(p);
		} else {
			assert(*mm_key == '\0' && *mm_prefix == '\0'); 
			/* KEY == E.PREFIX:  The element can be inserted
			 * directly as a value, or inserted into the
			 * sub-preset.  */
			if (! e.preset) {
				e.values.push_back(value);
			} else {
				e.preset->insert("", value); 
			}
		}
	} else {
		/* Insert a new character */
		chars.insert(chars.begin() + i, Entry{key, nullptr, {value}}); 
	}
}

template <typename T>
typename Preset <T> ::Input_Iterator Preset <T> ::find(string x)
{
	assert(x[0] != '\0'); 
	const char k= x[0];
	auto lb= lower_bound(chars.begin(),
			     chars.end(),
			     string(1, k),
			     [](const Entry &a, const string &b) -> bool {
				     return a.prefix[0] < b[0];
			     });
	const size_t i= lb - chars.begin(); 
	assert(i >= 0 && i <= chars.size()); 

	if (i < chars.size() && chars[i].prefix[0] == k) {
		Entry &entry= chars[i];
		assert(entry.prefix[0] == k); 
		if (equal(entry.prefix.begin(), entry.prefix.end(), x.begin())) {
			if (entry.preset) {
				return entry.preset->find(x.substr(entry.prefix.length()));
			} else {
				return Input_Iterator(this,
						      entry.values.data(),
						      entry.values.data() + entry.values.size()); 
			}
		} else {
			return Input_Iterator(this, nullptr, nullptr); 
		}
	} else {
		if (k != '\0' && i == chars.size()
		    && chars.size() != 0 && chars[0].prefix[0] == '\0') {
			/* There is a ""-prefixed entry that we didn't find, but
			 * we must return */
			assert(chars[0].values.size() != 0);
			assert(chars[0].preset == nullptr); 
			return Input_Iterator(this,
					      chars[0].values.data(),
					      chars[0].values.data() + chars[0].values.size()); 
		} else 
			return Input_Iterator(this, nullptr, nullptr);
	}
}

template <typename T>
void Preset <T> ::insert(string key, unique_ptr <Preset <T> > preset)
{
	/* This implementation follows that of the insert() function for individual values */  

	assert(! key.empty());
	const char k= key[0];
	auto lb= lower_bound(chars.begin(),
			     chars.end(),
			     string(1, k),
			     [](const Entry &a, const string &b) -> bool {
				     return a.prefix[0] < b[0];
			     });
	const size_t i= lb - chars.begin(); 
	assert(i >= 0 && i <= chars.size()); 
	if (i < chars.size() && chars[i].prefix[0] == k) {
		/* Add to existing character */

		Entry &e= chars[i]; 

		/* Find longest common prefix of both */
		auto mm= mismatch(e.prefix.begin(), e.prefix.end(),
				  key.begin(), key.end());
		auto mm_prefix= mm.first;
		auto mm_key=    mm.second; 

		if (*mm_prefix == '\0' && *mm_key != '\0') {
			/* KEY starts with E.PREFIX */
			if (! e.preset) {
				develop(e);
			}
			e.preset->insert(key.substr(e.prefix.length()), move(preset));
		} else if (*mm_prefix != '\0') {
			if (! e.preset) {
				develop(e); 
			}
			e.preset= move(preset);
			preset->parent= this;
			preset->index= k;
		} else /* *mm_key == '\0' && *mm_prefix == '\0' */ {
			assert(*mm_key == '\0' && *mm_prefix == '\0'); 
			/* KEY == E.PREFIX */
			if (! e.preset) {
				preset->insert("", move(e.values));
				e.preset= move(preset);
				assert(e.values.empty()); 
				preset->parent= this;
				preset->index= k;
			} else {
				assert(false); 
			}
		}
	} else {
		/* Insert a new character */
		preset->parent= this;
		preset->index= k;
		chars.insert(chars.begin() + i, Entry{key, move(preset), {}});
	}
}

template <typename T>
void Preset <T> ::insert(string key, vector <T> &&values)
{
	const char k= key[0];
	auto lb= lower_bound(chars.begin(),
			     chars.end(),
			     string(1, k),
			     [](const Entry &a, const string &b) -> bool {
				     return a.prefix[0] < b[0];
			     });
	const size_t i= lb - chars.begin(); 

	/* There is no entry yet starting with the same character or \0 */
	assert(i == chars.size() || chars[i].prefix[0] != key[0]);

	Entry e{key, nullptr, move(values)};
	chars.insert(chars.begin() + i, move(e)); 
}

template <typename T>
void Preset <T> ::develop(Entry &entry)
{
	assert(entry.preset == nullptr);

	entry.preset= make_unique <Preset <T> > (this, entry.prefix[0]);
	entry.preset->chars.resize(1); 
	entry.preset->chars[0].prefix= "";
	entry.preset->chars[0].values= move(entry.values); 
}

template <typename T>
void Preset <T> ::Input_Iterator::advance()
{
	while (preset && begin1 == end1) {

		if (begin2 != end2) {
			begin1= begin2;
			end1  = end2  ;
			begin2= nullptr;
			end2  = nullptr;
			return;
		}
		
		const Preset <T> *preset_new= preset->parent;

		if (! preset_new) {
			preset= nullptr;
			return;
		}

		auto lb= lower_bound(preset_new->chars.begin(),
				     preset_new->chars.end(),
				     string(1, preset->index),
				     [](const Entry &a, const string &b) -> bool {
					     return a.prefix[0] < b[0];
				     });
		const size_t i= lb - preset_new->chars.begin(); 
		assert(i >= 0 && i < preset_new->chars.size());
		assert(preset_new->chars[i].preset.get() == preset);

		preset= preset_new;

		begin1= preset_new->chars[i].values.data(); 
		end1  = preset_new->chars[i].values.data()
			+ preset_new->chars[i].values.size(); 

		if (preset_new->chars[0].prefix[0] == '\0') {
			assert(i > 0); 
			begin2= preset_new->chars[0].values.data();
			end2  = preset_new->chars[0].values.data()
				+ preset_new->chars[0].values.size();
		} else {
			begin2= nullptr;
			end2  = nullptr;
		}
	}
}

#endif /* ! PRESET_HH */
