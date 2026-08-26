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
		? base_dir - text.data() - start
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

void Hash_Dep::canonicalize_plain()
{
	TRACE_FUNCTION();
	check();
	TRACE("text(1)= %s", show(text)); //
	const char *base_dir= get_base_dir();
	TRACE("base_dir= %s", base_dir ? base_dir : "<NULL>"); //
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
	TRACE("text(2)= %s", show(text)); //
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

size_t Hash_Dep::get_dynamic_depth() const
{
	size_t ret;
	for (ret= 0; get_word(ret) & F_DYNAMIC; ++ret);
	return ret;
}


void Hash_Dep::check() const
/* The minimum length of TEXT is sizeof(word_t)+1: One word indicating a non-dynamic
 * target, and a text of length one.  (The text cannot be empty.) */
{
	assert(text.size() > sizeof(word_size_t) + sizeof(word_t));
	word_size_t b= *(const word_size_t *)text.data();
	TRACE("b= %s", frmt("%zu", b)); //
	TRACE("text.size()= %s", frmt("%zu", text.size())); //
	TRACE("(b < text.size())= %s", frmt("%d", b < text.size())); //
	TRACE("text.size()(2)= %s", frmt("%zu", text.size())); //
	TRACE("b(2)= %s", frmt("%zu", b)); //
	TRACE("static_cast <bool> (b < text.size())= %s", frmt("%d", static_cast <bool> (b < text.size()))); //

	//
//	int *pp= nullptr;
//	*pp= 1;

	//
	bool yyy= true;
	assert(yyy);

//	bool xxx= b < text.size(); //
//	TRACE("xxx= %s", frmt("%d", xxx)); //
//	assert(xxx); //
//	assert((b < text.size())); // XXX fix
}
		
#endif /* ! NDEBUG */
