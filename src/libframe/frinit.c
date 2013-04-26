#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

void
frinit(Frame *f, Rectangle r, Font *ft, Image *b, Image *cols[NCOL])
{
	f->styles = &(f->defaultstyle);
	f->styles->font = ft;
	f->font = ft;	/* Provide backwards compatability. */

	f->display = b->display;
	f->maxtab = 8*stringwidth(ft, "0");
	f->nbox = 0;
	f->nalloc = 0;
	f->nchars = 0;
	f->nlines = 0;
	f->p0 = 0;
	f->p1 = 0;
	f->box = 0;
	f->lastlinefull = 0;
	if(cols != 0)
		memmove(f->cols, cols, sizeof f->cols);

	// Setup-default.
	f->styles->bg = f->cols[BACK];
	f->styles->bgp = ZP;
	f->styles->src = f->cols[TEXT];
	f->styles->sp = ZP;
	f->mheight = ft->height;
	f->mascent = ft->ascent;

	frsetrects(f, r, b);
	if(f->tick==nil && f->cols[BACK]!=0)
		frinittick(f);
	f->msc = 1;
}

void
frinittick(Frame *f)
{
	Image *b;

	if(f->cols[BACK] == nil || f->display == nil)
		return;
	f->tickscale = scalesize(f->display, 1);
	b = f->display->screenimage;
	if(f->tick)
		freeimage(f->tick);
	f->tick = allocimage(f->display, Rect(0, 0, f->tickscale*FRTICKW, f->mheight), b->chan, 0, DWhite);
	if(f->tick == nil)
		return;
	if(f->tickback)
		freeimage(f->tickback);
	f->tickback = allocimage(f->display, f->tick->r, b->chan, 0, DWhite);
	if(f->tickback == 0){
		freeimage(f->tick);
		f->tick = 0;
		return;
	}
	/* background color */
	draw(f->tick, f->tick->r, f->cols[BACK], nil, ZP);
	/* vertical line */
	draw(f->tick, Rect(f->tickscale*(FRTICKW/2), 0, f->tickscale*(FRTICKW/2+1), f->mheight),
			f->display->black, nil, ZP);
	/* box on each end */
	draw(f->tick, Rect(0, 0, f->tickscale*FRTICKW, f->tickscale*FRTICKW), f->cols[TEXT], nil, ZP);
	draw(f->tick, Rect(0, f->mheight-f->tickscale*FRTICKW, f->tickscale*FRTICKW, f->mheight), 
			f->cols[TEXT], nil, ZP);
}

void
frsetrects(Frame *f, Rectangle r, Image *b)
{
	f->b = b;
	f->entire = r;
	f->r = r;
	f->r.max.y -= (r.max.y-r.min.y)%f->mheight;
	f->maxlines = (r.max.y-r.min.y)/f->mheight;
}

void
frclear(Frame *f, int freeall)
{
	if(f->nbox)
		_frdelbox(f, 0, f->nbox-1);
	if(f->box)
		free(f->box);
	if(freeall){
		freeimage(f->tick);
		freeimage(f->tickback);
		f->tick = 0;
		f->tickback = 0;
	}
	f->box = 0;
	f->ticked = 0;
}

// FIXME: compute mheight
void
frsinit(Frame *f, Rectangle r, Style* sdefns, int sc, Image *b, Image *cols[NCOL])
{
	int i;
	Style *s;

	fprint(2,"hello......\n");

	fprint(2,"s: %ld\n", sdefns[0].font);
	fprint(2,"s: %ld\n", sdefns[3].font);

	f->display = b->display;

	// widest? default font?
	f->maxtab = 8*stringwidth(sdefns[DEFAULTSTYLE].font, "0");
	f->nbox = 0;
	f->nalloc = 0;
	f->nchars = 0;
	f->nlines = 0;
	f->p0 = 0;
	f->p1 = 0;
	f->box = 0;
	f->lastlinefull = 0;
	f->msc = (sc > NSTYLE) ?  NSTYLE: sc ;
	f->styles = sdefns;

print("frsinit 1\n");
	f->mheight = 0;
	f->mascent = 0;
	for (i = 0, s = f->styles; i < f->msc; i++, s++) {
		print("i: %d, %d\n", i, f->msc);
		print("s: %d\n", s->font);
		f->mheight = (s->font->height > f->mheight) ? s->font->height : f->mheight;
		f->mascent = (s->font->ascent > f->mascent) ? s->font->ascent : f->mascent;
            }

print("frsinit 1.1\n");
	// Might need to adjust this?
	if(cols != 0)
		memmove(f->cols, cols, sizeof f->cols);
	frsetrects(f, r, b);
	if(f->tick==nil && f->cols[BACK]!=0)
		frinittick(f);
print("frsinit 2\n");
	
	// TODO(rjk): note need to do something better with background color
	// Colour fallback
	for (; sc > 0; sc--, sdefns++) {
		// If back is 0, we paint on the existing background
		if (sdefns->src == 0)
			sdefns->src = f->cols[TEXT];
	}
print("frsinit end\n");

}
