#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>

static bool had_mmap= false;

extern "C"
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	/* Length of the input file */
	if (length == 12) {
		had_mmap= true;
		errno= EACCES;
		return MAP_FAILED;
	}
	return ((void * (*)(void *, size_t, int, int, int, off_t))
		dlsym(RTLD_NEXT, "mmap"))(addr, length, prot, flags, fd, offset);
}

extern "C"
void *realloc(void *ptr, size_t size)
{
	/* Matches the realloc in Tokenizer::read_fd().  This is quite brittle because the
	 * standard library may do the same realloc() call.  Hence we only consider
	 * realloc() calls that come after our mmap() call. */
	if (had_mmap && ptr == nullptr && size == 0x1000) {
		errno= ENOMEM;
		return nullptr;
	}
	return ((void * (*)(void *, size_t))dlsym(RTLD_NEXT, "realloc"))(ptr, size);
}
