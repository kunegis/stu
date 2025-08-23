#ifndef HASH_DEP_HH
#define HASH_DEP_HH

/*
 * A representation of a simple dependency, mainly used as the key in the caching of
 * Executor objects.  The difference to the Dependency class is that Hash_Dep objects
 * don't store the Place objects, and don't support parametrization.  Thus, Hash_Dep
 * objects are used as keys in maps, etc.  Flags are included.
 *
 * TEXT is a linear representation of the target.  It begins with a certain number of
 * words (word_t, at least one), followed by the name of the target as a string.  A word_t
 * is represented by a fixed number of characters.
 *
 * A non-dynamic dependency is represented as a Type word (F_TARGET_TRANSIENT or 0)
 * followed by the name.
 *
 * A dynamic is represented as a dynamic word (F_TARGET_DYNAMIC) followed by the string
 * representation of the contained dependency.
 *
 * Any of the front words may contain additional flag bits.  There may be '\0' bytes in
 * the front words, but the name does not contain nul, as that is invalid in names.  The
 * name proper (excluding front words) is non-empty, i.e., is at least one byte long.
 *
 * The empty string denotes a "null" value for the type Hash_Dep, or equivalently the
 * target of the root dependency, in which case most functions should not be used.
 */

#include <stdint.h>

typedef uint16_t word_t;

static_assert(sizeof(word_t) == sizeof(uint8_t) && C_WORD <= 8
	|| sizeof(word_t) == sizeof(uint16_t) && C_WORD > 8 && C_WORD <= 16);

class Hash_Dep
{
public:
	explicit Hash_Dep(std::string_view text_)
		/* TEXT_ is the full text field of this Hash_Dep */
		: text(text_) { }

	Hash_Dep(Flags flags, string name)
	/* A plain target */
		: text(string_from_word(flags) + name)
	{
		assert((flags & ~F_TARGET_TRANSIENT) == 0);
		assert(name.find('\0') == string::npos); /* Names do not contain \0 */
		assert(! name.empty());
	}

	Hash_Dep(Flags flags, const Hash_Dep &target)
	/* Makes the given target once more dynamic with the given
	 * flags, which must *not* contain the 'dynamic' flag. */
		: text(string_from_word(flags | F_TARGET_DYNAMIC) + target.text)
	{
		assert((flags & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT)) == 0);
		assert(flags < (1 << C_WORD));
	}

	const string &get_text() const { return text; }
	string &get_text() { return text; }
	const char *get_text_c_str() const { return text.c_str(); }
	bool is_dynamic() const { check(); return get_word(0) & F_TARGET_DYNAMIC; }

	bool is_file() const {
		check();
		return (get_word(0) & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT)) == 0;
	}

	bool is_transient() const {
		check();
		return (get_word(0) & (F_TARGET_DYNAMIC | F_TARGET_TRANSIENT))
			== F_TARGET_TRANSIENT;
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

	void render(Parts &, Rendering= 0) const;

	string get_name_nondynamic() const
	/* Get the name of the target, knowing that the target is not dynamic */
	{
		check();
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0);
		return text.substr(sizeof(word_t));
	}

	const char *get_name_c_str_nondynamic() const
	/* Return a C pointer to the name of the file or transient.  The object must be
	 * non-dynamic. */
	{
		check();
		assert((get_word(0) & F_TARGET_DYNAMIC) == 0);
		return text.c_str() + sizeof(word_t);
	}

	const char *get_name_c_str_any() const
	{
		const char *ret= text.c_str();
		while ((*(const word_t *)ret) & F_TARGET_DYNAMIC)
			ret += sizeof(word_t);
		return ret += sizeof(word_t);
	}

	Flags get_front_word() const { return get_word(0); }

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
		return *(const word_t *)&text[0];
	}

	Flags get_word(size_t i) const
	/* For access to any front word */
	{
		assert(text.size() > sizeof(word_t) * (i + 1));
		return ((const word_t *)&text[0])[i];
	}

	bool operator==(const Hash_Dep &target) const { return text == target.text; }
	bool operator!=(const Hash_Dep &target) const { return text != target.text; }
	void canonicalize_plain(); /* In-place, knowing it is plain */

	static string string_from_word(Flags flags);
	/* Return a string of length sizeof(word_t) containing the given flags */

#ifndef NEBUG
	void canonicalize(); /* In-place */
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

void render(const Hash_Dep &hash_dep, Parts &parts, Rendering rendering= 0);

namespace std {
	template <> struct hash <Hash_Dep>
	{
		size_t operator()(const Hash_Dep &hash_dep) const {
			return hash <string> ()(hash_dep.get_text());
		}
	};
}

#endif /* ! HASH_DEP_HH */
