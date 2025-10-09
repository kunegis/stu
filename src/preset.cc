#include "preset.hh"

#include <algorithm>

template <typename T>
void Preset <T> ::insert(string key, T value)
{
	assert(! key.empty() || parent != nullptr);
	const char k= key[0];
	auto lb= lower_bound(
		chars.begin(), chars.end(), string(1, k),
		[](const Entry &a, const string &b) -> bool { return a.prefix[0] < b[0]; });
	const size_t i= lb - chars.begin();
	assert(i <= chars.size());
	if (i < chars.size() && chars[i].prefix[0] == k) {
		/* Add to existing character */

		Entry &e= chars[i];

		/* Find longest common prefix of both */
		auto mm= mismatch(e.prefix.begin(), e.prefix.end(),
				  key.begin(), key.end());
		auto mm_prefix= mm.first;
		auto mm_key=    mm.second;

		if (*mm_prefix == '\0' && *mm_key != '\0') {
			/* KEY starts with E.PREFIX:  The element can be inserted within
			 * the sub-preset. */
			if (! e.preset) {
				develop(e);
			}
			e.preset->insert(key.substr(e.prefix.length()), value);
		} else if (*mm_prefix != '\0') {
			/* KEY does not start with E.PREFIX:  The prefix must be made
			 * shorter. */

			/* We already know that the prefix starts with the
			 * correct character */
			assert(mm_prefix >= e.prefix.begin());

			if (! e.preset) {
				develop(e);
			}
			std::unique_ptr <Preset <T> > p= /* uncovered_due_to_bug_in_gcov */
				std::make_unique <Preset <T> > (this, e.prefix[0]);

			string key_new= &*mm_prefix;
			assert(! key_new.empty());
			std::unique_ptr <Preset <T> > preset= move(e.preset);
			preset->parent= p.get();
			preset->index= key_new[0];
			p->chars.push_back(Entry{key_new, move(preset), {}});
			p->insert("", value);
			e.prefix= string(e.prefix.begin(), mm_prefix);
			e.preset= move(p);
		} else {
			assert(*mm_key == '\0' && *mm_prefix == '\0');
			/* KEY == E.PREFIX:  The element can be inserted directly as a
			 * value, or inserted into the sub-preset. */
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
	auto lb= lower_bound(
		chars.begin(), chars.end(), string(1, k),
		[](const Entry &a, const string &b) -> bool { return a.prefix[0] < b[0]; });
	const size_t i= lb - chars.begin();
	assert(i <= chars.size());

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
			return Input_Iterator(
				this,
				chars[0].values.data(),
				chars[0].values.data() + chars[0].values.size());
		} else
			return Input_Iterator(this, nullptr, nullptr);
	}
}

template <typename T>
void Preset <T> ::develop(Entry &entry)
{
	assert(entry.preset == nullptr);

	entry.preset= std::make_unique <Preset <T> > (this, entry.prefix[0]);
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

		auto lb= lower_bound(
			preset_new->chars.begin(), preset_new->chars.end(),
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
