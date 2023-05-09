#include "dep.hh"

#include <bitset>

#include "color.hh"
#include "text.hh"

shared_ptr <Dep> Dep::clone(shared_ptr <const Dep> dep)
{
	assert(dep);

	if (to <Plain_Dep> (dep)) {
		return make_shared <Plain_Dep> (* to <Plain_Dep> (dep));
	} else if (to <Dynamic_Dep> (dep)) {
		return make_shared <Dynamic_Dep> (* to <Dynamic_Dep> (dep));
	} else if (to <Compound_Dep> (dep)) {
		return make_shared <Compound_Dep> (* to <Compound_Dep> (dep));
	} else if (to <Concat_Dep> (dep)) {
		return make_shared <Concat_Dep> (* to <Concat_Dep> (dep));
	} else if (to <Root_Dep> (dep)) {
		return make_shared <Root_Dep> (* to <Root_Dep> (dep));
	} else {
		assert(false);
		return nullptr;
	}
}

void Dep::add_flags(shared_ptr <const Dep> dep,
		    bool overwrite_places)
{
	for (unsigned i= 0;  i < C_PLACED;  ++i) {
		if (dep->flags & (1 << i)) {
			if (overwrite_places || ! (this->flags & (1 << i))) {
				this->set_place_flag(i, dep->get_place_flag(i));
			}
		}
	}
	this->flags |= dep->flags;
}

shared_ptr <const Dep> Dep::strip_dynamic(shared_ptr <const Dep> d)
{
	assert(d != nullptr);
	while (to <Dynamic_Dep> (d)) {
		d= to <Dynamic_Dep> (d)->dep;
	}
	assert(d != nullptr);
	return d;
}

#ifndef NDEBUG
void Dep::check() const
{
	assert(top.get() != this);

	for (unsigned i= 0;  i < C_PLACED;  ++i) {
		assert(((flags & (1 << i)) == 0) == get_place_flag(i).empty());
	}

	if (auto plain_this= dynamic_cast <const Plain_Dep *> (this)) {
		/* The F_TARGET_TRANSIENT flag is always set in the
		 * dependency flags, even though that is redundant.  */
		assert((plain_this->flags & F_TARGET_TRANSIENT)
		       == (plain_this->place_param_target.flags));

		if (! plain_this->variable_name.empty()) {
			assert((plain_this->place_param_target.flags & F_TARGET_TRANSIENT) == 0);
			assert(plain_this->flags & F_VARIABLE);
		}
	}

	if (auto dynamic_this= dynamic_cast <const Dynamic_Dep *> (this)) {
		assert(flags & F_TARGET_DYNAMIC);
		dynamic_this->dep->check();
	} else {
		assert(!(flags & F_TARGET_DYNAMIC));
	}

	if (auto concat_this= dynamic_cast <const Concat_Dep *> (this)) {
		assert(concat_this->deps.size() >= 2);
		for (auto i:  concat_this->deps) {
			assert(i);
		}
	}

	assert(index >= -1);
}
#endif

Target Plain_Dep::get_target() const
{
	Target ret= place_param_target.unparametrized();
	ret.get_front_word_nondynamic() |= (word_t)(flags & F_TARGET_BYTE);
	return ret;
}

string Plain_Dep::format(Style style, Quotes *q) const
{
	string f;
	if (!(style & S_NO_FLAGS)) {
		f= flags_format(flags & ~(F_VARIABLE | F_TARGET_TRANSIENT));
		if (! f.empty()) {
//			style |= S_MARKERS;
			f += ' ';
		}
	}
//	bool detached= (flags & F_VARIABLE) || (flags & F_TARGET_TRANSIENT);
//	style &= ~S_QUOTES;
//	style |= S_QUOTES * !detached;
//	style |= S_WANT_ESCAPE;
//	if (!detached)
//		quotes= true;
	string t= place_param_target.format(style | S_INNER, q);
	string ret= fmt("%s%s%s%s",
			f,
			flags & F_VARIABLE ? "$[" : "",
			t,
			flags & F_VARIABLE ? "]" : "");
//	if (style & S_COLOR_WORD) {
//		ret= fmt("%s%s%s", Color::word, ret, Color::end);
//	}
	return quote(ret, style, q);
}

