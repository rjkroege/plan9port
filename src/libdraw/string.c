#include <u.h>
#include <libc.h>
#include <draw.h>

enum
{
	Max = 100
};

Point
string(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s)
{
	return _string(dst, pt, src, sp, f, s, nil, 1<<24, dst->clipr, nil, ZP, SoverD);
}

Point
stringop(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, Drawop op)
{
	return _string(dst, pt, src, sp, f, s, nil, 1<<24, dst->clipr, nil, ZP, op);
}

Point
stringn(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, int len)
{
	return _string(dst, pt, src, sp, f, s, nil, len, dst->clipr, nil, ZP, SoverD);
}

Point
stringnop(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, int len, Drawop op)
{
	return _string(dst, pt, src, sp, f, s, nil, len, dst->clipr, nil, ZP, op);
}

Point
runestring(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r)
{
	return _string(dst, pt, src, sp, f, nil, r, 1<<24, dst->clipr, nil, ZP, SoverD);
}

Point
runestringop(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, Drawop op)
{
	return _string(dst, pt, src, sp, f, nil, r, 1<<24, dst->clipr, nil, ZP, op);
}

Point
runestringn(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, int len)
{
	return _string(dst, pt, src, sp, f, nil, r, len, dst->clipr, nil, ZP, SoverD);
}

Point
runestringnop(Image *dst, Point pt, Image *src, Point sp, Font *f, Rune *r, int len, Drawop op)
{
	return _string(dst, pt, src, sp, f, nil, r, len, dst->clipr, nil, ZP, op);
}

/*	Support for styled strings. */
// There may remain issues associated with background drawing.
// TODO(rjkroege): Lots of optimizations possible.
// TODO(rjkroege): Am I doing the right thing with runes?
Point
_ystring(Image *dst, Point pt, char *s, Rune *r, int len, Rectangle clipr, Image* bg, Point bgp, Drawop op, STag *t, Style *styledefns, int mheight, int mascent)
{
	Style *style;
	Point p;
	int i, o, w;
	STag def = 0;
	Rune rw;
	
	// Not all character cells will paint the entire background which
	// will leave little ugly pixel damages.
           p =  _ystringnwidth(r, s, len, t, styledefns, nil);
	draw(dst, Rect(pt.x, pt.y, pt.x + p.x, pt.y + mheight), bg, nil, bgp);

	o = (t) ? 1 : 0;
	t = (t) ? t : &def;

	// FIXME: Should aggregate sequences of letters of identical style.
	// High-road path is to push this all the way to devdraw.
	if (s) {
		// print("_ystring: s != 0\n");
		for (i = 0; i < len && *s; i++, t += o, s += w) {
			// print("_ystring: %d %d\n", i, len);
			if((rw = *s) < Runeself)
				w = 1;
			else
				w = chartorune(&rw, (char*)s);
			style = styledefns + *t;
			p = _string(dst, Pt(pt.x, pt.y + mascent - style->font->ascent), style->src, style->sp,
					style->font, s, 0, 1, clipr, bg, bgp, op);
			pt.x = p.x;
		}
	} else {
		for (i = 0; r[i] && i < len; i++, t += o) {
			style = styledefns + *t;
			p = _string(dst, Pt(pt.x, pt.y + mascent - style->font->ascent), style->src, style->sp,
					style->font, 0, r + i, 1, clipr, bg, bgp, op);
			pt.x = p.x;
		}
	}
	return pt;
}

Point
_string(Image *dst, Point pt, Image *src, Point sp, Font *f, char *s, Rune *r, int len, Rectangle clipr, Image *bg, Point bgp, Drawop op)
{
	int m, n, wid, max;
	ushort cbuf[Max], *c, *ec;
	uchar *b;
	char *subfontname;
	char **sptr;
	Rune **rptr;
	Font *def;
	Subfont *sf;

	if(len < 0)
		sysfatal("libdraw: _string len=%d", len);

	if(s == nil){
		s = "";
		sptr = nil;
	}else
		sptr = &s;
	if(r == nil){
		r = (Rune*) L"";
		rptr = nil;
	}else
		rptr = &r;
	sf = nil;
	while((*s || *r) && len){
		max = Max;
		if(len < max)
			max = len;
		n = cachechars(f, sptr, rptr, cbuf, max, &wid, &subfontname);
		if(n > 0){
			_setdrawop(dst->display, op);

			m = 47+2*n;
			if(bg)
				m += 4+2*4;
			b = bufimage(dst->display, m);
			if(b == 0){
				fprint(2, "string: %r\n");
				break;
			}
			if(bg)
				b[0] = 'x';
			else
				b[0] = 's';
			BPLONG(b+1, dst->id);
			BPLONG(b+5, src->id);
			BPLONG(b+9, f->cacheimage->id);
			BPLONG(b+13, pt.x);
			BPLONG(b+17, pt.y+f->ascent);
			BPLONG(b+21, clipr.min.x);
			BPLONG(b+25, clipr.min.y);
			BPLONG(b+29, clipr.max.x);
			BPLONG(b+33, clipr.max.y);
			BPLONG(b+37, sp.x);
			BPLONG(b+41, sp.y);
			BPSHORT(b+45, n);
			b += 47;
			if(bg){
				BPLONG(b, bg->id);
				BPLONG(b+4, bgp.x);
				BPLONG(b+8, bgp.y);
				b += 12;
			}
			ec = &cbuf[n];
			for(c=cbuf; c<ec; c++, b+=2)
				BPSHORT(b, *c);
			pt.x += wid;
			bgp.x += wid;
			agefont(f);
			len -= n;
		}
		if(subfontname){
			freesubfont(sf);
			if((sf=_getsubfont(f->display, subfontname)) == 0){
				def = f->display ? f->display->defaultfont : nil;
				if(def && f!=def)
					f = def;
				else
					break;
			}
			/* 
			 * must not free sf until cachechars has found it in the cache
			 * and picked up its own reference.
			 */
		}
	}
	return pt;
}
