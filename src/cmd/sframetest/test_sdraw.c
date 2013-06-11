#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <cursor.h>
#include <frame.h>

#if defined(HAVE_SFRAMES)

int
testStyledStringwidth()
{

	int success = 1;
	Font *a13,  *a24;
	Style *guardedstyle, *style;
	unsigned char *base, *styletags;	
	Rune *rbase;
	int w;


	a13 = openfont(display, "/mnt/font/ArialUnicodeMS/13a/font");
	a24 = openfont(display, "/mnt/font/ArialUnicodeMS/24a/font");

	// We subtract from the provided style constant so make it larger to
	// force the null exception if we are not returning the right value.
	guardedstyle = (Style*)mallocz(sizeof(Style) * 0x40, 1);
	guardedstyle[0x22].font = a13;
	guardedstyle[0x23].font = a24;
	style = guardedstyle + 0x22;

	base = "hello";
	styletags = "     ";
	rbase = L"hello";

	w = ystringwidth(style, base, styletags);
	success &= TESTINTVALUE(25, w, "ystringwidth, char");

	w = ystringwidth(style, base, 0);
	success &= TESTINTVALUE(25, w, "ystringnwidth, char, null style");

	w = stringnwidth(a13, base, 3);
	success &= TESTINTVALUE(16, w, "bad size stringnwidth, char");

	w = ystringnwidth(style, base, 3, styletags);
	success &= TESTINTVALUE(16, w, "ystringnwidth, char, sized");

	w = ystringnwidth(style, base, 3, 0);
	success &= TESTINTVALUE(16, w, "ystringnwidth, char, sized, null style");

	w = yrunestringnwidth(style, rbase, 3, styletags);
	success &= TESTINTVALUE(16, w, "yrunestringnwidth, sized, rune");

	w = yrunestringnwidth(style, rbase, 3, 0);
	success &= TESTINTVALUE(16, w, "yrunestringnwidth, sized, rune, null style");

	free(guardedstyle);
	return success;
}

#else

/*
 * Version of the test that does nothing for execution on trees without sframes.
 */
int
testStyledStringwidth() {
	return 1;
}

#endif