//string Plain_Dep::format_out() const
//{
//	Style style= S_NO_FLAGS | S_OUT | S_WANT_ESCAPE;
//	string ret= format(style);
//	if (style & S_QUOTES)
//		ret= fmt("\'%s\'", ret);
//	return ret;
//}

//string Plain_Dep::format_src() const
//{
//	Style style= S_SRC;
//	return format(style);
//}

//string Plain_Dep::format_err() const
//{
//	Style style= S_NO_FLAGS | S_ERR | S_COLOR_WORD | S_WANT_ESCAPE | (Color::quotes * S_QUOTES); 
//	string ret= format(style);
//	if (style & S_QUOTES)
//		ret= fmt("\'%s\'", ret);
//	return ret;
//}

Target Dynamic_Dep::get_target() const
{
	string text;
	const Dep *d= this;
	while (dynamic_cast <const Dynamic_Dep *> (d)) {
		Flags f= F_TARGET_DYNAMIC;
		assert(d->flags & F_TARGET_DYNAMIC);
		f |= d->flags & F_TARGET_BYTE;
		text += Target::string_from_word(f);
		d= dynamic_cast <const Dynamic_Dep *> (d)->dep.get();
	}
	assert(dynamic_cast <const Plain_Dep *> (d));
	const Plain_Dep *sin= dynamic_cast <const Plain_Dep *> (d);
	assert(!(sin->flags & F_TARGET_DYNAMIC));
	Flags f= sin->flags & F_TARGET_BYTE;
	text += Target::string_from_word(f);
	text += sin->place_param_target.unparametrized().get_name_nondynamic();

	return Target(text);
}

string Dynamic_Dep::format(Style style, Quotes *q) const
{
//	quotes= false;
	Style style_inner= style;
//	bool quotes_inner= false;
	string ret;
	if (! (style & S_NO_FLAGS)) {
		string text_flags= flags_format(flags & ~F_TARGET_DYNAMIC);
		if (! text_flags.empty())
			text_flags += ' ';
		ret += text_flags;
	}
	string text= dep->format(style_inner, q);
	ret += fmt("[%s]", text);
//	if (style & S_COLOR_WORD) 
//		ret= fmt("%s%s%s", Color::word, ret, Color::end);
	return ret;
}

// string Dynamic_Dep::format_out() const
// {
// 	Style style= S_NO_FLAGS | S_OUT;
// 	return format(style);
// }

// string Dynamic_Dep::format_src() const
// {
// 	Style style= S_SRC;
// 	return format(style);
// }

// string Dynamic_Dep::format_err() const
// {
// 	Style style= S_NO_FLAGS | S_ERR | S_COLOR_WORD;
// 	return format(style);
// }

shared_ptr <const Dep> Dynamic_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Dynamic_Dep> ret= make_shared <Dynamic_Dep>
		(flags, places, dep->instantiate(mapping));
	ret->index= index;
	ret->top= top;
	return ret;
}

shared_ptr <const Dep> Plain_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Place_Param_Target> ret_target= place_param_target.instantiate(mapping);

	shared_ptr <Dep> ret= make_shared <Plain_Dep>
		(flags, places, *ret_target, place, variable_name);
	ret->index= index;
	ret->top= top;

	assert(ret_target->place_name.get_n() == 0);

	string this_name= ret_target->place_name.unparametrized();
	if ((flags & F_VARIABLE) && this_name.find('=') != string::npos) {
		assert((ret_target->flags & F_TARGET_TRANSIENT) == 0);
		place << fmt("dynamic variable %s must not be instantiated with parameter value that contains %s",
			     dynamic_variable_format(this_name),
			     char_format('='));
		throw ERROR_LOGICAL;
	}

	return ret;
}

shared_ptr <const Dep>
Compound_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Compound_Dep> ret= make_shared <Compound_Dep> (flags, places, place);
	ret->index= index;
	ret->top= top;

	for (const shared_ptr <const Dep> &d:  deps) {
		ret->push_back(d->instantiate(mapping));
	}

	return ret;
}

