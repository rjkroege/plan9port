#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

Point
_frptofcharptb(Frame *f, ulong p, Point pt, int* boxnum)
{
	Rune *s;
	Frbox *b;
	int l, bn;
	STag* t;
	
	bn = *boxnum;

	for(b = &f->box[bn]; bn<f->nbox; bn++,b++){
		_frcklinewrap(f, &pt, b);
		if(p < (l=NRUNE(b))){
			if(b->nrune > 0)
				// FIXME: could simplify?
				// FIXME: is this right? It would seem we're not advancing
				// the style string...
				for(s=b->ptr, t = b->ptags; p>0; s++, p--, t++){
					pt.x += srunestringnwidth(s, 1, t, f->styles);
					if(pt.x>f->r.max.x)
						drawerror(f->display, "frptofchar");
				}
			break;
		}
		p -= l;
		_fradvance(f, &pt, b);
	}
	*boxnum = (bn < f->nbox) ? bn : bn - 1;
	return pt;
}

Point
frptofchar(Frame *f, ulong p)
{
	int n = 0;
	return _frptofcharptb(f, p, f->r.min, &n);
}

Point
_frsptofchar(Frame *f, ulong p, int* box)
{
	return _frptofcharptb(f, p, f->r.min, box);
}

// FIXME: Handle the situation of interest with out the _fradvance
// FIXME: remove this when all uses of this function have been converted to frsorignofchar
Point
_frsptofcharh(Frame *f, ulong p, int* height)
{
	int b = 0;
	Point pt = _frptofcharptb(f, p, f->r.min, &b);
	*height = 	(b >= 0) ? f->box[b].height : 0;
	return pt;
}

Point
_frptofcharnb(Frame *f, ulong p, int nb)	/* doesn't do final _fradvance to next line */
{
	Point pt;
	int nbox;
	int boxn = 0;

	nbox = f->nbox;
	f->nbox = nb;
	pt = _frptofcharptb(f, p, f->r.min, &boxn);
	f->nbox = nbox;
	return pt;
}

static
Point
_frgrid(Frame *f, Point p)
{
	p.y -= f->r.min.y;
	p.y -= p.y%f->font->height;
	p.y += f->r.min.y;
	if(p.x > f->r.max.x)
		p.x = f->r.max.x;
	return p;
}

ulong
frcharofpt(Frame *f, Point pt)
{
	Point qt;
	int bn;
	Rune *s;
	Frbox *b;
	ulong p;
	Rune r;

	pt = _frgrid(f, pt);
	qt = f->r.min;
	for(b=f->box,bn=0,p=0; bn<f->nbox && qt.y<pt.y; bn++,b++){
		_frcklinewrap(f, &qt, b);
		if(qt.y >= pt.y)
			break;
		_fradvance(f, &qt, b);
		p += NRUNE(b);
	}
	for(; bn<f->nbox && qt.x<=pt.x; bn++,b++){
		_frcklinewrap(f, &qt, b);
		if(qt.y > pt.y)
			break;
		if(qt.x+b->wid > pt.x){
			if(b->nrune < 0)
				_fradvance(f, &qt, b);
			else{
				s = b->ptr;
				for(;;){
					if(r == 0)
						drawerror(f->display, "end of string in frcharofpt");
					qt.x += srunestringnwidth(s, 1, b->ptags, f->styles);
					s++;
					if(qt.x > pt.x)
						break;
					p++;
				}
			}
		}else{
			p += NRUNE(b);
			_fradvance(f, &qt, b);
		}
	}
	return p;
}
