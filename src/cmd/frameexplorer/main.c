#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <keyboard.h>
#include <cursor.h>
#include <frame.h>

// lifted from acme
#define	STACK	65536

// ------------------------ the defn's for styled string drawing ------------------


enum {
	// DEFAULTSTYLE,
	BOLD = 1,
	COMMENT = 2,
	TODO = 3,
	KEYWORD = 4,
	TYPE = 5,
	FNAME = 6,
	FCALL =  7,
	LITSTRING = 8,
	// NSTYLE = 255,
};

// Point styledstringn(char *, int,  int, STag *, Style *, Point , Image*, int);
// Point styledrunestringn(Rune *, int,  int, STag *, Style *, Point , Image*, int);
// void styledstringverticalmetrics(int, int, STag*, Style*, int*, int*);

// -----------------------------------


// Add this an optional attribute to a frame
// The frame object has a pointer to this. It could be varied per
// frame or be made consistent across all of the frames.
Style *styles[NSTYLE];

// TODO(rjkroege): Fix these up.
Display		*display;
Image		*screen;
Font			*font;
Mouse		*mouse;
Mousectl		*mousectl;
Keyboardctl	*keyboardctl;
Rectangle	cntlr;		/* control region */
Rectangle	editr;		/* editing region */
Rectangle	textr;		/* text region */
char		*file;

// Entry points here.
void eresized(int new);
void error(Display *d, char *s);
void	mousethread(void*);
void	keyboardthread(void*);

// Control logging
int dumpLotsOfFontificationLogging = 0;

/*
	Goals

	To serve as a lab for drawing a character string containing multiple 
	different styles.

	a. draw strings at several different colors
	b. draw strings in several different sizes
	c. draw strings in several different styles

	d. create a data structure that encodes font, style, color, size and position in a string
	e. draw a single attributed string

	e.1 clean up the code a bit...spa
	f. draw multiple lines of different height of attributed string.
		- it works. sort of
		- we need to correct the baselines.

--> HERE <--
	g. make a frame
	h. get unattributed text working in the frame
	i. get attributed text client side coded
	j. get 
*/


// --------------- local model -----------------------
enum {
	lcBACK,
	lcPAINT,
	lcGREEN,
	lcRED,
	lcNCOL
};

Image *tagcols[lcNCOL];

enum {
	BIGFONT,
	LITTLEFONT,
	NFONT
};

Font *fonts[NFONT];

// The list-of-styles
Style styledefns[NSTYLE];

Rune *codestring = L"main(volatile int argc, char **volatile argv) /* foo */";
Rune *rs1 = L"Άρχιμήδης";
Rune *rs2 = L"Москва - Московский международный портал";
Rune *rs3 = L"くらしの手続きなどの行政サービス案内";
int tagstringlen;
STag *tagstring;
int rs3_len;
STag *rs3_tags;


// The "side" model. We'll begin by using Frame to get a sense of its API
// before attempting to inject new stuff into it.
typedef struct  {
	Frame frame;
	/*
		When we insert this functionality into Frame, tagstring will default to
		null. Setting it to non-null will force a redraw. There needs to be 
		functions that add and update the tagstring. It might have a more
		complex internal representation based on how frame actually works.
	*/
	STag *tagstring;		// Add to frame
	Style *styledefns;		// Add to frame

	// Actual text contents.
	Rune *larger_buffer;
	int lastr;
	int maxr;
	int forg; // origin within frame from larger_buffer;

	// support cursor motion
	int point;
} StyleFrame;

// What will the methods look like?
// the additional sfr is for "styled"
// and takes additional parameters.
// void	sfrinit(Frame*, Rectangle, Font*, Image*, Image**);
// etc.

Image *framecols[NCOL];
StyleFrame sframe;

// ------------------- end of the model ---------------------

