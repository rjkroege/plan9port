// Workaround for the highsierra pty bug.
// See https://github.com/9fans/plan9port/issues/110 for details.

#include <Availability.h>

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 101300
#include "bsdpty.c"
#else

#define getpts not_using_this_getpts
#include "bsdpty.c"
#undef getpts

int
getpts(int fd[], char *slave) {
	char *a;

	fd[1] = posix_openpt(O_RDWR);
	if (fd[1] < 0) {
		sysfatal("no ptys: open: %r");
	}
	a = ptsname(fd[1]);
	if (a == NULL) {
		sysfatal("no ptys: ptsname: %r");
	}
	if (grantpt(fd[1]) < 0) {
		sysfatal("no ptys: grantpt: %r");
	}
	if (unlockpt(fd[1]) < 0) {
		sysfatal("no ptys: unlockpt: %r");
	}
	strcpy(slave, a);
	if((fd[0] = open(a, O_RDWR)) >= 0)
		return 0;
	sysfatal("no ptys: open slave: %r");
	return 0;
}
#endif