bool Compound_Dep::is_unparametrized() const
/* A compound dependency is parametrized when any of its contained
 * dependency is parametrized.  */
{
	for (shared_ptr <const Dep> d:  deps) {
		if (! d->is_unparametrized())
			return false;
	}
	return true;
}

string Compound_Dep::format(Style style, Quotes *q) const
/* Ignore QUOTES, as everything inside the parentheses will not need
 * it.  */
{
	string ret;
	for (const shared_ptr <const Dep> &d:  deps) {
		if (! ret.empty())
			ret += " ";
		ret += d->format(style, q);
	}
	if (deps.size() != 1)
		ret= fmt("(%s)", ret);
	return ret;
}

// string Compound_Dep::format_err() const
// {
// 	Style style= S_ERR | S_NO_FLAGS | S_COLOR_WORD | S_WANT_ESCAPE | S_QUOTES * (Color::quotes);
// 	string ret= format(style);
// 	if (style & S_QUOTES)
// 		ret= '\'' + ret + '\'';
// 	return ret;
// }

// string Compound_Dep::format_out() const
// {
// 	Style style= S_OUT | S_NO_FLAGS | S_WANT_ESCAPE | S_QUOTES * Color::quotes;
// //	bool quotes= Color::quotes;
// 	string ret= format(style);
// 	if (style & S_QUOTES)
// 		ret= '\'' + ret + '\'';
// 	return ret;
// }

// string Compound_Dep::format_src() const
// {
// 	Style style= S_SRC | S_WANT_ESCAPE | S_QUOTES * Color::quotes;
// //	bool quotes= Color::quotes;
// 	string ret= format(style);
// 	if (style & S_QUOTES)
// 		ret= '\'' + ret + '\'';
// 	return ret;
// }

shared_ptr <const Dep> Concat_Dep::instantiate(const map <string, string> &mapping) const
{
	shared_ptr <Concat_Dep> ret= make_shared <Concat_Dep> (flags, places);
	ret->index= index;
	ret->top= top;

	for (const shared_ptr <const Dep> &d:  deps) {
		ret->push_back(d->instantiate(mapping));
	}

	return ret;
}

bool Concat_Dep::is_unparametrized() const
/* A concatenated dependency is parametrized when any of its contained
 * dependency is parametrized.  */
{
	for (shared_ptr <const Dep> d:  deps) {
		if (! d->is_unparametrized())
			return false;
	}
	return true;
}

const Place &Concat_Dep::get_place() const
/* Return the place of the first dependency, or an empty place */
{
	if (deps.empty())
		return Place::place_empty;

	return deps.front()->get_place();
}

string Concat_Dep::format(Style style, Quotes *q) const
/* We only need quotes when *all* components need quotes */
{
	assert(bitset <sizeof(Style)> (style & S_CHANNEL).count() <= 1);
	string ret;
	if (!(style & S_NO_FLAGS)) {
		string f= flags_format(flags);
		if (! f.empty()) {
//			style |= S_MARKERS;
			f += ' ';
		}
		ret += f;
	}
//	Style style_ret= style | S_QUOTES;
//	style |= S_QUOTES;
//	bool quotes_ret= true;
	for (const shared_ptr <const Dep> &d:  deps) {
//		Style style_d= style;
//		bool quotes_d= quotes;
		ret += d->format(style, q);
		//		if (! (q.is()
		//		       style_d & S_QUOTES
		//		       ))
		//			style_ret &= ~S_QUOTES;
//			quotes_ret= false;
	}
//	quotes= quotes_ret;
//	style= style_ret;
	return ret;
}

// string Concat_Dep::format_err() const
// {
// 	Style style= S_ERR | S_NO_FLAGS | S_COLOR_WORD | S_WANT_ESCAPE | S_QUOTES * Color::quotes;
// //	bool quotes= Color::quotes;
// 	string ret= format(style);
// 	if (style & S_QUOTES)
// 		ret= '\'' + ret + '\'';
// 	return ret;
// }

// string Concat_Dep::format_out() const
// {
// 	Style style= S_OUT | S_NO_FLAGS | S_WANT_ESCAPE | S_QUOTES * Color::quotes;
// //	bool quotes= Color::quotes;
// 	string ret= format(style);
// 	if (style & S_QUOTES)
// 		ret= '\'' + ret + '\'';
// 	return ret;
// }

