#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <cursor.h>
#include <frame.h>

#include "tests.h"

/*
 * Baseline tests for drawing text strings.
*/
int
testStringwidth()
{

	int success = 1;
	Font *a13,  *a24;
	char *base;
	char *styletags;	
	Rune *rbase;
	int w;

	a13 = openfont(display, "/mnt/font/ArialUnicodeMS/13a/font");
	a24 = openfont(display, "/mnt/font/ArialUnicodeMS/24a/font");

	base = "hello";
	styletags = "     ";
	rbase = L"hello";

	w = stringwidth(a13, base);
	success &= TESTINTVALUE(25, w, "bad size stringwidth");

	w = runestringwidth(a13, rbase);
	success &= TESTINTVALUE(25, w, "bad size runestringsize");

	return success;
}