// Additional test functions.
void boringFontifyBufferTest(StyleFrame*);
void displayFontiffiedBufferTest(Image*, StyleFrame*, Point);
void dumpTheStyleString(StyleFrame *);


void
threadmain(volatile int argc, char **volatile argv)
{

	if(initdraw(error, 0, "frameexplorer") < 0){
		fprint(2, "frameexplorer: initdraw failed: %r\n");
		threadexitsall("initdraw");
	}

	tagcols[lcBACK] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xDADBDAff);
	tagcols[lcPAINT] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x999999ff);
	tagcols[lcGREEN] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x00dd00ff);
	tagcols[lcRED] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xdd0000ff);

	fonts[BIGFONT] = openfont(display, "/mnt/font/ArialUnicodeMS/24a/font");
	if (!fonts[BIGFONT]) {
		print("couldn't open /mnt/font/ArialUnicodeMS/24a/font\n");
		fonts[BIGFONT] = font;
	}

	fonts[LITTLEFONT] = openfont(display, "/mnt/font/ArialUnicodeMS/10a/font");
	if (!fonts[LITTLEFONT]) {
		print("couldn't open /mnt/font/ArialUnicodeMS/10a/font\n");
	}
	
	// It would be nice if there was a way to adjust the styles in Acme.
	// The colors and fonts need to be preserved to the dump file. And read back.
	// So, just edit the dump file?
	styledefns[DEFAULTSTYLE].font = openfont(display, "/mnt/font/ArialUnicodeMS/13a/font");
	styledefns[DEFAULTSTYLE].src = display->black;
	styledefns[DEFAULTSTYLE].sp = ZP;
	styledefns[DEFAULTSTYLE].bg = 0;
	styledefns[DEFAULTSTYLE].bgp = ZP;

	styledefns[BOLD].font = openfont(display, "/mnt/font/ArialUnicodeMS/24a/font");
	styledefns[BOLD].src = display->black;
	styledefns[BOLD].sp = ZP;
	styledefns[BOLD].bg = tagcols[lcBACK];
	styledefns[BOLD].bgp = ZP;

	styledefns[COMMENT].font = openfont(display, "/mnt/font/Optima-Italic/13a/font");
	styledefns[COMMENT].src = tagcols[lcGREEN];
	styledefns[COMMENT].bg = tagcols[lcBACK];
	styledefns[COMMENT].sp = ZP;
	styledefns[COMMENT].bgp = ZP;

	styledefns[KEYWORD].font = styledefns[DEFAULTSTYLE].font;
	styledefns[KEYWORD].src = tagcols[lcRED];
	styledefns[KEYWORD].bg = nil;
	styledefns[KEYWORD].sp = ZP;
	styledefns[KEYWORD].bgp = ZP;
	
	tagstringlen = runestrlen(codestring);
	tagstring = mallocz(tagstringlen, 1);


	// setup
	setstagsforrunerange(tagstring + 5, KEYWORD, 8);
	setstagsforrunerange(tagstring + 24, KEYWORD, 4);
	setstagsforrunerange(tagstring + 31, KEYWORD,  8);
	setstagsforrunerange(tagstring + 14, KEYWORD, 3);
	setstagsforrunerange(tagstring + 46, COMMENT, 9);
	setstagsforrunerange(tagstring + 0, BOLD, 4);

	// Japanese styling
	rs3_len = runestrlen(rs3);
	rs3_tags = mallocz(tagstringlen, 1);

	// Japanese styling
	rs3_len = runestrlen(rs3);
	rs3_tags = mallocz(tagstringlen, 1);
	setstagsforrunerange(rs3_tags + 4, BOLD, 4);
	setstagsforrunerange(rs3_tags + 14, BOLD, 4);
	
	// need colors
	// adjust the conflicting enum above.
	framecols[BACK] = allocimagemix(display, DPaleyellow, DWhite);
	framecols[HIGH] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DDarkyellow);
	framecols[BORD] = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DYellowgreen);
	framecols[TEXT] = display->black;
	framecols[HTEXT] = display->black;

	// make the frame.
	frsinit(&sframe.frame, rectaddpt(Rect(0, 0, 400, 200), Pt(30, 200)),
			styledefns, 4, screen, framecols);
	// iNB: a styledframe does not use the font
	sframe.styledefns = styledefns;
	sframe.lastr = 0;
	sframe.point = 0;
	sframe.maxr = 100;
	sframe.tagstring = mallocz(sizeof(char) * sframe.maxr, 1);
	sframe.larger_buffer = mallocz(sizeof(Rune) * sframe.maxr, 1);

	// Run unit tests on Frame.
	runAllTests(&sframe.frame);

	// cribbed from acme
	mousectl = initmouse(nil, screen);
	if(mousectl == nil){
		fprint(2, "acme: can't initialize mouse: %r\n");
		threadexitsall("mouse");
	}
	mouse = &mousectl->m;
	keyboardctl = initkeyboard(nil);
	if(keyboardctl == nil){
		fprint(2, "acme: can't initialize keyboard: %r\n");
		threadexitsall("keyboard");
	}
	threadcreate(keyboardthread, nil, STACK);
	threadcreate(mousethread, nil, STACK);
	
	eresized(0);
	// print("exiting?");
	// threadexitsall("threadmain exited");

}

