#include "buffering.hh"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "error.hh"

void init_buffering()
{
	/* Set STDOUT to line buffered, so that the
	 * output of command lines always happens before the output of
	 * commands themselves */
	/* Note:  Setting the buffering like this is only possible if we
	 * have not written anything yet.  */
	if (0 != setvbuf(stdout, nullptr, _IOLBF, 0)) {
		print_errno("setvbuf");
		exit(ERROR_FATAL);
	}

	/* Set STDOUT to append mode; this is also done by GNU Make */
	int flags= fcntl(fileno(stdout), F_GETFL, 0);
	if (flags >= 0)
		fcntl(fileno(stdout), F_SETFL, flags | O_APPEND);

	/* Set STDERR to append mode; this is also done by GNU Make */
	flags= fcntl(fileno(stderr), F_GETFL, 0);
	if (flags >= 0)
		fcntl(fileno(stderr), F_SETFL, flags | O_APPEND);
}
