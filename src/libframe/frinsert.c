#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

#define	DELTA	25
#define	TMPSIZE	256
static Frame		frame;

/*

   Current plan
   ============

	We must
	a) find the start & end origins (note new type) of inserted region
	b) obtain origin of predecessor box to see if we need to re-draw at new height
	c) reflow the displaced existing boxes as necessary regions, working in origins
	d) move all un-affected boxes down as needed
	e) draw the inserted characters in the newly cleared space

    Note that until the clean step, there can be 2 boxes for the first and
    last lines respectively.

*/

static
Point
bxscan(Frame *f, Rune *sp, Rune *ep, Point *ppt, STag* sstp, int h0)
{
	int w, c, nb, delta, nl, nr, increment, h, a, ca;
	Frbox *b;
#if 0
	// char *s, tmp[TMPSIZE+3];	/* +3 for rune overflow */
	// Rune tmp[TMPSIZE];
#endif

	STag def;
	Rune* p;
	Rune* s;
	STag *starttp;
	Point tp;

	def = DEFAULTSTYLE;
	increment = sstp ? 1 : 0;

	frame.r = f->r;
	frame.b = f->b;
	frame.font = f->font;
	frame.maxtab = f->maxtab;
	frame.nbox = 0;
	frame.nchars = 0;
	memmove(frame.cols, f->cols, sizeof frame.cols);
	frame.styles = f->styles;
	delta = DELTA;
	nl = 0;
	for(nb=0; sp<ep && nl<=f->maxlines; nb++,frame.nbox++){
		if(nb == frame.nalloc){
			_frgrowbox(&frame, delta);
			if(delta < 10000)
				delta *= 2;
		}
		b = &frame.box[nb];
		c = *sp;
		if(c=='\t' || c=='\n'){
			b->bc = c;
			b->wid = 5000;
			b->height = f->font->height;
			b->minwid = (c=='\n')? 0 : stringwidth(frame.font, " ");	// NB: don't have to convert.
			b->nrune = -1;
			if(c=='\n')
				nl++;
			frame.nchars++;
			sp++;
		}else{
			s = sp;
			starttp = sstp;
			nr = 0;
			a = 0;
			w = 0;			
			h = 0;
			while(sp < ep){
				c = *sp;
				if(c=='\t' || c=='\n')
					break;
				// rw = runetochar(s, sp);
				if(sp - s >= TMPSIZE)
					break;
				tp = _srunestringnwidth(sp, 1, sstp, f->styles, &ca);
				w += tp.x;
				h = _max(tp.y, h);
				a = _max(a, ca);
				sp++;
				sstp += increment;
				nr++;
			}
			p = _frallocstr(f, sp - s);
			b = &frame.box[nb];
			b->ptr = p;
			memmove(p, s, (sp - s) * sizeof(Rune));
			if (sstp) {
				b->ptags = _fralloctags(f, sp - s);
				memmove(b->ptags, starttp, (sp - s) * sizeof(STag));
			} else {
				b->ptags = 0;
			}
			b->wid = w;
			b->height = h;
			b->ascent = a;
			b->nrune = nr;
			frame.nchars += nr;
		}
	}
	_frcklinewrap0(f, ppt, &frame.box[0], h0);
	// NB: _frdraw, despite its name, doesn't actually do any drawing.
	return _frdraw(&frame, *ppt);
}

static
void
chopframe(Frame *f, Point pt, ulong p, int bn)
{
	Frbox *b;

	for(b = &f->box[bn]; ; b++){
		if(b >= &f->box[f->nbox])
			drawerror(f->display, "endofframe");
		_frcklinewrap(f, &pt, b);
		if(pt.y >= f->r.max.y)
			break;
		p += NRUNE(b);
		_fradvance(f, &pt, b);
	}
	f->nchars = p;
	f->nlines = f->maxlines;
	if(b<&f->box[f->nbox])				/* BUG */
		_frdelbox(f, (int)(b-f->box), f->nbox-1);
}