void
keyboardthread(void *v)
{

	Rune r;
	enum { KKey, NKALT };
	static Alt alts[NKALT+1];
	Frame *f = &sframe.frame;

	USED(v);
	alts[KKey].c = keyboardctl->c;
	alts[KKey].v = &r;
	alts[KKey].op = CHANRCV;
	alts[NKALT].op = CHANEND;

	threadsetname("keyboardthread");
	for(;;){
		switch(alt(alts)){
		case KKey:
			print("received rune %d, '%c'\n", r, r);
			// mesg("key event\n");
			
			switch (r) {
			case 8: /*Backspace */
				if (sframe.lastr > 0) {
					print("sframe.lastr %d\n", sframe.lastr);
					sframe.larger_buffer[sframe.lastr] = 0;
					sframe.lastr--;
					sframe.point--;
					
					reFontify(&sframe);

					frdelete(f, sframe.lastr - sframe.forg, sframe.lastr - sframe.forg + 1);
					if (f->nlines < 3 && sframe.forg > 0) {
						print("inside scrolling code\n");
						// iNB maxlines varies with content. this might cause all
						// kinds of bugs.
						// look at acme ../acme/text.c:1531:textbacknl(Text *t, uint p, uint n)
						// the way we deal is we go back 1 line or 128 characters and 
						// then fill. and it's more complicated that I wanted.
						// it might be better to the test harness in Acme
	
						// goal: to re-fill lines.
						// what is going on here?
						while (f->nlines < f->maxlines / 2 + 1 && sframe.forg > 1) {
							frsinsert(f, sframe.larger_buffer + sframe.forg - 1,
									 sframe.larger_buffer + sframe.forg, sframe.tagstring + sframe.forg - 1, 0);
							sframe.forg--;
						}
						if (sframe.forg > 0) {
							// trim last inserted char
							frdelete(f, 0, 1);
							sframe.forg++;
						}
					}
				}
				break;
			case 0x10: /* ^P  */
				print("^p\n");
				break;
			case 0x0e: /* ^N */
				print("^n\n");
				break;
			case 0x06:	/* ^F: was complete */
				print("^f\n");
				frtick(f, frptofchar(f, sframe.point - sframe.forg), 0);
				if (sframe.point < sframe.lastr)
					sframe.point++;
				frtick(f, frptofchar(f, sframe.point - sframe.forg), 1);
				break;
			case 0x2:  /* ^B */
				print("^b\n");
				frtick(f, frptofchar(f, sframe.point - sframe.forg), 0);
				if (sframe.point > 0)
					sframe.point--;
				frtick(f, frptofchar(f, sframe.point - sframe.forg), 1);
				break;			
			default:
				insertCharacter(&sframe, r);
				break;
			}
		
			displayFontiffiedBufferTest(screen, &sframe, Pt(500, 200));
			flushimage(display, 1);
			// break;
		}
	}
}

