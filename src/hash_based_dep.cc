#include "hash_based_dep.hh"

Hash_Based_Dep::Hash_Based_Dep(Flags flags, const Hash_Based_Dep &target)
{
	assert((flags & (F_DYNAMIC | F_PHONY)) == 0);
	assert(flags < (1 << C_WORD));

	const char *base_dir= target.get_base_dir();
	if (base_dir) {
		text= string(sizeof(word_t) + target.text.size(), '\0');
		*(word_size_t *)text.data()=
			sizeof(word_t) + *(const word_size_t *)target.text.data();
		get_front_word_any()= flags | F_DYNAMIC;
		memcpy(
			text.data() + sizeof(word_size_t) + sizeof(word_t),
			target.text.data() + sizeof(word_size_t),
			target.text.size() - sizeof(word_size_t));
	} else {
		text= string(sizeof(word_t) + target.text.size(), '\0');
		get_front_word_any()= flags | F_DYNAMIC;
		memcpy(
			text.data() + sizeof(word_size_t) + sizeof(word_t),
			target.text.data() + sizeof(word_size_t),
			target.text.size() - sizeof(word_size_t));
	}

	check();
}

Hash_Based_Dep::Hash_Based_Dep(string base_dir, Hash_Based_Dep hash_dep)
	: text(hash_dep.text.size() + 1 + base_dir.size(), 0)
{
	assert(! hash_dep.get_base_dir());
	if (base_dir.empty()) {
		text= hash_dep.text;
		return;
	}
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

Hash_Based_Dep::Hash_Based_Dep(Hash_Bare_Dep d)
	: text(string_from_size(0) + d.get_text())
{ }

void Hash_Based_Dep::render(Parts &parts, Rendering rendering) const
{
	check();
	const char *base_dir= get_base_dir();
	size_t i;
	for (i= 0; get_word(i) & F_DYNAMIC; ++i) {
		assert((get_word(i) & F_PHONY) == 0);
		parts.append_marker("[");
	}
	assert(text.size() > sizeof(word_size_t) + sizeof(word_t) * (i + 1));

#ifndef NDEBUG
	if (rendering & R_SHOW_FLAGS)
		::render(Flags_View(get_word(i) & ~(F_PHONY | F_VARIABLE)),
			parts, rendering);
#endif /* ! NDEBUG */

	if (get_word(i) & F_PHONY) {
		parts.append_marker("@");
	}
	size_t start= sizeof(word_size_t) + sizeof(word_t) * (i + 1);
	parts.append_text(text.substr(
		start,
		base_dir
		? base_dir - text.data() - start - 1
		: text.size() - start));
	for (i= 0; get_word(i) & F_DYNAMIC; ++i) {
		parts.append_marker("]");
	}

	// TODO probably not needed
	if (base_dir) {
		parts.append_marker("(");
		parts.append_text(base_dir);
		parts.append_marker(")");
	}
}

void Hash_Based_Dep::canonicalize_plain()
{
	TRACE_FUNCTION();
	check();
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

bool Hash_Based_Dep::is_any_file() const
{
	size_t i= 0;
	while (get_word(i) & F_DYNAMIC) {
		++i;
	}
	return (get_word(i) & F_PHONY) == 0;
}

//bool Hash_Based_Dep::is_any_phony() const
//{
//	size_t i= 0;
//	while (get_word(i) & F_DYNAMIC) {
//		++i;
//	}
//	return get_word(i) & F_PHONY;
//}

void render(const Hash_Based_Dep &hash_dep, Parts &parts, Rendering rendering)
{
	hash_dep.render(parts, rendering);
}

size_t std::hash <Hash_Based_Dep> ::operator()(const Hash_Based_Dep &hash_based_dep) const
{
	return std::hash <string> ()(hash_based_dep.get_text());
}

#ifndef NDEBUG

string show_trace(const Hash_Based_Dep &hash_dep)
{
	Parts parts;
	render(hash_dep, parts, R_SHOW_FLAGS);
	return show(parts, S_DEBUG);
}

void Hash_Based_Dep::canonicalize()
// TODO code is nearly identicaly to NDEBUG function; merge
{
	TRACE_FUNCTION();
	check();
	const char *base_dir= get_base_dir();
	TRACE("base_dir= %s", base_dir ? base_dir : "<NULL>");
	char *b= (char *)text.c_str() + sizeof(word_size_t), *p= b;
	while ((*(word_t *)p) & F_DYNAMIC)
		p += sizeof(word_t);
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

size_t Hash_Based_Dep::get_dynamic_depth() const
{
	size_t ret;
	for (ret= 0; get_word(ret) & F_DYNAMIC; ++ret);
	return ret;
}

void Hash_Based_Dep::check() const
/* The minimum length of TEXT is sizeof(word_t)+1: One word indicating a non-dynamic
 * target, and a text of length one.  (The text cannot be empty.) */
{
	TRACE_FUNCTION();
	assert(text.size() > sizeof(word_size_t) + sizeof(word_t));
}

#endif /* ! NDEBUG */
