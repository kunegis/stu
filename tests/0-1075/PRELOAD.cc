/* Assume that 1 is never the PID of a child process of Stu. */

#include <sys/types.h>

extern "C"
pid_t waitpid(pid_t pid, int *wstatus, int options)
{
	return 1;
}