void
reFontify(StyleFrame* sframe)
{
	if (dumpLotsOfFontificationLogging) {
		print("\n\nbefore\n----\n");
		dumpTheStyleString(sframe);
	}
	boringFontifyBufferTest(sframe);
	if (dumpLotsOfFontificationLogging) {
		print("\nafter\n----\n");
		dumpTheStyleString(sframe);
	}
}


/*
	Insert a Rune r into the specifies sframe.  r must be an
	insertable character -- in particular, not a backspace.
*/
void
insertCharacter(StyleFrame* sframe, Rune r)
{
	Point tp;
	Frame *f = &(sframe->frame);
	// append runes to the text model 
	if (sframe->lastr + 1 == sframe->maxr) {
		print("resizing the buffer\n");
		sframe->larger_buffer = realloc(sframe->larger_buffer, sizeof(Rune) * sframe->maxr * 2);
		sframe->tagstring = realloc(sframe->tagstring, sizeof(char) * sframe->maxr * 2);
		setstagsforrunerange(sframe->tagstring, DEFAULTSTYLE, sframe->maxr * 2);
		sframe->maxr *= 2;
	}

	// FIXME: we want some stupid text insertion.
	// Can maintain an additional character for "the point". And move it around.
	// And insert characters there.

	// Make room for more, have enough space already.
	if (sframe->point != sframe->lastr) {
		memmove(sframe->larger_buffer + sframe->point + 1,
				sframe->larger_buffer + sframe->point,
				sframe->lastr - sframe->point);
	}

	sframe->larger_buffer[sframe->point] = r;

	// Probably broken with the introduction of cursor motion.
	if (f->lastlinefull || f->maxlines - f->nlines < 2) {
		// scroll up a line
		sframe->forg = sframe->forg + f->nchars / 2;
		frdelete(f, 0, f->nchars / 2);
	}

	// Fontify the buffer here.
	sframe->lastr++;
	sframe->point++;
	reFontify(sframe); 
	// FIXME: need to adjust the inserted content based on what's changed while re-fontifying.
	// Observation: considerable room for optimization exists here.

	frsinsert(f, sframe->larger_buffer + sframe->point -1,
			sframe->larger_buffer + sframe->point, sframe->tagstring + sframe->point - 1,
			sframe->point - 1 - sframe->forg);

	tp = frptofchar(f, sframe->frame.p1);
	print("point of char %d: (%d, %d)\n", sframe->frame.p1, tp.x, tp.y);
}

void
mousethread(void *v)
{

	Mouse m;
	enum { MResize, MMouse, NMALT };
	static Alt alts[NMALT+1];

	USED(v);
	threadsetname("mousethread");
	alts[MResize].c = mousectl->resizec;
	alts[MResize].v = nil;
	alts[MResize].op = CHANRCV;
	alts[MMouse].c = mousectl->c;
	alts[MMouse].v = &mousectl->m;
	alts[MMouse].op = CHANRCV;
	alts[NMALT].op = CHANEND;
	
	for(;;){
		switch(alt(alts)){
		case MMouse:
			/*
			 * Make a copy so decisions are consistent; mousectl changes
			 * underfoot.  Can't just receive into m because this introduces
			 * another race; see /sys/src/libdraw/mouse.c.
			 */
			m = mousectl->m;
			 if (m.buttons & 1) {
				// Point rpt = subpt(m.xy, Pt(30, 200));
				// print("mouse draw? %d, %d, %d\n", m.xy.x, m.xy.y, m.msec);
				draw(screen, rectaddpt(Rect(0,0,5,5), m.xy), tagcols[lcPAINT], nil, ZP);
				print("abs pos: %d, %d\n", m.xy.x, m.xy.y);
				flushimage(display, 1);
			}
			break;
		case MResize:
			eresized(1);
			break;
		}
	}
}

