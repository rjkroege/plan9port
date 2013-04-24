#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

#define	CHUNK	16
#define	ROUNDUP(n)	((n+CHUNK)&~(CHUNK-1))

uchar *
_frallocstr(Frame *f, unsigned n)
{
	uchar *p;

	p = malloc(ROUNDUP(n));
	if(p == 0)
		drawerror(f->display, "out of memory");
	return p;
}

void
_frinsure(Frame *f, int bn, unsigned n)
{
	Frbox *b;
	uchar *p;
	STag* s;

	b = &f->box[bn];
	if(b->nrune < 0)
		drawerror(f->display, "_frinsure");
	if(ROUNDUP(b->nrune) > n)	/* > guarantees room for terminal NUL */
		return;
	p = _frallocstr(f, n);
	b = &f->box[bn];
	memmove(p, b->ptr, NBYTE(b)+1);
	free(b->ptr);
	b->ptr = p;
	if (b->ptags) {
		s = _fralloctags(f, n);
		memmove(s, b->ptags, b->nrune * sizeof(STag));
		free(b->ptags);
		b->ptags = s;
	}
}

STag*
_fralloctags(Frame* f, unsigned n)
{
	print("_fralloctags %d\n", n);
	STag* s;
	s = (STag*)malloc(sizeof(STag) * ROUNDUP(n));
	if (s == 0)
		drawerror(f->display, "out of memory");
	return s;
}
