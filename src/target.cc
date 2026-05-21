#include "target.hh"

void Placed_Target::render(Parts &parts, Rendering rendering) const
{
	if (flags & F_TARGET_PHONY)
		parts.append_marker("@");
	placed_name.render(parts, rendering);
}

void Placed_Target::canonicalize()
{
	placed_name.canonicalize();
}

void render(
	const Placed_Target &placed_target,
	Parts &parts,
	Rendering rendering)
{
	return placed_target.render(parts, rendering);
}
