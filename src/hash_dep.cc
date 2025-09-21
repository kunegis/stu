#include "hash_dep.hh"

void Hash_Dep::render(Parts &parts, Rendering rendering) const
{
	TRACE_FUNCTION();
	TRACE("show_flags= %s", frmt("%d", (rendering & R_SHOW_FLAGS) != 0));
	TRACE("depth= %s", frmt("%zu", get_dynamic_depth()));
	size_t i;
	for (i= 0; get_word(i) & F_TARGET_DYNAMIC; ++i) {
		TRACE("One dynamic");
		assert((get_word(i) & F_TARGET_TRANSIENT) == 0);
		if (rendering & R_SHOW_FLAGS) {
			/* Hash_Dep::render() is only called with the -d option from
			 * File_Executor and Transient_Executor, none of which uses
			 * dynamic Hash_Dep's. */
			should_not_happen();
			render_flags(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT),
				parts, rendering);
		}
		parts.append_marker("[");
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
	if (rendering & R_SHOW_FLAGS) {
		render_flags(get_word(i) & ~(F_TARGET_TRANSIENT | F_VARIABLE),
			parts, rendering);
	}
	if (get_word(i) & F_TARGET_TRANSIENT) {
		parts.append_marker("@");
	}
	parts.append_text(text.substr(sizeof(word_t) * (i + 1)));
	for (i= 0; get_word(i) & F_TARGET_DYNAMIC; ++i) {
		parts.append_marker("]");
	}
}

#ifndef NDEBUG

void Hash_Dep::canonicalize()
{
	char *b= (char *)text.c_str(), *p= b;
	while ((*(word_t *)p) & F_TARGET_DYNAMIC)
		p += sizeof(word_t);
	p += sizeof(word_t);
	p= canonicalize_string(A_BEGIN | A_END, p);
	text.resize(p - b);
}

size_t Hash_Dep::get_dynamic_depth() const
{
	size_t ret;
	for (ret= 0; get_word(ret) & F_TARGET_DYNAMIC; ++ret);
	return ret;
}

#endif /* ! NDEBUG */

void Hash_Dep::canonicalize_plain()
{
	char *b= (char *)text.c_str(), *p= b;
	assert(! ((*(word_t *)p) & F_TARGET_DYNAMIC));
	p += sizeof(word_t);
	p= canonicalize_string(A_BEGIN | A_END, p);
	text.resize(p - b);
}

string Hash_Dep::string_from_word(Flags flags)
{
	assert(flags < 1 << C_WORD);
	char ret[sizeof(word_t) + 1];
	ret[sizeof(word_t)]= '\0';
	word_t w= (word_t)flags;
	memcpy(ret, &w, sizeof(word_t)); /* Assigning it directly is undefined behavior */
	return string(ret, sizeof(word_t));
}

void render(const Hash_Dep &hash_dep, Parts &parts, Rendering rendering)
{
	hash_dep.render(parts, rendering);
}
