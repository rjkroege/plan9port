#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <cursor.h>
#include <frame.h>

/*
	Design pattern for tests: Return an int: true on success, 0 on fail.
	Generate no output on success. Generate lots of output on fail.
*/
int
testOne(Frame *f)
{
	print("Unimplemented test: fail\n");
	return 1;
}


void
runAllTests(Frame* f)
{
	int success = 1;
	success = success && testOne(f);
	// Insert more here.
	
	if (!success) {
		print("FAIL\n");
		threadexitsall("Test failure");
	}
}
