#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <cursor.h>
#include <frame.h>

#include "tests.h"

// lifted from acme
#define	STACK	65536

/* Useful globals */
Display		*display;
Image		*screen;
Mouse		*mouse;
Mousectl		*mousectl;
Keyboardctl	*keyboardctl;
char		*file;

/* Entry points here. */
void eresized(int new);
void error(Display *d, char *s);
void	mousethread(void*);
void	keyboardthread(void*);


void
threadmain(volatile int argc, char **volatile argv)
{

	if(initdraw(error, 0, "frameexplorer") < 0){
		fprint(2, "frameexplorer: initdraw failed: %r\n");
		threadexitsall("initdraw");
	}

	runAllTests();
	threadexitsall("threadmain exited");
}

void
keyboardthread(void *v) {}

void
mousethread(void *v) {}

void
eresized(int new) {}

void
error(Display *d, char *s)
{
	USED(d);

	if(file)
		print("can't read %s: %s: %r", file, s);
	else
		print("/dev/bitblt error: %s", s);
	threadexitsall(s);
}