// Called implicitly when the window is resized.
// One could 
void
eresized(int new)
{
	int ascent;
	Point p;
	int w;
	if(new && getwindow(display, Refnone) < 0)
		error(display, "can't reattach to window");
	cntlr = insetrect(screen->clipr, 1);
	draw(screen, screen->clipr, display->white, nil, ZP);

	// Note: Rects are positions... Not pt + size
	draw(screen, Rect(100, 100,150,150),tagcols[lcBACK] , nil, ZP);
	draw(screen, Rect(200, 200,250,250),tagcols[lcBACK] , nil, ZP);
	draw(screen, Rect(300, 300,350,350),tagcols[lcBACK] , nil, ZP);
	draw(screen, Rect(500, 200, 900,450), tagcols[lcBACK] , nil, ZP);

	// This has become increasingly superfluous?
	p = string(screen, Pt(100, 100), display->black, ZP, fonts[LITTLEFONT] ,
			"The quick brown fox");
	p = string(screen, p, tagcols[lcGREEN], ZP, fonts[BIGFONT], "saw Quetzcoatl");
	
	draw(screen, rectaddpt(Rect(0, 0, 400, 200), Pt(30, 200)), framecols[BACK], nil, ZP);

	// styledstringn(codestring, 0, strlen(codestring), tagstring, styledefns, Pt(150, 50), screen, 0);  // original
	srunestring(screen, Pt(150, 50), codestring, tagstring, styledefns, 0);

	// FIXME: add styles to at least some of these
	runestring(screen, Pt(450, 50), tagcols[lcGREEN], ZP, fonts[LITTLEFONT], rs1);
	runestring(screen, Pt(550, 50), tagcols[lcGREEN], ZP, fonts[LITTLEFONT], rs2);
	// runestring(screen, Pt(450, 90), tagcols[lcBACK], ZP, fonts[BIGFONT], rs3);
	srunestring(screen, Pt(450, 90), rs3, rs3_tags, styledefns, 0);

	flushimage(display, 1);

	// Unit tests for libdraw changes here.
	// Will want to measure the styled.
	// FIXME: compare the width of all-one-font with same font specificed via styles.
	// FIXME: measure mixed fonts?
	p = runestringsize(fonts[LITTLEFONT], rs1);
	w = runestringwidth(fonts[LITTLEFONT], rs1);
	if (p.x != 42 || p.y != 12 || w != 42) {
		print("1 actual size: %d,%d width: %d of %S\n", p.x, p.y, w, rs1);
		print("expected size: 42,12 width: 42\n\n");
	}

	p = runestringsize(fonts[LITTLEFONT], rs2);
	w = runestringwidth(fonts[LITTLEFONT], rs2);
	if (p.x != 192 || p.y != 12 || w != 192) {
		print("2 actual size: %d,%d width: %d of %S\n", p.x, p.y, w, rs2);
		print("expected size: 192,12 width: 192 \n\n");
	}

	p = runestringsize(fonts[LITTLEFONT], codestring);
	w = runestringwidth(fonts[LITTLEFONT], codestring);
	if (p.x != 191 || p.y != 12 || w != 191) {
		print("3 actual size: %d,%d width: %d of %S\n", p.x, p.y, w, codestring);
		print("expected size: 191,12 width: 191 \n\n");
	}

	// FIXME: add some kidn of validation of the size of the styled string.
	p = _srunestringnwidth(rs3, rs3_len, rs3_tags, styledefns, &w);
	if (p.x != 322 || p.y != 28 || w != 22) {
		print("4 actual size: %d,%d ascent: %d of %S\n", p.x, p.y, w, rs3);
		print("expected size: 322,28 ascent: 22 \n\n");
	}

	p = _srunestringnwidth(codestring, tagstringlen, tagstring, styledefns, &w);
	if (p.x != 288 || p.y != 28 || w != 22) {
		print("5 actual size: %d,%d ascent: %d of %S\n", p.x, p.y, w, codestring);
		print("expected size: 288,28 ascent: 22 \n\n");
	}
}

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

