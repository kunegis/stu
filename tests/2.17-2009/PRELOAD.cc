#include <dlfcn.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENV_NAME "STU_PRELOAD_NAME"
#define ENV_I "STU_PRELOAD_I"

char user_dir[PATH_MAX+1];
char user_shell[]= "/bin/sh";

extern "C"
struct passwd *getpwnam(const char *name)
{
	static const char *env_name= getenv(ENV_NAME);
	const char *env_i= getenv(ENV_I);
	static struct passwd pwd;
	if (name && !strcmp(name, env_name)) {
		pwd.pw_name= (char *)env_name;
		pwd.pw_uid= 12345;
		pwd.pw_gid= 321;
		int r= snprintf(user_dir, PATH_MAX+1, "list.%s", env_i);
		if (r < 0) {
			perror("snprintf");
			exit(77);
		}
		if (r > PATH_MAX) {
			fprintf(stderr, "buffer too small\n");
			exit(78);
		}
		pwd.pw_dir= user_dir;
		pwd.pw_shell= user_shell;
		return &pwd;
	}
	return ((struct passwd * (*)(const char *))dlsym(RTLD_NEXT, "getpwnam"))(name);
}
