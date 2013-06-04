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

