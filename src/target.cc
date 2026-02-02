#include "target.hh"

void Place_Target::render(Parts &parts, Rendering rendering) const
{
	if (flags & F_TARGET_PHONY)
		parts.append_marker("@");
	place_name.render(parts, rendering);
}

void Place_Target::canonicalize()
{
	place_name.canonicalize();
}

void render(
	const Place_Target &place_target,
	Parts &parts,
	Rendering rendering)
{
	return place_target.render(parts, rendering);
}
