#ifndef HASH_BARE_DEP_HH
#define HASH_BARE_DEP_HH

#include "flags.hh"
#include "hash_dep_utils.hh"

// TODO check which functions to move to .cc

class Hash_Bare_Dep
// TODO rename Plain -> something else ('plain' already has another meaning)
{
public:
	explicit
	Hash_Bare_Dep(std::string_view text_): text(text_) { }
	/* TEXT_ is the full text field of this Hash_Dep */

	Hash_Bare_Dep(Flags flags, string name)
	/* A plain target */
		: text(string_from_word(flags) + name)
	{
		assert((flags & ~F_PHONY) == 0);
		assert(name.find('\0') == string::npos); /* Names do not contain \0 */
		assert(! name.empty());
	}

	Hash_Bare_Dep(Flags flags, const Hash_Bare_Dep &target)
	// TODO rename arg target -> d
	/* Makes the given target once more dynamic with the given
	 * flags, which must *not* contain the 'dynamic' flag. */
		: text(string_from_word(flags | F_DYNAMIC) + target.text)
	{
		assert((flags & (F_DYNAMIC | F_PHONY)) == 0);
		assert(flags < (1 << C_WORD));
	}

	const string &get_text() const { return text; }
	string &get_text() { return text; }
	const char *get_text_c_str() const { return text.c_str(); }
	bool is_dynamic() const { check(); return get_word(0) & F_DYNAMIC; }

	bool is_file() const {
		check();
		return (get_word(0) & (F_DYNAMIC | F_PHONY)) == 0;
	}

	bool is_phony() const {
		check();
		return (get_word(0) & (F_DYNAMIC | F_PHONY)) == F_PHONY;
	}

	bool is_any_file() const {
		size_t i= 0;
		while (get_word(i) & F_DYNAMIC) {
			++i;
		}
		return (get_word(i) & F_PHONY) == 0;
	}

	bool is_any_phony() const {
		size_t i= 0;
		while (get_word(i) & F_DYNAMIC) {
			++i;
		}
		return get_word(i) & F_PHONY;
	}

	void render(Parts &, Rendering= 0) const;

	string get_name_nondynamic() const
	/* Get the name of the target, knowing that the target is not dynamic */
	{
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return text.substr(sizeof(word_t));
	}

	const char *get_name_c_str_nondynamic() const
	/* Return a C pointer to the name of the file or phony.  The object must be
	 * non-dynamic. */
	{
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return text.c_str() + sizeof(word_t);
	}

	const char *get_name_c_str_any() const
	{
		const char *ret= text.c_str();
		while ((*(const word_t *)ret) & F_DYNAMIC)
			ret += sizeof(word_t);
		return ret += sizeof(word_t);
	}

	Flags get_front_word() const { return get_word(0); }
	word_t &get_front_word_any() { return *(word_t *)&text[0]; }

	word_t &get_front_word_nondynamic()
	/* Get the front byte, given that the target is not dynamic */
	{
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return *(word_t *)&text[0];
	}

	Flags get_front_word_nondynamic() const {
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return *(const word_t *)&text[0];
	}

	Flags get_word(size_t i) const
	/* For access to any front word */
	{
		assert(text.size() > sizeof(word_t) * (i + 1));
		return ((const word_t *)&text[0])[i];
	}

	bool operator==(const Hash_Bare_Dep &target) const { return text == target.text; }
	bool operator!=(const Hash_Bare_Dep &target) const { return text != target.text; }
	void canonicalize_plain(); /* In-place, knowing it is plain */

#ifndef NEBUG
	void canonicalize(); /* In-place */
	size_t get_dynamic_depth() const;
#endif /* ! NDEBUG */

private:
	string text;

	void check() const {
		/* The minimum length of TEXT is sizeof(word_t)+1:  One word indicating a
		 * non-dynamic target, and a text of length one.  (The text cannot be
		 * empty.) */
#ifndef NDEBUG
		assert(text.size() > sizeof(word_t));
#endif /* ! NDEBUG */
	}
};

void render(const Hash_Bare_Dep &hash_bare_dep, Parts &parts, Rendering rendering= 0);

#ifndef NDEBUG
string show_trace(const Hash_Bare_Dep &hash_bare_dep);
#endif /* ! NDEBUG */

namespace std {
	template <> struct hash <Hash_Bare_Dep>
	{
		size_t operator()(const Hash_Bare_Dep &hash_bare_dep) const;
	};
}

#endif /* ! HASH_BARE_DEP_HH */
