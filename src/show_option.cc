#include "show_option.hh"

void render(Option_View ov, Parts &parts, Rendering rendering)
{
	parts.append_marker("-");
	render(string(1, ov.c), parts, rendering);
}
