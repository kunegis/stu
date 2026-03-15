/*
 * Test that Stu can handle PIDs being returned in any order by successive fork() calls.
 * Since many operating systems will return growing PID numbers, this test is needed to
 * achieve coverage in code in file_executor.cc that handles the pid_t container.
 *
 * Use a (K/2-1)+(K/2) bit unbalanced Feistel network to map PIDs to mock PIDs (MIDs),
 * where K is the number of bits in pid_t.
 *
 *  * Only the lower K-1 bits are pseudorandomized. The sign bit is left intact. Due to K-1
 *    being odd, we need to use an unbalanced Feistel network.
 *  * The value zero is processed specially.  In case 0 is not encrypted to 0, the value
 *    that would otherwise encrypt to 0 is encrypted to the value that 0 would otherwise
 *    encrypt to.
 *  * Mock all functions used by Stu that receive/return PIDs.  For instance, tcgetpgrp()
 *    is not mocked.
 *  * Also mock execve() in order to remove LD_PRELOAD from the environment.
 *  * The returned mock PIDs do not take into account the maximum PID setting of the
 *    operating system.  But that is not used by Stu.
 */

#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef pid_t mid_t;
typedef pid_t left_t; /* K/2-1 bits are used */
typedef pid_t right_t; /* K/2 bits are used */
typedef uint32_t keypart_t;

constexpr int K = sizeof(pid_t) * CHAR_BIT;
constexpr int N = 4;

keypart_t keys[N]= {
	0x1234,
	0x5678,
	0x9ABC,
	0xDEF0
};

left_t f(right_t x, keypart_t key)
{
	return ((1 << (K/2-1)) - 1) & ((x ^ key) * 0x98765);
}

void swap(left_t &l, right_t &r, int dir)
{
	if (dir > 0) {
		left_t l_new= r & ((1 << (K/2-1)) - 1);
		r >>= K/2-1;
		r |= (l << 1);
		l= l_new;
	} else {
		left_t l_new= r >> 1;
		r &= 1;
		r <<= K/2-1;
		r |= l;
		l= l_new;
	}
}

pid_t crypt(pid_t x, int dir)
{
	left_t l= (x >> K/2) & ((1 << (K/2-1)) - 1);
	right_t r= x & ((1 << K/2) - 1);

	for (int i= 0; i < N; ++i) {
		if (i) swap(l, r, dir);
		size_t index= (dir > 0 ? 0 : N-1) + dir * i;
		keypart_t key= keys[index];
		l ^= f(r, key);
	}
	return l << K/2 | r;
}

mid_t encrypt(pid_t pid)
{
	mid_t mid= crypt(pid, +1);
	if (mid == 0) mid= crypt(0, +1);
	return mid;
}

pid_t decrypt(mid_t mid)
{
	pid_t pid= crypt(mid, -1);
	if (pid == 0) pid= crypt(0, -1);
	return pid;
}

extern "C"
pid_t getpid()
{
	pid_t ret= ((pid_t (*)())dlsym(RTLD_NEXT, "getpid"))();
	return encrypt(ret);
}

extern "C"
pid_t fork()
{
	pid_t ret= ((pid_t (*)())dlsym(RTLD_NEXT, "fork"))();
	if (ret > 0)
		ret= encrypt(ret);
	return ret;
}

extern "C"
int execve(const char *pathname, char *const argv[], char *const envp[])
{
	size_t n= 0;
	for (const char *const *p= envp; *envp; ++envp) ++n;
	const char **envp_new= (const char **)malloc(sizeof(const char *) * (n+1));
	if (!envp_new) exit(1);
	const char **p= envp_new;
	constexpr const char *prefix= "LD_PRELOAD=";
	for (const char *const *q= envp; *envp; ++envp) {
		if (!strncmp(prefix, *q, strlen(prefix))) continue;
		*(p++)= *q;
	}
	*p= nullptr;
	int ret= ((int (*)(const char *, char *const[], char *const[]))
		dlsym(RTLD_NEXT, "execve"))(pathname, argv, (char *const *)envp_new);
	return ret;
}

extern "C"
pid_t waitpid(pid_t pid, int *wstatus, int options)
{
	if (pid < -1) pid= - decrypt(-pid);
	if (pid > 0) pid= decrypt(pid);
	pid_t ret= ((pid_t (*)(pid_t, int *, int))dlsym(RTLD_NEXT, "waitpid"))
		(pid, wstatus, options);
	if (ret > 0) ret= encrypt(ret);
	return ret;
}

extern "C"
int kill(pid_t pid, int sig)
{
	if (pid < -1) pid= - decrypt(-pid);
	if (pid > 0) pid= decrypt(pid);
	int ret= ((int (*)(pid_t, int))dlsym(RTLD_NEXT, "kill"))(pid, sig);
	return ret;
}

extern "C"
pid_t getpgid(pid_t pid)
{
	if (pid > 0) pid= decrypt(pid);
	pid_t ret= ((pid_t (*)(pid_t))dlsym(RTLD_NEXT, "getpgid"))(pid);
	if (ret > 0) ret= encrypt(ret);
	return ret;
}

extern "C"
int setpgid(pid_t pid, pid_t pgid)
{
	if (pid > 0) pid= decrypt(pid);
	if (pgid > 0) pgid= decrypt(pgid);
	int ret= ((int (*)(pid_t, pid_t))dlsym(RTLD_NEXT, "setpgid"))(pid, pgid);
	return ret;
}

extern "C"
int tcsetpgrp(int fd, pid_t pgid)
{
	if (pgid > 0) pgid= decrypt(pgid);
	int ret= ((int (*)(int, pid_t))dlsym(RTLD_NEXT, "tcsetpgrp"))(fd, pgid);
	return ret;
}
