#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

#define	SLOP	25

void
_fraddbox(Frame *f, int bn, int n)	/* add n boxes after bn, shift the rest up,
				 * box[bn+n]==box[bn] */
{
	int i;

	if(bn > f->nbox)
		drawerror(f->display, "_fraddbox");
	if(f->nbox+n > f->nalloc)
		_frgrowbox(f, n+SLOP);
	for(i=f->nbox; --i>=bn; )
		f->box[i+n] = f->box[i];
	f->nbox+=n;
}

void
_frclosebox(Frame *f, int n0, int n1)	/* inclusive */
{
	int i;

	if(n0>=f->nbox || n1>=f->nbox || n1<n0)
		drawerror(f->display, "_frclosebox");
	n1++;
	for(i=n1; i<f->nbox; i++)
		f->box[i-(n1-n0)] = f->box[i];
	f->nbox -= n1-n0;
}

void
_frdelbox(Frame *f, int n0, int n1)	/* inclusive */
{
	if(n0>=f->nbox || n1>=f->nbox || n1<n0)
		drawerror(f->display, "_frdelbox");
	_frfreebox(f, n0, n1);
	_frclosebox(f, n0, n1);
}

void
_frfreebox(Frame *f, int n0, int n1)	/* inclusive */
{
	int i;

	if(n1<n0)
		return;
	if(n0>=f->nbox || n1>=f->nbox)
		drawerror(f->display, "_frfreebox");
	n1++;
	for(i=n0; i<n1; i++)
		if(f->box[i].nrune >= 0) {
			free(f->box[i].ptr);
			if (f->box[i].ptags)
				free(f->box[i].ptags);
		}
}

void
_frgrowbox(Frame *f, int delta)
{
	f->nalloc += delta;
	f->box = realloc(f->box, f->nalloc*sizeof(Frbox));
	if(f->box == 0)
		drawerror(f->display, "_frgrowbox");
}

static
void
dupbox(Frame *f, int bn)
{
	Rune *p;
	STag* s;

	if(f->box[bn].nrune < 0)
		drawerror(f->display, "dupbox");
	_fraddbox(f, bn, 1);
	if(f->box[bn].nrune >= 0){
		p = _frallocstr(f, NRUNE(&f->box[bn])+1);
		// runestrcpy(p, f->box[bn].ptr);
		memcpy(p, f->box[bn].ptr, NRUNE(&f->box[bn]) * sizeof(Rune));
		f->box[bn+1].ptr = p;
		f->box[bn+1].ptags = 0;
		if (f->box[bn].ptags) {
			s = _fralloctags(f, NRUNE(&f->box[bn])+1);
			memcpy(s, f->box[bn].ptags, NRUNE(&f->box[bn]) * sizeof(STag));
			f->box[bn+1].ptags = s;
		}
	}
}

static
void
truncatebox(Frame *f, Frbox *b, int n)	/* drop last n chars; no allocation done */
{
	Point pt;

	print("truncatebox dropping %d\n", n);
	print("before: <%0.*S>\n", b->nrune, b->ptr);
	if(b->nrune<0 || b->nrune<n)
		drawerror(f->display, "truncatebox");
	b->nrune -= n;
	// b->ptr[b->nrune] = 0; // FIXME possibly unnecessary.
	pt = _srunestringnwidth(b->ptr, b->nrune, b->ptags, f->styles, &b->ascent);
	b->wid = pt.x;
	b->height = pt.y;
	print("after: <%0.*S>\n", b->nrune, b->ptr);
}

static
void
chopbox(Frame *f, Frbox *b, int n)	/* drop first n chars; no allocation done */
{
	Rune *p;
	Point pt;

	print("chopbox dropping %d\n", n);
	print("before: <%0.*S>\n", b->nrune, b->ptr);
	if(b->nrune<0 || b->nrune<n)
		drawerror(f->display, "chopbox");
	// p = (char*)runeindex(b->ptr, n);
	p = b->ptr + n;
	b->nrune -= n;
	memmove(b->ptr, p, sizeof(Rune) * b->nrune);
	if (b->ptags)
		memmove(b->ptags, b->ptags + n, sizeof(STag) * b->nrune);
	pt = _srunestringnwidth(b->ptr, b->nrune, b->ptags, f->styles, &b->ascent);
	b->wid = pt.x;
	b->height = pt.y;
	print("after: <%0.*S>\n", b->nrune, b->ptr);
}

void
_frsplitbox(Frame *f, int bn, int n)
{
	dupbox(f, bn);
	truncatebox(f, &f->box[bn], f->box[bn].nrune-n);
	chopbox(f, &f->box[bn+1], n);
}

void
_frmergebox(Frame *f, int bn)		/* merge bn and bn+1 */
{
	Frbox *b;

	b = &f->box[bn];
	_frinsure(f, bn, b[0].nrune + b[1].nrune + 1);
	memcpy(b[0].ptr + b[0].nrune, b[1].ptr, (b[1].nrune + 1) * sizeof(Rune));
	if (b->ptags)
		memcpy(b[0].ptags + b[0].nrune, b[1].ptags, (b[1].nrune + 1) * sizeof(STag));
	b[0].wid += b[1].wid;
	b[0].height = _max(b[0].height, b[1].height);
	b[0].ascent = _max(b[0].ascent, b[1].ascent);
	b[0].nrune += b[1].nrune;
	_frdelbox(f, bn+1, bn+1);
}

int
_frfindbox(Frame *f, int bn, ulong p, ulong q)	/* find box containing q and put q on a box boundary */
{
	Frbox *b;

	for(b = &f->box[bn]; bn<f->nbox && p+NRUNE(b)<=q; bn++, b++)
		p += NRUNE(b);
	if(p != q)
		_frsplitbox(f, bn++, (int)(q-p));
	return bn;
}
