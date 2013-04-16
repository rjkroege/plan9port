#include <u.h>
#include <libc.h>
#include <draw.h>

static Rune empty[] = { 0 };
int
_stringnwidth(Font *f, char *s, Rune *r, int len)
{
	int wid, twid, n, max, l;
	char *name;
	enum { Max = 64 };
	ushort cbuf[Max];
	Rune rune, **rptr;
	char *subfontname, **sptr;
	Font *def;
	Subfont *sf;

	if(s == nil){
		s = "";
		sptr = nil;
	}else
		sptr = &s;
	if(r == nil){
		r = empty;
		rptr = nil;
	}else
		rptr = &r;
	twid = 0;
	while(len>0 && (*s || *r)){
		max = Max;
		if(len < max)
			max = len;
		n = 0;
		sf = nil;
		while((l = cachechars(f, sptr, rptr, cbuf, max, &wid, &subfontname)) <= 0){
			if(++n > 10){
				if(*r)
					rune = *r;
				else
					chartorune(&rune, s);
				if(f->name != nil)
					name = f->name;
				else
					name = "unnamed font";
				freesubfont(sf);
				fprint(2, "stringwidth: bad character set for rune 0x%.4ux in %s\n", rune, name);
				return twid;
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
		freesubfont(sf);
		agefont(f);
		twid += wid;
		len -= l;
	}
	return twid;
}

/* Support for measuring the size of a styled string. */
/*
	Optimizations abound: I think that I could end up re-writing all of the Plan9
	font and character drawing code if I want to efficiently support the drawing
	of styled text runs.
*/
Point
_ystringnwidth(Rune* r, char *s,  int len, STag* st, Style* styledefns, int* ascent)
{
	Point pt = Pt(0, 0);
	int asc = 0, o, w;
	Rune rw;
	Style *style;
	STag def = 0;
	
	o = (st) ? 1 : 0;
	st = (st) ? st : &def;

	while( len > 0 && (s || r)){
		if(r || s && ((rw = *s) < Runeself))
			w = 1;
		else
			w = chartorune(&rw, (char*)s);

		// print("%S %d\n", r, *st);
		style = styledefns + *st;
		asc = (style->font->ascent > asc) ? style->font->ascent : asc;
		pt.y = (style->font->height > pt.y) ? style->font->height : pt.y;
		pt.x += _stringnwidth(style->font, s, r, 1);
		len--;
		r +=  (r) ? w : 0;
		s += (s) ? w : 0;
		st += o;
	}
	if (ascent) {
		*ascent = asc;
	}
	return pt;
}

int
stringnwidth(Font *f, char *s, int len)
{
	return _stringnwidth(f, s, nil, len);
}

int
stringwidth(Font *f, char *s)
{
	return _stringnwidth(f, s, nil, 1<<24);
}

Point
stringsize(Font *f, char *s)
{
	return Pt(_stringnwidth(f, s, nil, 1<<24), f->height);
}

int
runestringnwidth(Font *f, Rune *r, int len)
{
	return _stringnwidth(f, nil, r, len);
}

int
runestringwidth(Font *f, Rune *r)
{
	return _stringnwidth(f, nil, r, 1<<24);
}

/* Support for styled strings */
Point
ystringsize(Style *styles, char *s, STag* stags, int* ascent)
{
	return _ystringnwidth(nil, s, 1<<24, stags, styles, ascent);
}

int
ystringwidth(Style* styles, char* s, STag* tags)
{
	return _ystringnwidth(nil, s, 1<<24, tags, styles, nil).x;
}

int
ystringnwidth(Style* styles, char* s, int len, STag* tags)
{
	return _ystringnwidth(nil, s, len, tags, styles, nil).x;
}
