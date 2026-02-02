#include "hash_dep.hh"

void Hash_Dep::render(Parts &parts, Rendering rendering) const
{
	size_t i;
	for (i= 0; get_word(i) & F_TARGET_DYNAMIC; ++i) {
		assert((get_word(i) & F_TARGET_PHONY) == 0);
		parts.append_marker("[");
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
#ifndef NDEBUG
	if (rendering & R_SHOW_FLAGS) {
		render_flags(get_word(i) & ~(F_TARGET_PHONY | F_VARIABLE),
			parts, rendering);
	}
#endif /* ! NDEBUG */
	if (get_word(i) & F_TARGET_PHONY) {
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

#ifndef NDEBUG
string show_trace(const Hash_Dep &hash_dep)
{
	Parts parts;
	render(hash_dep, parts, R_SHOW_FLAGS);
	return show(parts, S_DEBUG);
}
#endif /* ! NDEBUG */

size_t std::hash <Hash_Dep> ::operator()(const Hash_Dep &hash_dep) const
{
	return std::hash <string> ()(hash_dep.get_text());
}
