#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

// FIXME: need to pass back the height...
// Propsoal: we need to 


// FIXME: obtain the height as part of this function?
int
_frcanfit(Frame *f, Point pt, Frbox *b)
{
	int left, nr;
	Rune *p;

	left = f->r.max.x-pt.x;
	if(b->nrune < 0)
		return b->minwid <= left;
	if(left >= b->wid)
		return b->nrune;
	for(nr=0, p=b->ptr; *p; p++,nr++){
		left  -= _srunestringnwidth(p, 1, b->ptags, f->styles, 0).x;
		if(left < 0)
			return nr;
	}
	drawerror(f->display, "_frcanfit can't");
	return 0;
}

void
_frcklinewrap(Frame *f, Point *p, Frbox *b)
{
	if((b->nrune<0 ? b->minwid : b->wid) > f->r.max.x-p->x){
		p->x = f->r.min.x;
		p->y += b->height;
	}
}

/*
	This function is radically different than _frcklinewrap and yet has
	a deceptively similar name. Update so it's easier to understand.
*/
void
_frcklinewrap0(Frame *f, Point *p, Frbox *b, int h)
{
	print("_frcklinewrap0: <%0.*S>, h %d\n", b->nrune, b->ptr, h);
	if(_frcanfit(f, *p, b) == 0){
		p->x = f->r.min.x;
		// Must advance by the height of the line that the box doesn't fit on.
		// This might be flawed if the inserted box in bxscan needs multiple
		// lines?
		// FIXME: max(h, height-of-piece-that-fits)
		p->y += h;
	}
}

/*
	Updates point p to be where the box should be drawn so that it
	displays successfully.
	
	Note that this requires to know the height of the box that *preceeds* b in
	case *b* doesn't actually fit. To address this, we must provide the height
	of the previous box (which we then update with the current height.)
	
	Also returns true if the current box is at the start of a line. height values
	do not have to be correct to correctly determine line breaking. (Which
	I might want to ensure somehow.)
	
	NB: Ths function is remarkably similar to _frcklinewrap and can conceivably
	be merged with that one.
*/
int
_frlinewrappoint(Frame *f, Point *p, Frbox *b, int* height)
{
	if((b->nrune<0 ? b->minwid : b->wid) > f->r.max.x - p->x) {
		p->x = f->r.min.x;
		p->y += *height;
		*height = b->height;
		return 1;
	}
	*height = b->height;
	return 0;
}

void
_fradvance(Frame *f, Point *p, Frbox *b)
{
	if(b->nrune<0 && b->bc=='\n'){
		p->x = f->r.min.x;
		p->y += b->height;
	}else
		p->x += b->wid;
}

int
_frnewwid(Frame *f, Point pt, Frbox *b)
{
	b->wid = _frnewwid0(f, pt, b);
	return b->wid;
}

int
_frnewwid0(Frame *f, Point pt, Frbox *b)
{
	int c, x;

	c = f->r.max.x;
	x = pt.x;
	if(b->nrune>=0 || b->bc!='\t')
		return b->wid;
	if(x+b->minwid > c)
		x = pt.x = f->r.min.x;
	x += f->maxtab;
	x -= (x-f->r.min.x)%f->maxtab;
	if(x-pt.x<b->minwid || x>c)
		x = pt.x+b->minwid;
	return x-pt.x;
}

void
_frclean(Frame *f, Point pt, int n0, int n1)	/* look for mergeable boxes */
{
	Frbox *b;
	int nb, c;

	c = f->r.max.x;
	for(nb=n0; nb<n1-1; nb++){
		b = &f->box[nb];
		_frcklinewrap(f, &pt, b);
		while(b[0].nrune>=0 && nb<n1-1 && b[1].nrune>=0 && pt.x+b[0].wid+b[1].wid<c){
			_frmergebox(f, nb);
			n1--;
			b = &f->box[nb];
		}
		_fradvance(f, &pt, &f->box[nb]);
	}
	for(; nb<f->nbox; nb++){
		b = &f->box[nb];
		_frcklinewrap(f, &pt, b);
		_fradvance(f, &pt, &f->box[nb]);
	}
	f->lastlinefull = 0;
	if(pt.y >= f->r.max.y)
		f->lastlinefull = 1;
}

void
_frdiagdump(Frame *f)
{
	int i;
	Frbox* b;
	print("nbox: %d\tp0: %d\tp1: %d\n", f->nbox, f->p0, f->p1);
	for (i = 0, b = &f->box[0]; i < f->nbox; i++, b++) {
		if (b->nrune > -1)
			print("\t[%d]: wid: %d\tminwid: %d\theight: %d\tascent: %d\tnrune: %d,\t<%0.*S>\n",
				i, b->wid, b->minwid, b->height, b->ascent, b->nrune, b->nrune, b->ptr);
		else
			print("\t[%d]: wid: %d\tminwid: %d\theight: %d\tascent: %d\tnrune: %d\n",
				 i, b->wid, b->minwid, b->height, b->ascent, b->nrune);
	}
}

/*
	Fix up the heights and ascents of boxes so that they are correct
	in the presence of tab boxes and newline boxes.
	
	FIXME: This code should be integrated into the larger frinsert/frdelete
	code paths.
*/
void
_frfixheights(Frame* f) 
{
	Frbox* sb = 0;			/* starting box on line */
	Frbox* eb = 0;			/* ending box on line */
	Frbox*  b = 0;			/* current box on line */
	int nb;
	
	int h = 0;			/* computed height for this line's group of boxes. */
	int a = 0;			/* computed ascent for this line's group of boxes. */
	Point p =  f->r.min; 	/* box corner */
	int height;			/* Advance height. */

	for (b = sb = eb = f->box, nb = 0, height = 0; nb < f->nbox; eb++, nb++) {
		/* You're using a function that won't generate correct points until you've finished running this one... */
		if ( _frlinewrappoint(f, &p, eb, &height))
			sb = eb;
		else if (eb->nrune<0 && eb->bc=='\n') {	/* else here is possibly suspect */
			for(b = sb; b < eb; b++) {
				h = _max(h, b->height);
				a = _max(a, b->ascent);
			}
			for(b = sb; b <= eb; b++) {
				b->height = h;
				b->ascent = a;
			}
			h = 0;
			a = 0;
			sb = eb;
			sb++;
		}
		p.x += b->wid;
	}
}
