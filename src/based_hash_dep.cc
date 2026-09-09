#include "based_hash_dep.hh"

#include "show_flags.hh"

Based_Hash_Dep::Based_Hash_Dep(string base_dir, Based_Hash_Dep hash_dep)
	: text(hash_dep.text.size() + 1 + base_dir.size(), 0)
{
	assert(! hash_dep.get_base());
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

Based_Hash_Dep::Based_Hash_Dep(Hash_Bare_Dep d)
	: text(string_from_size(0) + d.get_text())
{ }

size_t std::hash <Based_Hash_Dep> ::operator()(const Based_Hash_Dep &based_hash_dep) const
{
	return std::hash <string> ()(based_hash_dep.get_text());
}

#ifndef NDEBUG

void Based_Hash_Dep::render(Parts &parts, Rendering rendering) const
{
	check();
	const char *base_dir= get_base();
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

void Based_Hash_Dep::canonicalize()
// TODO code is nearly identicaly to NDEBUG function; merge
{
	TRACE_FUNCTION();
	check();
	const char *base_dir= get_base();
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

size_t Based_Hash_Dep::get_dynamic_depth() const
{
	size_t ret;
	for (ret= 0; get_word(ret) & F_DYNAMIC; ++ret);
	return ret;
}

void Based_Hash_Dep::check() const
/* The minimum length of TEXT is sizeof(word_t)+1: One word indicating a non-dynamic
 * target, and a text of length one.  (The text cannot be empty.) */
{
	TRACE_FUNCTION();
	assert(text.size() > sizeof(word_size_t) + sizeof(word_t));
}

void render(const Based_Hash_Dep &hash_dep, Parts &parts, Rendering rendering)
{
	hash_dep.render(parts, rendering);
}

string show_trace(const Based_Hash_Dep &hash_dep)
{
	Parts parts;
	render(hash_dep, parts, R_SHOW_FLAGS);
	return show(parts, S_TRACE);
}

#endif /* ! NDEBUG */
