#ifndef PRESET_HH
#define PRESET_HH

/*
 * A pre-set is a container data structure with the following properties:
 *   - It contains a multimap of strings to type T.
 *   - It allows to check, given a string x, which of the key strings in the map
 *     is the longest prefix of x.
 */

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
				end2  = p->chars[0].values.data()
					+ p->chars[0].values.size();
			} else {
				begin2= nullptr;
				end2  = nullptr;
			}
		}

		/* Make sure the next element to return is at BEGIN1, or the
		 * iterator has ended */
		void advance();
	};

	Preset():  parent(nullptr)  {  }

	Preset(const Preset <T> *_parent, char _index)
		:  parent(_parent), index(_index)  {  }

	void insert(string key, T value);
	/* Insert the key-value pair */

	Input_Iterator find(string x);
	/* Find values matching the given string X, i.e., find all values for
	 * which the key is a prefix of X.  Returned from longest to shortest
	 * match.  X must not be the empty string.  */

	End_Iterator end() const  {  return End_Iterator();  }

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
	 * (string[0] returns '\0' when string is "".)  */

	void insert(string key, unique_ptr <Preset <T> > preset);

	void insert(string key, vector <T> &&values);
	/* There must not be an entry there yet starting with KEY[0].  KEY may be empty. */

	void develop(Entry &);
	/* Transform a VALUES-containing entry to a PRESET-containing entry */
};

#endif /* ! PRESET_HH */
