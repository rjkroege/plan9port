#include <u.h>
#include <libc.h>
#include <draw.h>
#include <assert.h>

// Sets the styles for multiple positions.
void
setstagsforrunerange(STag *styletags, STag style, int lr)
{
	int i;
	for (i = 0; i < lr; i++) {
		assert(style >= ' ');
		styletags[i] = style;
	}
}

// FIXME: this method needs to vanish with rune measurement.
// the sstringwidth provides the height + ascent
void
styledstringverticalmetrics(char* s, Rune* r, int lr, STag *styletags, Style *styledefns, int *ascent, int *height)
{
	Style *style;
	int i;
	*ascent = 0;
	*height = 0;

	if (r) {
		for (i = 0; r[i] && i < lr; i++) {
			style = styledefns + styletags[i];
			*ascent = (style->font->ascent > *ascent) ? style->font->ascent : *ascent;
			*height = (style->font->height > *height) ? style->font->height : *height;
		}
	} else {
		for (i = 0; s[i] && i < lr; i++) {
			style = styledefns + styletags[i];
			*ascent = (style->font->ascent > *ascent) ? style->font->ascent : *ascent;
			*height = (style->font->height > *height) ? style->font->height : *height;
		}
	}
}

