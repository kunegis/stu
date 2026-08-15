#include "object.hh"

void Placed_Object::render(Parts &parts, Rendering rendering) const
{
	if (flags & F_PHONY)
		parts.append_marker("@");
	name.render(parts, rendering);
}

void Placed_Object::canonicalize()
{
	name.canonicalize();
}

void render(
	const Placed_Object &object,
	Parts &parts,
	Rendering rendering)
{
	return object.render(parts, rendering);
}

shared_ptr <Placed_Object> Placed_Object::instantiate(
	const std::map <string, string> &mapping) const
{
	return std::make_shared <Placed_Object> (flags, *name.instantiate(mapping), place);
}
