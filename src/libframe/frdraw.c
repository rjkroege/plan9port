#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

/*
	In the context of sframes where every line can conceivably
	have a different height, this code assumes that the boxes
	have been "cleaned" and so that this function receives
	sequences of valid boxes.
	
	FIXME: Tabs need to get the height of the highest box on the
	line so that co-linear box sequences end up with the same 
	height.
*/
void
_frdrawtext(Frame *f, Point pt, Image *text, Image *back)
{
	Frbox *b;
	int nb;

	for(nb=0,b=f->box; nb<f->nbox; nb++, b++){
		print("frdraw.c:/^_frdrawtext/  pt: %d %d\n",  pt.x, pt.y);
		if(!f->noredraw && b->nrune >= 0)
			srunestringn(f->b, pt, b->ptr, b->nrune, b->ptags, f->styles, b->ascent);
		_frcklinewrap(f, &pt, b);
		print("end of _frdrawtext loop after wrapping: frdraw.c:/^_frdrawtext/  pt: %d %d\n",  pt.x, pt.y);
	}
}

void
frdrawsel(Frame *f, Point pt, ulong p0, ulong p1, int issel)
{
	Image *back, *text;

	if(f->ticked)
		frtick(f, frptofchar(f, f->p0), 0);

	if(p0 == p1){
		frtick(f, pt, issel);
		return;
	}

	if(issel){
		back = f->cols[HIGH];
		text = f->cols[HTEXT];
	}else{
		back = f->cols[BACK];
		text = f->cols[TEXT];
	}

	frdrawsel0(f, pt, p0, p1, back, text);
}

// FIXME: adjust for height.
Point
frdrawsel0(Frame *f, Point pt, ulong p0, ulong p1, Image *back, Image *text)
{
	Frbox *b;
	int nb, nr, w, x, trim;
	Point qt;
	uint p;
	char *ptr;
	
	if(p0 > p1)
		sysfatal("libframe: frdrawsel0 p0=%lud > p1=%lud", p0, p1);

	p = 0;
	b = f->box;
	trim = 0;
	for(nb=0; nb<f->nbox && p<p1; nb++){
		nr = b->nrune;
		if(nr < 0)
			nr = 1;
		if(p+nr <= p0)
			goto Continue;
		if(p >= p0){
			qt = pt;
			_frcklinewrap(f, &pt, b);
			/* fill in the end of a wrapped line */
			if(pt.y > qt.y)
				draw(f->b, Rect(qt.x, qt.y, f->r.max.x, pt.y), back, nil, qt);
		}
		ptr = b->ptr;
		if(p < p0){	/* beginning of region: advance into box */
			ptr += p0-p;
			nr -= (p0-p);
			p = p0;
		}
		trim = 0;
		if(p+nr > p1){	/* end of region: trim box */
			nr -= (p+nr)-p1;
			trim = 1;
		}
		if(b->nrune<0 || nr==b->nrune)
			w = b->wid;
		else
			w = _srunestringnwidth(ptr, nr, b->ptags, f->styles, 0).x;
		x = pt.x+w;
		if(x > f->r.max.x)
			x = f->r.max.x;
		draw(f->b, Rect(pt.x, pt.y, x, pt.y+f->font->height), back, nil, pt);
		if(b->nrune >= 0)
			srunestringn(f->b, pt, ptr, nr, b->ptags, f->styles, 0);
		pt.x += w;
	    Continue:
		b++;
		p += nr;
	}
	/* if this is end of last plain text box on wrapped line, fill to end of line */
	if(p1>p0 &&  b>f->box && b<f->box+f->nbox && b[-1].nrune>0 && !trim){
		qt = pt;
		_frcklinewrap(f, &pt, b);
		if(pt.y > qt.y)
			draw(f->b, Rect(qt.x, qt.y, f->r.max.x, pt.y), back, nil, qt);
	}
	return pt;
}