enum {
	fSTART,
	fCOMMENT,
	fKEY,
	fDEF,
	fSEP
};

// This routine is useful. I might want to preserve it for posterity.
void
dumpTheStyleString(StyleFrame *sframe)
{
	int i;
	
	for (i = 0;  i < sframe->lastr; i++) {
		print("%c%c", " *@"[sframe->tagstring[i]], sframe->larger_buffer[i]);
	}				
}

void
boringFontifyBufferTest(StyleFrame* sframe) {
	// Really hokey rules:
	// Comments start with //
	// Second word on line is bold
	// other words alternate between keyword and default
	// print("boringFontifyBufferTest\n");

	// this code is crap.

	int state = fDEF, i, o;
	Rune r;

	for (i = 0, o = 0 ; i <	sframe->lastr; i++) {
		r = sframe->larger_buffer[i];		
		switch (state) {
		case fSTART:
			if (r == ' ' || r == '\t' )
				state = fDEF;
			else if (r == '/' && i + 1 < sframe->lastr && sframe->larger_buffer[i+1] == '/')
				state = fCOMMENT;
			setstagsforrunerange(sframe->tagstring + o, BOLD, i - o);
			o = i;
			break;
		case fDEF:
			if (r == '/' && i + 1 < sframe->lastr && sframe->larger_buffer[i+1] == '/') {
				setstagsforrunerange(sframe->tagstring + o, DEFAULTSTYLE, i - o);
				state = fCOMMENT;
				o = i;
			} else if (r == '\n') {
				setstagsforrunerange(sframe->tagstring + o, DEFAULTSTYLE, i - o);
				o = i + 1;
				state = fDEF;
			} else if (r == ' ' || r == '\t') {
				state = fSTART;
				o = i;
			}
			break;
		case fCOMMENT:
			if (r == '\n') {
				print("end of a comment\n");
				setstagsforrunerange(sframe->tagstring + o, COMMENT, i - o);
				o = i + 1;
				state = fDEF;
			}
			break;
		}		
	}
	
	// clean up
	switch (state) {
	case fSTART:
		setstagsforrunerange(sframe->tagstring + o, BOLD, i - o);
		break;
	case fDEF:
		setstagsforrunerange(sframe->tagstring + o, DEFAULTSTYLE, i - o);
		break;
	case fCOMMENT:
		setstagsforrunerange(sframe->tagstring + o, COMMENT, i - o);
		break;
	}		
}

void
displayFontiffiedBufferTest(Image* dst, StyleFrame* sframe, Point p) {
	int i, o, ascent, height;

	// print("displayFontiffiedBufferTest\n");
	draw(dst, Rect(500, 200, 900,450), tagcols[lcBACK] , nil, ZP);

	for (i  = 0, o = 0; i < sframe->lastr; i++) {
		if (sframe->larger_buffer[i] == '\n' ) {
			styledstringverticalmetrics(nil, sframe->larger_buffer + o, i - o, sframe->tagstring, sframe->styledefns,
					&ascent, &height);
			// styledrunestringn(sframe->larger_buffer, o, i, sframe->tagstring, sframe->styledefns, p, dst, ascent); // original
			srunestringn(dst, p, sframe->larger_buffer + o, i - o, sframe->tagstring + o, sframe->styledefns, ascent);
			p.y += height;
			o = i + 1;
		}
	}
	srunestringn(dst, p, sframe->larger_buffer + o, i - o, sframe->tagstring + o, sframe->styledefns, 0);
}
