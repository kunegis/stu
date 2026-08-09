#include "target.hh"

void Placed_Target::render(Parts &parts, Rendering rendering) const
{
	if (flags & F_TARGET_PHONY)
		parts.append_marker("@");
	name.render(parts, rendering);
}

void Placed_Target::canonicalize()
{
	name.canonicalize();
}

void render(
	const Placed_Target &target,
	Parts &parts,
	Rendering rendering)
{
	return target.render(parts, rendering);
}
