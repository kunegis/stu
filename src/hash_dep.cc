#include "hash_dep.hh"

void Hash_Dep::render(Parts &parts, Rendering rendering) const
{
	TRACE_FUNCTION(SHOW, Target::show);
	size_t i;
	for (i= 0; get_word(i) & F_TARGET_DYNAMIC; ++i) {
		assert((get_word(i) & F_TARGET_TRANSIENT) == 0);
		if (rendering & R_SHOW_FLAGS) {
			render_flags
				(get_word(i) & ~(F_TARGET_DYNAMIC | F_TARGET_TRANSIENT),
				 parts, rendering);
		}
		parts.append_operator("[");
	}
	assert(text.size() > sizeof(word_t) * (i + 1));
	if (rendering & R_SHOW_FLAGS) {
		render_flags(get_word(i) & ~(F_TARGET_TRANSIENT | F_VARIABLE), parts, rendering);
	}
	if (get_word(i) & F_TARGET_TRANSIENT) {
		parts.append_operator("@");
	}
	parts.append_text(text.substr(sizeof(word_t) * (i + 1)));
	for (i= 0; get_word(i) & F_TARGET_DYNAMIC; ++i) {
		parts.append_operator("]");
	}
}

void Hash_Dep::canonicalize()
{
	char *b= (char *)text.c_str(), *p= b;
	while ((*(word_t *)p) & F_TARGET_DYNAMIC)
		p += sizeof(word_t);
	p += sizeof(word_t);
	p= canonicalize_string(A_BEGIN | A_END, p);
	text.resize(p - b);
}

string Hash_Dep::string_from_word(Flags flags)
{
	assert(flags <= 1 << C_WORD);
	char ret[sizeof(word_t) + 1];
	ret[sizeof(word_t)]= '\0';
	*(word_t *)ret= (word_t)flags;
	return string(ret, sizeof(word_t));
}

void render(const Hash_Dep &hash_dep, Parts &parts, Rendering rendering)
{
	hash_dep.render(parts, rendering);
}