void
frredraw(Frame *f)
{
	int ticked;
	Point pt;

	if(f->p0 == f->p1){
		ticked = f->ticked;
		if(ticked)
			frtick(f, frptofchar(f, f->p0), 0);
		// FIXME: adjust for styles.
		frdrawsel0(f, frptofchar(f, 0), 0, f->nchars, f->cols[BACK], f->cols[TEXT]);
		if(ticked)
			frtick(f, frptofchar(f, f->p0), 1);
		return;
	}

	// FIXME: see... need to set the background and not take it out of the style.
	// Cause we use the background for drawing the selection highlight.
	// FIXME: should let the draw call override the background from the style.
	pt = frptofchar(f, 0);
	pt = frdrawsel0(f, pt, 0, f->p0, f->cols[BACK], f->cols[TEXT]);
	pt = frdrawsel0(f, pt, f->p0, f->p1, f->cols[HIGH], f->cols[HTEXT]);
	pt = frdrawsel0(f, pt, f->p1, f->nchars, f->cols[BACK], f->cols[TEXT]);
}

#if 0
/*
	This routine is for backwards compatibility.  It does not do
	the right thing when called from outside.  You could preserve
	the api or you could write a better/different routine.

	The possibiity exists that there are many uses of this routine
	in acme and so changing the routine to support them might be
	desirable. (In fact, there are 7 so perhaps updating them all is
	not particularly onerous.)
	
	So: this routine is deprecated as it does the wrong tihing.
*/
void
frtick(Frame *f, Point pt, int ticked)
{
	print("frtick is DEPRECATED. Your cursor won't look right.\n");
	frstick(f, pt, ticked, f->font->height);
}
#endif

/*
	This is the new routine that code should use going forward.
*/
void
frstick(Frame *f, Point pt, int ticked, int h) {
	Rectangle r;

	print("frstick: (%d, %d), height %d, show %d\n", pt.x, pt.y, h, ticked); 
	
	if(f->ticked==ticked || f->tick==0 || !ptinrect(pt, f->r))
		return;
	print("frstick: proceeding\n");
	pt.x -= f->tickscale;	/* looks best just left of where requested */
	r = Rect(pt.x, pt.y, pt.x+FRTICKW*f->tickscale, pt.y + h);
	/* can go into left border but not right */
	if(r.max.x > f->r.max.x)
		r.max.x = f->r.max.x;
	if(ticked){
		/*	My assumption is that the tick is hidden before we move things.
			You will be sad if you violate this assumption. Really. */
		draw(f->tickback, f->tickback->r, f->b, nil, pt);
		_frresizetick(f, h);
		draw(f->b, r, f->tick, nil, ZP);
	}else
		draw(f->b, r, f->tickback, nil, ZP);
	f->ticked = ticked;
}

/*
	TODO(rjkroege): Fix this.
*/
void
frtick(Frame *f, Point pt, int ticked)
{
	print("frtick won't look right.  Your cursor won't look right.\n");
	if(f->tickscale != scalesize(f->display, 1)) {
		if(f->ticked)
			// _frtick(f, pt, 0);
			frstick(f, pt, 0, f->font->height);  
		frinittick(f);
	}
	// _frtick(f, pt, ticked);
	frstick(f, pt, ticked, f->font->height);  
}

Point
_frdraw(Frame *f, Point pt)
{
	Frbox *b;
	int nb, n;

	for(b=f->box,nb=0; nb<f->nbox; nb++, b++){
		_frcklinewrap0(f, &pt, b, b->height);
		if(pt.y == f->r.max.y){
			f->nchars -= _frstrlen(f, nb);
			_frdelbox(f, nb, f->nbox-1);
			break;
		}
		if(b->nrune > 0){
			n = _frcanfit(f, pt, b);
			if(n == 0)
				break;
			if(n != b->nrune){
				_frsplitbox(f, nb, n);
				b = &f->box[nb];
			}
			pt.x += b->wid;
		}else{
			if(b->bc == '\n'){
				pt.x = f->r.min.x;
				pt.y+=f->font->height;
			}else
				pt.x += _frnewwid(f, pt, b);
		}
	}
	return pt;
}

int
_frstrlen(Frame *f, int nb)
{
	int n;

	for(n=0; nb<f->nbox; nb++)
		n += NRUNE(&f->box[nb]);
	return n;
}
