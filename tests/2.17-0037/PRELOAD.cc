#include <dlfcn.h>
#include <pwd.h>
#include <string.h>

char user_name[]= "abc";
char user_dir[]= "list.";
char user_shell[]= "/bin/sh";

extern "C"
struct passwd *getpwnam(const char *name)
{
	if (name && !strcmp(name, user_name)) {
		static struct passwd pwd;
		pwd.pw_name= user_name;
		pwd.pw_uid= 12345;
		pwd.pw_gid= 321;
		pwd.pw_dir= user_dir;
		pwd.pw_shell= user_shell;
		return &pwd;
	}
	return ((struct passwd * (*)(const char *))dlsym(RTLD_NEXT, "getpwnam"))(name);
}
