#include "dep.hh"

#include <bitset>

#include "color.hh"
#include "format.hh"
#include "trace.hh"

void Dep::normalize(shared_ptr <const Dep> dep,
		    std::vector <shared_ptr <const Dep> > &deps,
		    int &error)
{
	if (to <Plain_Dep> (dep)) {
		deps.push_back(dep);
	} else if (shared_ptr <const Dynamic_Dep> dynamic_dep= to <Dynamic_Dep> (dep)) {
		std::vector <shared_ptr <const Dep> > deps_child;
		normalize(dynamic_dep->dep, deps_child, error);
		if (error && ! option_k)
			return;
		for (auto &d:  deps_child) {
			shared_ptr <Dep> dep_new=
				std::make_shared <Dynamic_Dep>
				(dynamic_dep->flags, dynamic_dep->places, d);
			if (dynamic_dep->index >= 0)
				dep_new->index= dynamic_dep->index;
			dep_new->top= dynamic_dep->top;
			deps.push_back(dep_new);
		}
	} else if (shared_ptr <const Compound_Dep> compound_dep= to <Compound_Dep> (dep)) {
		for (auto &d:  compound_dep->deps) {
			shared_ptr <Dep> dd= Dep::clone(d);
			dd->add_flags(compound_dep, false);
			if (compound_dep->index >= 0)
				dd->index= compound_dep->index;
			dd->top= compound_dep->top;
			normalize(dd, deps, error);
			if (error && ! option_k)
				return;
		}
	} else if (auto concat_dep= to <Concat_Dep> (dep)) {
		Concat_Dep::normalize_concat(concat_dep, deps, error);
		if (error && ! option_k)
			return;
	} else {
		unreachable();
	}
}

shared_ptr <const Dep> Dep::untrivialize(shared_ptr <const Dep> dep)
{
	assert(dep);
	assert(dep->is_normalized());
	if (to <Plain_Dep> (dep)) {
		if (! (dep->flags & F_TRIVIAL))
			return nullptr;
		shared_ptr <Dep> ret= Dep::clone(dep);
		ret->flags &= ~F_TRIVIAL;
		ret->places[I_TRIVIAL]= Place();
		return ret;
	} else if (to <Dynamic_Dep> (dep)) {
		shared_ptr <const Dynamic_Dep> dynamic_dep= to <Dynamic_Dep> (dep);
		shared_ptr <const Dep> ret_dep= untrivialize(to <Dynamic_Dep> (dep)->dep);
		if (ret_dep) {
			shared_ptr <Dynamic_Dep> ret= std::make_shared <Dynamic_Dep> (dynamic_dep, ret_dep);
			ret->flags &= ~F_TRIVIAL;
			ret->places[I_TRIVIAL]= Place();
			return ret;
		} else if (dep->flags & F_TRIVIAL) {
			shared_ptr <Dep> ret= Dep::clone(dep);
			ret->flags &= ~F_TRIVIAL;
			ret->places[I_TRIVIAL]= Place();
			return ret;
		} else {
			return nullptr;
		}
	} else if (to <Concat_Dep> (dep)) {
		shared_ptr <const Concat_Dep> concat_dep= to <Concat_Dep> (dep);
		shared_ptr <Concat_Dep> ret= std::make_shared <Concat_Dep> (dep);
		ret->deps.resize(concat_dep->deps.size());
		bool found= false;
		for (size_t i= 0; i < concat_dep->deps.size(); ++i) {
			shared_ptr <const Dep> dep_i= untrivialize(concat_dep->deps[i]);
			if (dep_i) {
				found= true;
				ret->deps[i]= dep_i;
			} else {
				ret->deps[i]= concat_dep->deps[i];
			}
		}
		if (! found && !(dep->flags & F_TRIVIAL))
			return nullptr;
		ret->flags &= ~F_TRIVIAL;
		ret->places[I_TRIVIAL]= Place();
		return ret;
	} else {
		unreachable();
	}
}

