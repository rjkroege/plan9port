#ifndef _TESTS_H_
#define _TESTS_H_  

enum {
	/* DEFAULTSTYLE,		From the styled defn */
	BIGGER = 1,
	NSTYLE = 2
};

/* Utilities. */
void runAllTests();
int testIntValue(int, int, char*, char*, int);
#define TESTINTVALUE(e, a, c) testIntValue(e, a, c, __FILE__, __LINE__)

Frame* createFrame();

#if defined(HAVE_SFRAMES)
void setoffsetstagsforrunerange(STag*, STage, int);

#endif

/* Tests */
int testStyledStringwidth();
int testStringwidth();

#endif // _TESTS_H_