// string Concat_Dep::format_src() const
// {
// 	Style style= S_SRC | S_WANT_ESCAPE | S_QUOTES * Color::quotes;
// //	bool quotes= Color::quotes;
// 	string ret= format(style);
// 	if (style & S_QUOTES)
// 		ret= '\'' + ret + '\'';
// 	return ret;
// }

bool Concat_Dep::is_normalized() const
{
	for (auto &i:  deps) {
		if (to <const Concat_Dep> (i))
			return false;
		if (! i->is_normalized())
			return false;
	}
	return true;
}

void Concat_Dep::normalize_concat(shared_ptr <const Concat_Dep> dep,
				  vector <shared_ptr <const Dep> > &deps_,
				  int &error)
{
	size_t k_init= deps_.size();
	normalize_concat(dep, deps_, 0, error);
	if (error && ! option_k)
		return;

	/* Add attributes from DEP */
	if (dep->flags || dep->index >= 0 || dep->top) {
		for (size_t k= k_init;  k < deps_.size();  ++k) {
			shared_ptr <Dep> d_new= Dep::clone(deps_[k]);
			/* The innermost flag is kept */
			d_new->add_flags(dep, false);
			if (dep->index >= 0)
				d_new->index= dep->index;
			d_new->top= dep->top;
			deps_[k]= d_new;
		}
	}
}

void Concat_Dep::normalize_concat(shared_ptr <const Concat_Dep> dep,
				  vector <shared_ptr <const Dep> > &deps_,
				  size_t start_index,
				  int &error)
{
	assert(start_index < dep->deps.size());

	if (start_index + 1 == dep->deps.size()) {
		shared_ptr <const Dep> dd= dep->deps.at(start_index);
		if (auto compound_dd= to <Compound_Dep> (dd)) {
			for (const auto &d:  compound_dd->deps) {
				normalize(d, deps_, error);
				if (error && ! option_k)
					return;
			}
		} else if (to <Plain_Dep> (dd)) {
			deps_.push_back(dd);
		} else if (auto concat_dd= to <Concat_Dep> (dd)) {
			normalize_concat(concat_dd, deps_, error);
			if (error && ! option_k)
				return;
		} else if (to <Dynamic_Dep> (dd)) {
			normalize(dd, deps_, error);
			if (error && ! option_k)
				return;
		} else {
			assert(false);
		}
	} else {
		vector <shared_ptr <const Dep> > vec1, vec2;
		normalize_concat(dep, vec2, start_index + 1, error);
		if (error && ! option_k)
			return;
		shared_ptr <const Dep> dd= dep->deps.at(start_index);
		if (auto compound_dd= to <Compound_Dep> (dd)) {
			for (const auto &d:  compound_dd->deps) {
				normalize(d, vec1, error);
				if (error && ! option_k)
					return;
			}
		} else if (to <Plain_Dep> (dd)) {
			vec1.push_back(dd);
		} else if (to <Dynamic_Dep> (dd)) {
			normalize(dd, vec1, error);
			if (error && ! option_k)
				return;
		} else if (auto concat_dd= to <Concat_Dep> (dd)) {
			normalize_concat(concat_dd, vec1, error);
			if (error && ! option_k)
				return;
		} else {
			assert(false);
		}

		for (const auto &d1:  vec1) {
			for (const auto &d2:  vec2) {
				shared_ptr <const Dep> d= concat(d1, d2, error);
				if (error && ! option_k)
					return;
				if (d)
					deps_.push_back(d);
			}
		}
	}
}

Target Concat_Dep::get_target() const
{
	/* Dep::get_target() is not used for complex dependencies */
	assert(false);
	return Target();
}

