#include <pwd.h>
#include <sys/types.h>

extern "C"
struct passwd *getpwuid(uid_t uid)
{
	return nullptr;
}
