#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <cursor.h>
#include <frame.h>

/*
	Implementation note.

	_ystring 
	_ystringnwidth
ystringsize
yrunestringnwidth
ystringnbg
setstagsforrunerange

*/


int
TestIntValue(int expected, int actual, char* complain, char* file, int line) {
	if (expected == actual) {
		return 1;
	}
	print("FAIL %s:%d %d != %d: %s\n",file, line,  expected, actual, complain);
	return 0;
}


#define TESTINTVALUE(e, a, c) TestIntValue(e, a, c, __FILE__, __LINE__)


/*
	Design pattern for tests: Return an int: true on success, 0 on fail.
	Generate no output on success. Generate lots of output on fail.
*/
int
testStringwidth(Frame *f)
{

	int success = 1;
	Font *a13,  *a24;
	Style *guardedstyle, *style;
	char *base, *styletags;	
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

	// TODO(rjkroege): make a better macro.
	w = stringwidth(a13, base);
	success &= TESTINTVALUE(25, w, "bad size stringwidth");

	w = runestringwidth(a13, rbase);
	success &= TESTINTVALUE(25, w, "bad size runestringsize");
	
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

/*
	How do I test the graphics? I imagine that I can record the
	picture request and compare the existing value to what gets
	written into the connection to devdraw. I note in passing that
	this would be easier in go. :-)

	I think that I should use a narrower frame for testing to make
	engendering wrap and the like easier.

	I need to do this with the unmodified tree to get correct baselines...
	As always, I should have written the unit tests first.

	# tasks
	split the unit tests out of frameexplorer.

	compile the unit tests against original frame / graphics code

	tests that are specific to styled frame need to be broken out nicely.

	suppress valgrind complaining about the openfont code?

*/
int
testTwo(Frame *f)
{

	// 1. insert a single normal character: beginning of  block, middle block, end of block
	// validate block structure: 1
	// 1a. insert a single new line at end of block, middle of block, beginning of block


	// what does validation look like? 
	// start by dumping the interesting bits of the block structure.
	// so write the block dumper
	// write block validator


	// 2. insert 2 normal characters, end,middle,beginning


	// 3. insert 1 character between 2, no breaking
	// 4. insert 2 char between 2, no breaking



	// 3. insert a character string with embedded '\n' characters
	// validate block structure	


	// refine this...
	// remove character sequence at start of block, middle, end
	// remove character sequence at start / end that requires merging with previous block

	return 1;
}



/*
	Should I be doing it like this.
*/
void
runAllTests(Frame* f)
{
	int success = 1;
	success = success && testStringwidth(f);
	// Insert more here.
	
	if (!success) {
		print("FAIL\n");
		threadexitsall("Test failure");
	}

	// Clean up before the interactive exploration.
	exit(0);
}
