#ifndef BASED_HASH_DEP_HH
#define BASED_HASH_DEP_HH

/*
 * Representation of a simple dependency, used as the key for caching of Executor objects.
 * The difference to the Dependency class is that Hash_Dep objects don't store the Place
 * objects, and don't support parametrization.  Thus, Hash_Dep objects are used as keys in
 * maps, etc.  Flags are included.  Does not support dependencies of non-cached executors.
 *
 * TEXT is a linear representation of the target.  It contains, from left to right:
 *   - [word_size_t] the starting address of the base directory.  As an index in the
 *     string.  Zero if there is no base.
 *   - [K * word_t] Dynamic words; the number K is equal to the dynamic multiplicity of
 *     the dependency, and may be zero.  F_DYNAMIC is set in each word.  They also contain
 *     other flags from F_WORD.
 *   - [word_t] A plain word; F_DYNAMIC is not set.  Also contains other flags from
 *     F_WORD, including F_PHONY.
 *   - The name of the contained object.  Always non-empty and '\0'-terminated.  The
 *     terminating '\0' is part of the std::string if there is a base, and the
 *     implicit terminating '\0' if not.
 *   - (optional) A base directory.  A string of length at least one.  Terminated by the
 *     std::string's implicit terminating '\0'.
 *
 * A non-based non-dynamic dependency is represented as a zero size_t, then a type word_t
 * (F_TARGET_PHONY or 0), and then the name.
 *
 * A non-based dynamic is represented as a zero size_t, a dynamic word (F_TARGET_DYNAMIC),
 * and the string representation of the contained dependency.
 *
 * Any of the word_t elements may contain additional flag bits, but only those from F_WORD.
 * There may be '\0' bytes in the size_t/word_t values, but the base and object name do not
 * contain '\0', as that is invalid in names.  The base and name proper (excluding
 * front words) are non-empty, i.e., are at least one byte long.
 *
 * The empty std::string denotes a "null" value for the type Hash_Dep, or equivalently the
 * target of the root dependency, in which case most functions should not be used.
 */

#include "hash_dep_utils.hh"

class Based_Hash_Dep
{
public:
	explicit
	Based_Hash_Dep(std::string_view text_): text(text_) {
		check();
	}
	/* TEXT_ is the full text field of this Hash_Dep */

	Based_Hash_Dep(Flags flags, string name)
	/* A plain target; no base directory */
	// TODO use direct std::string constructor with correct length, then
	// assign content directly
		: text(string_from_size(0) + string_from_word(flags) + name)
	{
		assert((flags & ~F_PHONY) == 0);
		assert(name.find('\0') == string::npos); /* Names do not contain \0 */
		assert(! name.empty());
	}

	Based_Hash_Dep(string base, Based_Hash_Dep hash_based_dep);
	Based_Hash_Dep(Hash_Dep d);

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

	string get_name_nondynamic() const
	/* Get the name of the target, knowing that the target is not dynamic */
	{
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return text.substr(sizeof(word_size_t) + sizeof(word_t));
	}

	const char *get_name_c_str_nondynamic() const
	/* Return a C pointer to the name of the file or phony.  The object must be
	 * non-dynamic. */
	{
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return text.c_str() + sizeof(word_size_t) + sizeof(word_t);
	}

	// TODO move to .cc
	const char *get_name_c_str_any() const
	{
		const char *ret= text.c_str() + sizeof(word_size_t);
		while ((*(const word_t *)ret) & F_DYNAMIC)
			ret += sizeof(word_t);
		return ret += sizeof(word_t);
	}

	Flags get_front_word() const { return get_word(0); }
	word_t &get_front_word_any() { return *(word_t *)&text[sizeof(word_size_t)]; }

	word_t &get_front_word_nondynamic()
	/* Get the front byte, given that the target is not dynamic */
	{
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return *(word_t *)(text.data() + sizeof(word_size_t));
	}

	Flags get_front_word_nondynamic() const {
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return *(const word_t *)(text.data() + sizeof(word_size_t));
	}

	Flags get_word(size_t i) const
	/* For access to any front word */
	{
		assert(text.size() > sizeof(word_size_t) + sizeof(word_t) * (i + 1));
		return ((const word_t *)&text[sizeof(word_size_t)])[i];
	}

	const char *get_base() const
	{
		return (*(const word_size_t *)text.data()) != 0
			? text.data() + (*(const word_size_t *)text.data())
			: nullptr;
	}

	bool operator==(const Based_Hash_Dep &d) const { return text == d.text; }
	bool operator!=(const Based_Hash_Dep &d) const { return text != d.text; }

	// TODO these two functions should not be needed if we always have constructors
	// that properly use std::string(size_t, '\0').

#ifndef NDEBUG
	void render(Parts &, Rendering= 0) const;
	void canonicalize(); /* In-place */
	size_t get_dynamic_depth() const;
#endif /* ! NDEBUG */

private:
	string text;

	// TODO put into .cc when !NDEBUG

#ifdef NDEBUG
	void check() const {}
#else /* ! NDEBUG */
	void check() const;
#endif /* ! NDEBUG */
};

#ifndef NDEBUG
void render(const Based_Hash_Dep &hash_based_dep, Parts &parts, Rendering rendering= 0);
string show_trace(const Based_Hash_Dep &hash_based_dep);
#endif /* ! NDEBUG */

namespace std {
	template <> struct hash <Based_Hash_Dep>
	{
		size_t operator()(const Based_Hash_Dep &based_hash_dep) const;
	};
}

#endif /* ! BASED_HASH_DEP_HH */