shared_ptr <Dep> Dep::clone(shared_ptr <const Dep> dep)
{
	assert(dep);
	if (to <Plain_Dep> (dep)) {
		return std::make_shared <Plain_Dep> (* to <Plain_Dep> (dep));
	} else if (to <Dynamic_Dep> (dep)) {
		return std::make_shared <Dynamic_Dep> (* to <Dynamic_Dep> (dep));
	} else if (to <Compound_Dep> (dep)) {
		return std::make_shared <Compound_Dep> (* to <Compound_Dep> (dep));
	} else if (to <Concat_Dep> (dep)) {
		return std::make_shared <Concat_Dep> (* to <Concat_Dep> (dep));
	} else if (to <Root_Dep> (dep)) {
		return std::make_shared <Root_Dep> (* to <Root_Dep> (dep));
	} else {
		unreachable();
	}
}

void Dep::add_flags(shared_ptr <const Dep> dep,
		    bool overwrite_places)
{
	for (unsigned i= 0; i < C_PLACED; ++i) {
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

	for (unsigned i= 0; i < C_PLACED; ++i) {
		assert(((flags & (1 << i)) == 0) == get_place_flag(i).empty());
	}

	if (auto plain_this= dynamic_cast <const Plain_Dep *> (this)) {
		/* The F_TARGET_TRANSIENT flag is always set in the
		 * dependency flags, even though that is redundant.  */
		assert((plain_this->flags & F_TARGET_TRANSIENT)
		       == (plain_this->place_target.flags));

		if (! plain_this->variable_name.empty()) {
			assert((plain_this->place_target.flags & F_TARGET_TRANSIENT) == 0);
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
		for (auto i: concat_this->deps) {
			assert(i);
		}
	}

	assert(index >= -1);
}
#endif

Hash_Dep Plain_Dep::get_target() const
{
	Hash_Dep ret= place_target.unparametrized();
	ret.get_front_word_nondynamic() |= (word_t)(flags & F_TARGET_WORD);
	return ret;
}

void Plain_Dep::render(Parts &parts, Rendering rendering) const
{
	TRACE_FUNCTION(SHOW, Plain_Dep::render);

	if (render_flags(flags, parts, rendering))
		parts.append_space();
	if (flags & F_VARIABLE)
		parts.append_operator("$[");
	if (flags & F_INPUT && rendering & R_SHOW_INPUT)
		parts.append_operator("<");
	place_target.render(parts, rendering);
	if (flags & F_VARIABLE)
		parts.append_operator("]");
}

Hash_Dep Dynamic_Dep::get_target() const
{
	string text;
	const Dep *d= this;
	while (dynamic_cast <const Dynamic_Dep *> (d)) {
		Flags f= F_TARGET_DYNAMIC;
		assert(d->flags & F_TARGET_DYNAMIC);
		f |= d->flags & F_TARGET_WORD;
		text += Hash_Dep::string_from_word(f);
		d= dynamic_cast <const Dynamic_Dep *> (d)->dep.get();
	}
	assert(dynamic_cast <const Plain_Dep *> (d));
	const Plain_Dep *sin= dynamic_cast <const Plain_Dep *> (d);
	assert(!(sin->flags & F_TARGET_DYNAMIC));
	Flags f= sin->flags & F_TARGET_WORD;
	text += Hash_Dep::string_from_word(f);
	text += sin->place_target.unparametrized().get_name_nondynamic();

	return Hash_Dep(text);
}

void Dynamic_Dep::render(Parts &parts, Rendering rendering) const
{
	TRACE_FUNCTION(SHOW, Dynamic_Dep::render);
	if (render_flags(flags & ~F_TARGET_DYNAMIC, parts, rendering))
		parts.append_space();
	parts.append_operator("[");
	dep->render(parts, rendering | R_NO_COMPOUND_PARENTHESES);
	parts.append_operator("]");
}

shared_ptr <const Dep> Dynamic_Dep::instantiate(const std::map <string, string> &mapping) const
{
	shared_ptr <Dynamic_Dep> ret= std::make_shared <Dynamic_Dep>
		(flags, places, dep->instantiate(mapping));
	ret->index= index;
	ret->top= top;
	return ret;
}

shared_ptr <const Dep> Plain_Dep::instantiate(const std::map <string, string> &mapping) const
{
	shared_ptr <Place_Target> ret_target= place_target.instantiate(mapping);

	shared_ptr <Dep> ret= std::make_shared <Plain_Dep>
		(flags, places, *ret_target, place, variable_name);
	ret->index= index;
	ret->top= top;

	assert(ret_target->place_name.get_n() == 0);

	string this_name= ret_target->place_name.unparametrized();
	if ((flags & F_VARIABLE) && this_name.find('=') != string::npos) {
		assert((ret_target->flags & F_TARGET_TRANSIENT) == 0);
		place << fmt("dynamic variable %s must not be instantiated with parameter value that contains %s",
			     show_dynamic_variable(this_name),
			     show_operator('='));
		throw ERROR_LOGICAL;
	}

	return ret;
}

shared_ptr <const Dep>
Compound_Dep::instantiate(const std::map <string, string> &mapping) const
{
	shared_ptr <Compound_Dep> ret= std::make_shared <Compound_Dep> (flags, places, place);
	ret->index= index;
	ret->top= top;
	for (const shared_ptr <const Dep> &d: deps) {
		ret->push_back(d->instantiate(mapping));
	}
	return ret;
}

bool Compound_Dep::is_unparametrized() const
/* A compound dependency is parametrized when any of its contained
 * dependency is parametrized.  */
{
	for (shared_ptr <const Dep> d: deps) {
		if (! d->is_unparametrized())
			return false;
	}
	return true;
}

void Compound_Dep::render(Parts &parts, Rendering rendering) const
{
	TRACE_FUNCTION(SHOW, Compound_Dep::render);
	if (!(rendering & R_NO_COMPOUND_PARENTHESES))
		parts.append_operator("(");
	bool first= true;
	for (const shared_ptr <const Dep> &d: deps) {
		if (first)
			first= false;
		else
			parts.append_space();
		d->render(parts, rendering & ~R_NO_COMPOUND_PARENTHESES);
	}
	if (!(rendering & R_NO_COMPOUND_PARENTHESES))
		parts.append_operator(")");
}

shared_ptr <const Dep> Concat_Dep::instantiate(const std::map <string, string> &mapping) const
{
	shared_ptr <Concat_Dep> ret= std::make_shared <Concat_Dep> (flags, places);
	ret->index= index;
	ret->top= top;

	for (const shared_ptr <const Dep> &d: deps) {
		ret->push_back(d->instantiate(mapping));
	}

	return ret;
}

bool Concat_Dep::is_unparametrized() const
/* A concatenated dependency is parametrized when any of its contained
 * dependency is parametrized.  */
{
	for (shared_ptr <const Dep> d: deps) {
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

void Concat_Dep::render(Parts &parts, Rendering rendering) const
{
	TRACE_FUNCTION(SHOW, Concat_Dep::show);
	if (rendering & R_SHOW_FLAGS) {
		if (render_flags(flags, parts, rendering)) {
			parts.append_space();
		}
	}
	for (const shared_ptr <const Dep> &d: deps) {
		d->render(parts, rendering);
	}
}

bool Concat_Dep::is_normalized() const
{
	for (auto &i: deps) {
		if (to <const Concat_Dep> (i))
			return false;
		if (! i->is_normalized())
			return false;
	}
	return true;
}

void Concat_Dep::normalize_concat(shared_ptr <const Concat_Dep> dep,
				  std::vector <shared_ptr <const Dep> > &deps_,
				  int &error)
{
	size_t k_init= deps_.size();
	normalize_concat(dep, deps_, 0, error);
	if (error && ! option_k)
		return;

	/* Add attributes from DEP */
	if (dep->flags || dep->index >= 0 || dep->top) {
		for (size_t k= k_init; k < deps_.size(); ++k) {
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
				  std::vector <shared_ptr <const Dep> > &deps_,
				  size_t start_index,
				  int &error)
{
	assert(start_index < dep->deps.size());

	if (start_index + 1 == dep->deps.size()) {
		shared_ptr <const Dep> dd= dep->deps.at(start_index);
		if (auto compound_dd= to <Compound_Dep> (dd)) {
			for (const auto &d: compound_dd->deps) {
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
			unreachable();
		}
	} else {
		std::vector <shared_ptr <const Dep> > vec1, vec2;
		normalize_concat(dep, vec2, start_index + 1, error);
		if (error && ! option_k)
			return;
		shared_ptr <const Dep> dd= dep->deps.at(start_index);
		if (auto compound_dd= to <Compound_Dep> (dd)) {
			for (const auto &d: compound_dd->deps) {
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
			unreachable();
		}

		for (const auto &d1: vec1) {
			for (const auto &d2: vec2) {
				shared_ptr <const Dep> d= concat(d1, d2, error);
				if (error && ! option_k)
					return;
				if (d)
					deps_.push_back(d);
			}
		}
	}
}

Hash_Dep Concat_Dep::get_target() const
{
	/* Dep::get_target() is not used for complex dependencies */
	unreachable();
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
				      show(a),
				      show_operator('<'));
		b->get_place() << fmt("because %s is concatenated to it",
				      show(b));
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (b->flags & F_INPUT) {
		/* We don't save the place for the '<', so we cannot
		 * have "using '<'" on an extra line.  */
		b->get_place() << fmt("%s cannot have input redirection using %s",
				      show(b),
				      show_operator('<'));
		a->get_place() << fmt("in concatenation to %s", show(a));
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
				      show(b), flags_phrases[i_flag]);
		b->places[i_flag] << fmt("using %s",
					 show_operator(frmt("-%c", flags_chars[i_flag])));
		a->get_place() << fmt("in concatenation to %s", show(a));
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (b->flags & F_TARGET_TRANSIENT) {
		b->get_place() << fmt("transient target %s is invalid", show(b));
		a->get_place() << fmt("in concatenation to %s", show(a));
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (a->flags & F_VARIABLE) {
		a->get_place() << fmt("the variable dependency %s cannot be used",
				      show(a));
		b->get_place() << fmt("in concatenation with %s",
				      show(b));
		error |= ERROR_LOGICAL;
		return nullptr;
	}

	if (b->flags & F_VARIABLE) {
		b->get_place() << fmt("variable dependency %s is invalid",
				      show(b));
		a->get_place() << fmt("in concatenation to %s", show(a));
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
	assert(! a->place_target.place_name.is_parametrized());
	assert(! b->place_target.place_name.is_parametrized());

	/*
	 * Combine
	 */

	Flags flags_combined= a->flags | b->flags;

	Place_Name place_name_combined(a->place_target.place_name.unparametrized() +
				       b->place_target.place_name.unparametrized(),
				       a->place_target.place_name.place);

	shared_ptr <Plain_Dep> ret=
		std::make_shared <Plain_Dep>
		(flags_combined,
		 a->places,
		 Place_Target(flags_combined & F_TARGET_TRANSIENT,
			      place_name_combined,
			      a->place_target.place),
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

	shared_ptr <Concat_Dep> ret= std::make_shared <Concat_Dep> ();

	if (auto concat_a= to <const Concat_Dep> (a)) {
		for (auto d: concat_a->deps)
			ret->push_back(d);
	} else {
		ret->push_back(a);
	}

	if (auto concat_b= to <const Concat_Dep> (b)) {
		for (auto d: concat_b->deps)
			ret->push_back(d);
	} else {
		ret->push_back(b);
	}

	return ret;
}

void Root_Dep::render(Parts &parts, Rendering) const
{
	parts.append_operator("ROOT");
}
