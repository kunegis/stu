#include "buffering.hh"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "error.hh"

void init_buffering()
{
	/* Set STDOUT to line buffered, so that the output of command lines always happens
	 * before the output of commands themselves.  Setting the buffering like this is
	 * only possible if we have not written anything yet. */
	errno= 0;
	if (0 != setvbuf(stdout, nullptr, _IOLBF, 0)) {
		/* Color has not been initializet yet, so we cannot use print_errno(). */
		fprintf(stderr, "setvbuf: %s\n", errno ? strerror(errno) : "Error");
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
