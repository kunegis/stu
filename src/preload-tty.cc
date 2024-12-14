#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * Act as if every file descriptor is a TTY.
 *
 * ENVIRONMENT
 *	$STU_PRELOAD_TTY May contain "e" and/or "o" to make isatty() return 0 for stderr
 *	and stdout.
 */

#define ENV_STU_PRELOAD_TTY "STU_PRELOAD_TTY"

extern "C"
int isatty(int fd)
{
	static const char *v= NULL;
	if (!v) v= getenv(ENV_STU_PRELOAD_TTY);
	if (!v) v= "";
	struct stat stat;
	int r= fstat(fd, &stat);
	if (r < 0) {
		errno= EBADF;
		return 0;
	}
	if (fd == 1 && strchr(v, 'o')) {
		return 0;
	}
	if (fd == 2 && strchr(v, 'e')) {
		return 0;
	}
	return 1;
}
