#ifndef HASH_BASED_DEP_HH
#define HASH_BASED_DEP_HH

/*
 * Representation of a simple dependency, used as the key for caching of Executor objects.
 * The difference to the Dependency class is that Hash_Dep objects don't store the Place
 * objects, and don't support parametrization.  Thus, Hash_Dep objects are used as keys in
 * maps, etc.  Flags are included.  Does not support dependencies of non-cached executors.
 *
 * TEXT is a linear representation of the target.  It contains, from left to right:
 *   - [word_size_t] the starting address of the base directory.  As an index in the
 *     string.  Zero if there is no base dir.
 *   - [K * word_t] Dynamic words; the number K is equal to the dynamic multiplicity of
 *     the dependency, and may be zero.  F_DYNAMIC is set in each word.  They also contain
 *     other flags from F_WORD. 
 *   - [word_t] A plain word; F_DYNAMIC is not set.  Also contains other flags from
 *     F_WORD, including F_PHONY. 
 *   - The name of the contained object.  Always non-empty and '\0'-terminated.  The
 *     terminating '\0' is part of the std::string if there is a base dir, and the
 *     implicit terminating '\0' if not.
 *   - (optional) A base dir.  A string of length at least one.  Terminated by the
 *     std::string's implicit terminating '\0'.
 *
 * A non-based non-dynamic dependency is represented as a zero size_t, then a type word_t
 * (F_TARGET_PHONY or 0), and then the name.
 *
 * A non-based dynamic is represented as a zero size_t, a dynamic word (F_TARGET_DYNAMIC),
 * and the string representation of the contained dependency.
 *
 * Any of the word_t elements may contain additional flag bits, but only those from F_WORD.
 * There may be '\0' bytes in the size_t/word_t values, but the base dir and object name do not
 * contain '\0', as that is invalid in names.  The base dir and name proper (excluding
 * front words) are non-empty, i.e., are at least one byte long.
 *
 * The empty std::string denotes a "null" value for the type Hash_Dep, or equivalently the
 * target of the root dependency, in which case most functions should not be used.
 */

#include "hash_dep_utils.hh"

class Hash_Based_Dep
// TODO rename Based_Hash_Dep
{
public:
	explicit
	Hash_Based_Dep(std::string_view text_): text(text_) {
		check(); 
	}
	/* TEXT_ is the full text field of this Hash_Dep */

	Hash_Based_Dep(Flags flags, string name)
	/* A plain target; no base dir */
		// TODO use direct std::string constructor with correct length, then
		// assign content directly
		: text(string_from_size(0) + string_from_word(flags) + name)
	{
		assert((flags & ~F_PHONY) == 0);
		assert(name.find('\0') == string::npos); /* Names do not contain \0 */
		assert(! name.empty());
		check(); // RM
	}

	Hash_Based_Dep(Flags flags, const Hash_Based_Dep &target);
	/* Makes the given target once more dynamic with the given flags, which must *not*
	 * contain the 'dynamic' flag. */

	Hash_Based_Dep(string base_dir, Hash_Based_Dep hash_based_dep);
	Hash_Based_Dep(Hash_Bare_Dep d);
	
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

	// TODO move to .cc
	bool is_any_file() const {
		check(); //
		size_t i= 0;
		while (get_word(i) & F_DYNAMIC) {
			++i;
		}
		return (get_word(i) & F_PHONY) == 0;
	}

	// TODO move to .cc
	bool is_any_phony() const {
		check(); //
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
		check(); //
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
//		return *(word_t *)&text[0];
	}

	Flags get_front_word_nondynamic() const {
		check();
		assert((get_word(0) & F_DYNAMIC) == 0);
		return *(const word_t *)(text.data() + sizeof(word_size_t));
//		return *(const word_t *)&text[0];
	}

	Flags get_word(size_t i) const
	/* For access to any front word */
	{
		check(); //
		assert(text.size() > sizeof(word_size_t) + sizeof(word_t) * (i + 1));
		return ((const word_t *)&text[sizeof(word_size_t)])[i];
	}

	const char *get_base_dir() const
	{
		check(); //
		return (*(const word_size_t *)text.data()) != 0
			? text.data() + (*(const word_size_t *)text.data())
			: nullptr;
	}
	
	bool operator==(const Hash_Based_Dep &d) const { return text == d.text; }
	bool operator!=(const Hash_Based_Dep &d) const { return text != d.text; }
	void canonicalize_plain(); /* In-place, knowing it is plain */

	// TODO these two functions should not be needed if we always have constructors
	// that properly use std::string(size_t, '\0').
//	static string string_from_word(Flags flags);
//	/* Return a string of length sizeof(word_t) containing the given flags */

#ifndef NDEBUG
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

void render(const Hash_Based_Dep &hash_based_dep, Parts &parts, Rendering rendering= 0);

#ifndef NDEBUG
string show_trace(const Hash_Based_Dep &hash_based_dep);
#endif /* ! NDEBUG */

namespace std {
	template <> struct hash <Hash_Based_Dep>
	{
		size_t operator()(const Hash_Based_Dep &hash_based_dep) const;
	};
}

#endif /* ! HASH_BASED_DEP_HH */