/*
	sps is a pointer to the STag* run for the given Rune run. There
	is a 1-to-1 correspondence between Runes and STags.
	sps is allowed to be 0 which makes the font become the default.

	FIXME: make sure that the font is set up correctly.
*/
void
frsinsert(Frame* f, Rune* sp, Rune* ep, STag* sps, ulong p0)
{
	Point pt0, pt1, opt0, ppt0, ppt1, pt;
	Frbox *b;
	int n, n0, nn0, y;
	ulong cn0;
	Image *col, *tcol;
	Rectangle r;
	static struct{
		Point pt0, pt1;
	}*pts;
	static int nalloc=0;
	int npts;

	int h0; 			// FIXME: might not need.


	print("start of frinsert\n");
	_frdiagdump(f);
	print("\n");

	if(p0>f->nchars || sp==ep || f->b==nil)
		return;
	_frsptofcharh(f, p0, &h0);	// FIXME: remove redundant measurement.

	// n0 is the box immediately after the split.
	// Box n0 may be getting shoved onto the end of the last inserted box.
	n0 = _frfindbox(f, 0, 0, p0);
	cn0 = p0;
	nn0 = n0;
	pt0 = _frptofcharnb(f, p0, n0);		// Top left corner of box n0. switch to origin
	ppt0 = pt0;
	opt0 = pt0;
	// Builds a line-broken group of new boxes for the inserted material.
	pt1 = bxscan(f, sp, ep, &ppt0, sps, h0);
	print("\noutput of bxscan @ pt1: %d %d\n", pt1.x, pt1.y);
	_frdiagdump(&frame);

	// What is this for.
	ppt1 = pt1;
	if(n0 < f->nbox){
		_frcklinewrap(f, &pt0, b = &f->box[n0]);	/* for frdrawsel() */
		_frcklinewrap0(f, &ppt1, b, b->height);
	}
	f->modified = 1;
	/*
	 * ppt0 and ppt1 are start and end of insertion as they will appear when
	 * insertion is complete. pt0 is current location of insertion position
	 * (p0); pt1 is terminal point (without line wrap) of insertion.
	 */

	/*
	 * The new boxes inherit height / ascent from the boxes they'll merge with.
	 */
	// FIXME: are we correctly handling inserting of a new line. When we have lines of different height.
	// From here down, we need to reflow the inserted boxes. 
	if (n0 > 0 && n0-1 < f->nbox) {
		frame.box[0].height = _max(frame.box[0].height, f->box[n0-1].height);
		frame.box[0].ascent = _max(frame.box[0].ascent, f->box[n0-1].ascent);
	}
	if (n0 >= 0 && n0 < f->nbox) {
		frame.box[frame.nbox].height = _max(frame.box[frame.nbox].height, f->box[n0].height);
		frame.box[frame.nbox].ascent = _max(frame.box[frame.nbox].ascent, f->box[n0].ascent);
	}

	// Hide/show the cursor
	// FIXME: refactoring opportunities abound.
	if(f->p0 == f->p1) {
		int b0 = 0;
		Point pt = _frsptofchar(f, f->p0, &b0);
		// FIXME: we already have the box num right.
		// we even already have the right height?
		print("n0 == b0?: %d, %d\n", n0, b0);
		frstick(f, pt, 0, (b0 >= 0) ? f->box[b0].height : 0);
	}

/*
	 * Find point where old and new x's line up
	 * Invariants:
	 *	pt0 is where the next box (b, n0) is now
	 *	pt1 is where it will be after the insertion
	 * If pt1 goes off the rectangle, we can toss everything from there on
	 */
	for(b = &f->box[n0],npts=0;
	     pt1.x!=pt0.x && pt1.y!=f->r.max.y && n0<f->nbox; b++,n0++,npts++){
		_frcklinewrap(f, &pt0, b);
		_frcklinewrap0(f, &pt1, b, b->height);
		if(b->nrune > 0){
			n = _frcanfit(f, pt1, b);
			if(n == 0)
				drawerror(f->display, "_frcanfit==0");
			if(n != b->nrune){
				_frsplitbox(f, n0, n);
				b = &f->box[n0];
			}
		}
		if(npts == nalloc){
			pts = realloc(pts, (npts+DELTA)*sizeof(pts[0]));
			nalloc += DELTA;
			b = &f->box[n0];
		}
		pts[npts].pt0 = pt0;
		pts[npts].pt1 = pt1;
		/* has a text box overflowed off the frame? */
		if(pt1.y == f->r.max.y)
			break;
		_fradvance(f, &pt0, b);
		pt1.x += _frnewwid(f, pt1, b);
		cn0 += NRUNE(b);
	}
	if(pt1.y > f->r.max.y)
		drawerror(f->display, "frinsert pt1 too far");
	if(pt1.y==f->r.max.y && n0<f->nbox){
		f->nchars -= _frstrlen(f, n0);
		_frdelbox(f, n0, f->nbox-1);
	}
	if(n0 == f->nbox)
		f->nlines = (pt1.y-f->r.min.y)/f->font->height+(pt1.x>f->r.min.x);
	else if(pt1.y!=pt0.y){
		int q0, q1;

		y = f->r.max.y;
		// FIXME: There is not one font height.
		q0 = pt0.y+f->font->height;
		q1 = pt1.y+f->font->height;
		f->nlines += (q1-q0)/f->font->height;
		if(f->nlines > f->maxlines)
			chopframe(f, ppt1, p0, nn0);
		if(pt1.y < y){
			r = f->r;
			r.min.y = q1;
			r.max.y = y;
			if(q1 < y)
				draw(f->b, r, f->b, nil, Pt(f->r.min.x, q0));
			r.min = pt1;
			r.max.x = pt1.x+(f->r.max.x-pt0.x);
			r.max.y = q1;
			draw(f->b, r, f->b, nil, pt0);
		}
	}

	// My observation is that we need to move down whole lines after n0
	// cause we're going to draw the rest.
	// two more rects: before (first whole line later) and after

	// just clear + draw the line broken rectangles instead of the below.

	// FIXME: for height. oh yeah
	/*
	 * Move the old stuff down to make room.  The loop will move the stuff
	 * between the insertion and the point where the x's lined up.
	 * The draw()s above moved everything down after the point they lined up.
	 */
	for((y=pt1.y==f->r.max.y?pt1.y:0),b = &f->box[n0-1]; --npts>=0; --b){
		pt = pts[npts].pt1;
		if(b->nrune > 0){
			r.min = pt;
			r.max = r.min;
			r.max.x += b->wid;
			r.max.y += f->font->height;
			draw(f->b, r, f->b, nil, pts[npts].pt0);
			/* clear bit hanging off right */
			if(npts==0 && pt.y>pt0.y){
				/*
				 * first new char is bigger than first char we're
				 * displacing, causing line wrap. ugly special case.
				 */
				r.min = opt0;
				r.max = opt0;
				r.max.x = f->r.max.x;
				r.max.y += f->font->height;
				if(f->p0<=cn0 && cn0<f->p1)	/* b+1 is inside selection */
					col = f->cols[HIGH];
				else
					col = f->cols[BACK];
				draw(f->b, r, col, nil, r.min);
			}else if(pt.y < y){
				r.min = pt;
				r.max = pt;
				r.min.x += b->wid;
				r.max.x = f->r.max.x;
				r.max.y += f->font->height;
				if(f->p0<=cn0 && cn0<f->p1)	/* b+1 is inside selection */
					col = f->cols[HIGH];
				else
					col = f->cols[BACK];
				draw(f->b, r, col, nil, r.min);
			}
			y = pt.y;
			cn0 -= b->nrune;
		}else{
			r.min = pt;
			r.max = pt;
			r.max.x += b->wid;
			r.max.y += f->font->height;
			if(r.max.x >= f->r.max.x)
				r.max.x = f->r.max.x;
			cn0--;
			if(f->p0<=cn0 && cn0<f->p1){ /* b is inside selection */
				col = f->cols[HIGH];
				tcol = f->cols[HTEXT];
			}else{
				col = f->cols[BACK];
				tcol = f->cols[TEXT];
			}
			draw(f->b, r, col, nil, r.min);
			y = 0;
			if(pt.x == f->r.min.x)
				y = pt.y;
		}
	}
	/* insertion can extend the selection, so the condition here is different */
	if(f->p0<p0 && p0<=f->p1){
		col = f->cols[HIGH];
		tcol = f->cols[HTEXT];
	}else{
		col = f->cols[BACK];
		tcol = f->cols[TEXT];
	}
	frselectpaint(f, ppt0, ppt1, col);

	/*
		FIXME: if the line height has changed, we ned to push down the
		current line.

		We find the heights of the box(es) enclosing the new boxes and
		adjust the height/ascent from that. (Above)
	*/

	 _frdrawtext(&frame, ppt0, tcol, col);
	_fraddbox(f, nn0, frame.nbox);
	for(n=0; n<frame.nbox; n++)
		f->box[nn0+n] = frame.box[n];
	if(nn0>0 && f->box[nn0-1].nrune>=0 && ppt0.x-f->box[nn0-1].wid>=f->r.min.x){
		--nn0;
		ppt0.x -= f->box[nn0].wid;
	}
	n0 += frame.nbox;
	_frclean(f, ppt0, nn0, n0<f->nbox-1? n0+1 : n0);
	f->nchars += frame.nchars;
	if(f->p0 >= p0)
		f->p0 += frame.nchars;
	if(f->p0 > f->nchars)
		f->p0 = f->nchars;
	if(f->p1 >= p0)
		f->p1 += frame.nchars;
	if(f->p1 > f->nchars)
		f->p1 = f->nchars;
	if(f->p0 == f->p1) {
		int b = 0; 
		Point pt = _frsptofchar(f, f->p0, &b);
		frstick(f, pt, 1, f->box[b].height);
	}
	print("\nEnd of frinsert\n");
	_frdiagdump(f);
}



void
frinsert(Frame *f, Rune *sp, Rune *ep, ulong p0)
{
	frsinsert(f, sp, ep, 0, p0);
}