shared_ptr <const Dep> Concat_Dep::concat(shared_ptr <const Dep> a,
					  shared_ptr <const Dep> b,
					  int &error)
{
	assert(a);
	assert(b);

	/*
	 * Check for invalid combinations
	 */

	if (a->flags & F_INPUT) {
		/* It would in principle be possible to allow
		 * concatenations in which the left component has an
		 * input redirection, but the current data structures do
		 * not allow that, and therefore we make that invalid.  */
		a->get_place() << fmt("%s cannot have input redirection using %s",
				      a->format(),
				      char_format('<'));
		b->get_place() << fmt("because %s is concatenated to it",
				      b->format());
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (b->flags & F_INPUT) {
		/* We don't save the place for the '<', so we cannot
		 * have "using '<'" on an extra line.  */
		b->get_place() << fmt("%s cannot have input redirection using %s",
				      b->format(),
				      char_format('<'));
		a->get_place() << fmt("in concatenation to %s", a->format());
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (b->flags & F_PLACED) {
		static_assert(C_PLACED == 3, "Expected C_PLACED == 3");
		unsigned i_flag=
			b->flags & F_PERSISTENT ? I_PERSISTENT :
			b->flags & F_OPTIONAL   ? I_OPTIONAL   :
			b->flags & F_TRIVIAL    ? I_TRIVIAL    :
			C_ALL;
		assert(i_flag != C_ALL);
		b->get_place() << fmt("%s cannot be declared as %s",
				      b->format(), flags_phrases[i_flag]);
		b->places[i_flag] << fmt("using %s",
					 name_format(frmt("-%c", flags_chars[i_flag])));
		a->get_place() << fmt("in concatenation to %s", a->format());
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (b->flags & F_TARGET_TRANSIENT) {
		b->get_place() << fmt("transient target %s is invalid", b->format());
		a->get_place() << fmt("in concatenation to %s", a->format());
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (a->flags & F_VARIABLE) {
		a->get_place() << fmt("the variable dependency %s cannot be used",
				      a->format());
		b->get_place() << fmt("in concatenation with %s",
				      b->format());
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (b->flags & F_VARIABLE) {
		b->get_place() << fmt("variable dependency %s is invalid",
				      b->format());
		a->get_place() << fmt("in concatenation to %s", a->format());
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (to <const Plain_Dep> (a) && to <const Plain_Dep> (b))
		return concat_plain(to <const Plain_Dep> (a), to <const Plain_Dep> (b));
	else
		return concat_complex(a, b);
}

shared_ptr <const Plain_Dep> Concat_Dep::concat_plain(shared_ptr <const Plain_Dep> a,
						      shared_ptr <const Plain_Dep> b)
{
	assert(a);
	assert(b);

	/* Parametrized dependencies are instantiated first before they
	 * are concatenated  */
	assert(! a->place_param_target.place_name.is_parametrized());
	assert(! b->place_param_target.place_name.is_parametrized());

	/*
	 * Combine
	 */

	Flags flags_combined= a->flags | b->flags;

	Place_Name place_name_combined(a->place_param_target.place_name.unparametrized() +
				       b->place_param_target.place_name.unparametrized(),
				       a->place_param_target.place_name.place);

	shared_ptr <Plain_Dep> ret=
		make_shared <Plain_Dep> (flags_combined,
					 a->places,
					 Place_Param_Target(flags_combined & F_TARGET_TRANSIENT,
							    place_name_combined,
							    a->place_param_target.place),
					 a->place, "");
	ret->top= a->top;
	if (! ret->top)
		ret->top= b->top;
	return ret;
}

shared_ptr <const Concat_Dep> Concat_Dep::concat_complex(shared_ptr <const Dep> a,
							 shared_ptr <const Dep> b)
/* We don't have to make any checks here because any errors will be
 * caught later when the resulting plain dependencies are concatenated.
 * However, checking errors here is faster, since it avoids building
 * dynamic dependencies unnecessarily.  */
{
	assert(! (to <const Plain_Dep> (a) && to <const Plain_Dep> (b)));

	shared_ptr <Concat_Dep> ret= make_shared <Concat_Dep> ();

	if (auto concat_a= to <const Concat_Dep> (a)) {
		for (auto d:  concat_a->deps)
			ret->push_back(d);
	} else {
		ret->push_back(a);
	}

	if (auto concat_b= to <const Concat_Dep> (b)) {
		for (auto d:  concat_b->deps)
			ret->push_back(d);
	} else {
		ret->push_back(b);
	}

	return ret;
}
