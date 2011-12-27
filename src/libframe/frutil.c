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

/*
	The presumption in the original code is that this
	determination happens at the start of the actual text line.
	But this moves us down too far.  So, we treat newline boxes as
	being at the end of lines and wrap then.

	FIXME: note that the change in the ordering assumption of the
	use of _frcklinewrap may cause bugs elsewhere in the code.
*/
void
_frcklinewrap(Frame *f, Point *p, Frbox *b)
{
	if((b->nrune<0? b->minwid : b->wid) > f->r.max.x-p->x){
		p->x = f->r.min.x;
		p->y += b->height;
	}
}

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
	print("nbox: %d\n", f->nbox);
	for (i = 0, b = &f->box[0]; i < f->nbox; i++, b++) {
		if (b->nrune > -1)
			print("\t[%d]: wid: %d\tminwid: %d\theight: %d\tascent: %d\tnrune: %d,\t<%0.*S>\n",
				i, b->wid, b->minwid, b->height, b->ascent, b->nrune, b->nrune, b->ptr);
		else
			print("\t[%d]: wid: %d\tminwid: %d\theight: %d\tascent: %d\tnrune: %d\n",
				 i, b->wid, b->minwid, b->height, b->ascent, b->nrune);
	}
}