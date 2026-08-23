#include "hash_dep.hh"

Hash_Dep::Hash_Dep(string base_dir, Hash_Dep hash_dep)
	: text(hash_dep.text.size() + 1 + base_dir.size(), 0)
{
	assert(! hash_dep.get_base_dir());
	assert(! base_dir.empty());
	*((word_size_t *)text.data())= hash_dep.text.size() + 1;
	memcpy(
		text.data() + sizeof(word_size_t),
		hash_dep.text.data() + sizeof(word_size_t),
		hash_dep.text.size() - sizeof(word_size_t));
	text[hash_dep.text.size()]= '\0';
	memcpy(
		text.data() + hash_dep.text.size() + 1,
		base_dir.data(),
		base_dir.size());
	check(); 
}

void Hash_Dep::render(Parts &parts, Rendering rendering) const
{
	size_t i;
	for (i= 0; get_word(i) & F_DYNAMIC; ++i) {
		assert((get_word(i) & F_PHONY) == 0);
		parts.append_marker("[");
	}
	assert(text.size() > sizeof(word_size_t) + sizeof(word_t) * (i + 1));
#ifndef NDEBUG
	if (rendering & R_SHOW_FLAGS) {
		::render(Flags_View(get_word(i) & ~(F_PHONY | F_VARIABLE)),
			parts, rendering);
	}
#endif /* ! NDEBUG */
	if (get_word(i) & F_PHONY) {
		parts.append_marker("@");
	}
	parts.append_text(text.substr(sizeof(word_t) * (i + 1)));
	for (i= 0; get_word(i) & F_DYNAMIC; ++i) {
		parts.append_marker("]");
	}
	const char *base_dir= get_base_dir();
	if (base_dir) {
		parts.append_marker("(");
		parts.append_text(base_dir);
		parts.append_marker(")");
	}
}

void Hash_Dep::canonicalize_plain()
{
	TRACE_FUNCTION();
	const char *base_dir= get_base_dir();
	char *b= (char *)text.c_str() + sizeof(word_size_t), *p= b;
	assert(! ((*(word_t *)p) & F_DYNAMIC));
	p += sizeof(word_t);
	p= canonicalize_string(A_BEGIN | A_END, p);
	if (base_dir) {
		size_t base_dir_len= text.size() - *(word_size_t *)text.data();
		memmove(
			p + 1,
			text.data() + *(word_size_t *)text.data(),
			base_dir_len);
		*(word_size_t *)text.data()= p - text.data() + 1;
		text.resize(p - text.data() + 1 + base_dir_len);
	} else {
		text.resize(p - text.data());
	}
	check();
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

string Hash_Dep::string_from_size(size_t size)
{
	string ret(sizeof(word_size_t), 0);
	*(word_size_t *)ret.data()= size;
	return ret;
}

void render(const Hash_Dep &hash_dep, Parts &parts, Rendering rendering)
{
	hash_dep.render(parts, rendering);
}

size_t std::hash <Hash_Dep> ::operator()(const Hash_Dep &hash_dep) const
{
	return std::hash <string> ()(hash_dep.get_text());
}

#ifndef NDEBUG

string show_trace(const Hash_Dep &hash_dep)
{
	Parts parts;
	render(hash_dep, parts, R_SHOW_FLAGS);
	return show(parts, S_DEBUG);
}

void Hash_Dep::canonicalize()
{
	const char *base_dir= get_base_dir();
	char *b= (char *)text.c_str(), *p= b;
	while ((*(word_t *)p) & F_DYNAMIC)
		p += sizeof(word_t);
	p += sizeof(word_t);
	p= canonicalize_string(A_BEGIN | A_END, p);

	// TODO code is identicaly to NDEBUG function; merge
	if (base_dir) {
		size_t base_dir_len= text.size() - *(word_size_t *)text.data();
		memmove(
			p + 1,
			text.data() + *(word_size_t *)text.data(),
			base_dir_len);
		*(word_size_t *)text.data()= p - text.data() + 1;
		text.resize(p - text.data() + 1 + base_dir_len);
	} else {
		text.resize(p - b);
	}
}

size_t Hash_Dep::get_dynamic_depth() const
{
	size_t ret;
	for (ret= 0; get_word(ret) & F_DYNAMIC; ++ret);
	return ret;
}

#endif /* ! NDEBUG */
