#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <cursor.h>
#include <frame.h>

#include "tests.h"


/* To be used from TEST* macros. */
int
testIntValue(int expected, int actual, char* complain, char* file, int line) {
	if (expected == actual) {
		return 1;
	}
	print("FAIL %s:%d %d != %d: %s\n", file, line,  expected, actual, complain);
	return 0;
}

void
runAllTests()
{
	int success = 1;
	success = success && testStringwidth();
	success = success && testStyledStringwidth();
	
	if (!success) {
		print("FAIL\n");
		threadexitsall("Test failure");
	}
	print("OK\n");
}

/*
 * Caller frees. Logic is that I will want to create many. Perhaps more than one
 * at a time.
 */
Image**
imageColorCreate() {
	Image** framecols = (Image**)mallocz(sizeof(Image) * NCOL, 1);
	framecols[BACK] = allocimagemix(display, DPaleyellow, DWhite);
	framecols[HIGH] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DDarkyellow);
	framecols[BORD] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DYellowgreen);
	framecols[TEXT] = display->black;
	framecols[HTEXT] = display->black;
	return framecols;
}

Frame*
frameCreate(Font* font, Image* screen, Image** framecols) {
	Frame* frame = (Frame*)mallocz(sizeof(Frame), 1);

	/*	Frame is made deliberately narrow so that I don't have insert
	 	 as much text to actually get breaks in various circumstances. */
	frinit(frame,
		rectaddpt(Rect(0, 0, 150, 200), Pt(30, 200)),
		font,
		screen,
		framecols);
	return frame;
}

#if defined(HAVE_SFRAMES)
void
setoffsetstagsforrunerange(STag *styletags, STag style, int lr) {
	setstagsforrunerange(styletags, style + ' ', lr);
}

Frame*
frameCreateStyled(Style* styledefns, Image* screen, Image* framecols) {
	Frame* frame = (Frame*)mallocz(sizeof(Frame), 1);
	frsinit(frame,
		rectaddpt(Rect(0, 0, 150, 200), Pt(30, 200)),
		styledefns, NSTYLE,
		screen,
		framecols);
	return frame;
}

Style*
styleCreate() {
	Style* styledefns = (Style*)mallocz(sizeof(Style)* NSTYLE, 1);
	Image* bg = allocimagemix(display, DPaleyellow, DWhite);

	styledefns[DEFAULTSTYLE].font =
			openfont(display, "/mnt/font/ArialUnicodeMS/13a/font");
	styledefns[DEFAULTSTYLE].src = display->black;
	styledefns[DEFAULTSTYLE].sp = ZP;
	styledefns[DEFAULTSTYLE].bg = 0;
	styledefns[DEFAULTSTYLE].bgp = ZP;

	styledefns[BIGGER].font = openfont(display, "/mnt/font/ArialUnicodeMS/24a/font");
	styledefns[BIGGER].src = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x00dd00ff);;
	styledefns[BIGGER].sp = ZP;
	styledefns[BIGGER].bg = bg;
	styledefns[BIGGER].bgp = ZP;

	return styledefns;
}


#endif

/*
	Mini-note on how to test drawing. Remember that we speak to devdraw via
	a pipe. Therefore, it ought to be possible to write a devdraw recorder. And 
	then do string comparison on a constant string.
*/
