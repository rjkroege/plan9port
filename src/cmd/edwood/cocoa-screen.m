/*
 * Cocoa's event loop must be in main thread.
 */


// Makesure that we make sending unhandled messages a compile-time error
// if possible.

/*
 * Note that it is difficult to manage includes without breaking many things.
 * because we need some basic stuff at the top.
*/

#define Cursor OSXCursor
#define Point OSXPoint
#define Rect OSXRect

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#undef Cursor
#undef Point
#undef Rect

#include "appwin.h"
#include "appdelegate.h"

#include <u.h>
#include <libc.h>
#include  "cocoa-thread.h"
#include <draw.h>
#include <memdraw.h>
#include <keyboard.h>
#include <cursor.h>
#include "cocoa-screen.h"
#include "osx-keycodes.h"
#include "edwood.h"

AUTOFRAMEWORK(Cocoa)
AUTOFRAMEWORK(WebKit)

// hybrid function, implemented in cocoa-screen.m
Memimage* initimg();

#define panic sysfatal

int usegestures = 0;
int useoldfullscreen = 0;
int usebigarrow = 0;

void
usage(void)
{
	fprint(2, "usage: devdraw (don't run directly)\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	/*
	 * Move the protocol off stdin/stdout so that
	 * any inadvertent prints don't screw things up.
	 */
	dup(0,3);
	dup(1,4);
	close(0);
	close(1);
	open("/dev/null", OREAD);
	open("/dev/null", OWRITE);

#if 0
	ARGBEGIN{
	case 'D':		/* for good ps -a listings */
		break;
	case 'f':
		useoldfullscreen = 1;
		break;
	case 'g':
		usegestures = 1;
		break;
	case 'b':
		usebigarrow = 1;
		break;
	default:
		usage();
	}ARGEND
#endif

	if(OSX_VERSION < 100700)
		[NSAutoreleasePool new];

	[NSApplication sharedApplication];

	// Load a nib file?
	// [NSBundle loadNibNamed:@"myMain" owner:NSApp];

	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
	[NSApp setDelegate:[AppDelegate new]];
	[NSApp activateIgnoringOtherApps:YES];
	[NSApp run];
}







Memimage*
attachscreen(char *label, char *winsize)
{
	static int first = 1;

	if(first)
		first = 0;
	else
		panic("attachscreen called twice");

	if(label == nil)
		label = "gnot a label";

	/*
	 * Create window in main thread, else no cursor
	 * change while resizing.
	 */
	[AppDelegate
		performSelectorOnMainThread:@selector(callmakewin:)
		withObject:[NSValue valueWithPointer:winsize]
		waitUntilDone:YES];
//	makewin(winsize);

	kicklabel(label);
	return initimg();
}




// FIXME: Move to appdelegate.
Memimage*
initimg(void)
{
	Memimage *i;
	NSSize size;
	Rectangle r;
	AppDelegate* delegate = [NSApp delegate];

	size = [[delegate contentview] bounds].size;

	r = Rect(0, 0, size.width, size.height);
	i = allocmemimage(r, XBGR32);
	if(i == nil)
		panic("allocmemimage: %r");
	if(i->data == nil)
		panic("i->data == nil");

	delegate.img = [[NSBitmapImageRep alloc]
		initWithBitmapDataPlanes:&i->data->bdata
		pixelsWide:Dx(r)
		pixelsHigh:Dy(r)
		bitsPerSample:8
		samplesPerPixel:3
		hasAlpha:NO
		isPlanar:NO
		colorSpaceName:NSDeviceRGBColorSpace
		bytesPerRow:bytesperline(r, 32)
		bitsPerPixel:32];

	return i;
}

void
_flushmemscreen(Rectangle r)
{
	NSRect rect;

	rect = NSMakeRect(r.min.x, r.min.y, Dx(r), Dy(r));

	/*
	 * Call "lockFocusIfCanDraw" from main thread, else
	 * we deadlock while synchronizing both threads with
	 * qlock(): main thread must apparently be idle while
	 * we call it.  (This is also why Devdraw shows
	 * occasionally an empty window: I found no
	 * satisfactory way to wait for P9P's image.)
	 */
	[AppDelegate
		performSelectorOnMainThread:@selector(calldrawimg:)
		withObject:[NSValue valueWithRect:rect]
		waitUntilDone:YES];
}

#if 0
// FIXME: unused code.
static void drawresizehandle(NSRect, NSWindow*);
#endif

/*
 * Called from application delegate.
 */


// FIXME: unused code. might need later.
#if 0
enum
{
	Pixel = 1,
	Barsize = 4*Pixel,
	Handlesize = 3*Barsize + 1*Pixel,
};

// Unused function.
static void
drawresizehandle(NSRect dr, NSWindow* window)
{
	NSColor *color[Barsize];
	NSPoint a,b;
	NSRect r;
	NSSize size;
	Point c;
	int i,j;

	size = [[[NSApp delegate] img] size];
	c = Pt(size.width, size.height);
	r = NSMakeRect(0, 0, Handlesize, Handlesize);
	r.origin = NSMakePoint(c.x-Handlesize, c.y-Handlesize);
	if(NSIntersectsRect(r,dr) == 0)
		return;

	[[window graphicsContext] setShouldAntialias:NO];

	color[0] = [NSColor clearColor];
	color[1] = [NSColor darkGrayColor];
	color[2] = [NSColor lightGrayColor];
	color[3] = [NSColor whiteColor];

	for(i=1; i+Barsize <= Handlesize; )
		for(j=0; j<Barsize; j++){
			[color[j] setStroke];
			i++;
			a = NSMakePoint(c.x-i, c.y-1);
			b = NSMakePoint(c.x-2, c.y+1-i);
			[NSBezierPath strokeLineFromPoint:a toPoint:b];
		}
}
#endif

void updatecursor(NSCursor*);





/*
 * Devdraw does not use NSTrackingArea, that often
 * forgets to update the cursor on entering and on
 * leaving the area, and that sometimes stops sending
 * us MouseMove events, at least on OS X Lion.
 */
void
updatecursor(NSCursor* c)
{
	int isdown, isinside;

	isinside = NSPointInRect([[NSApp delegate] mpos], [[[NSApp delegate] contentview] bounds]);
	isdown = ([[NSApp delegate] mbuttons] || [[NSApp delegate] kbuttons]);

	if (!c)
		if([[NSApp delegate] cursor] && (isinside || isdown))
			c = [[NSApp delegate] cursor];
		else if(isinside && usebigarrow)
			c = [[NSApp delegate] bigarrow];
		else
			c = [NSCursor arrowCursor];
	[c set];

	/*
	 * Without this trick, we can come back from the dock
	 * with a resize cursor.
	 */
	if(OSX_VERSION >= 100700)
		[NSCursor unhide];
}






void
setmouse(Point p)
{
	static int first = 1;
	NSPoint q;
	NSRect r;
	NSWindow* window;

	if([NSApp isActive] == 0)
		return;

	window = [NSApp mainWindow];

	if(first){
		/* Try to move Acme's scrollbars without that! */
		CGSetLocalEventsSuppressionInterval(0);
		first = 0;
	}
	[[NSApp delegate] setMpos: NSMakePoint(p.x, p.y)];	// race condition

	r = [[window screen] frame];

	q = [[[NSApp delegate] contentview] convertPoint:[[NSApp delegate] mpos] toView:nil];
	q = [window convertBaseToScreen:q];
	q.y = r.size.height - q.y;

	CGWarpMouseCursorPosition(NSPointToCGPoint(q));
}






// Construct a menu dynamically.




QLock snarfl;

char*
getsnarf(void)
{
	NSPasteboard *pb;
	NSString *s;

	pb = [NSPasteboard generalPasteboard];

	qlock(&snarfl);
	s = [pb stringForType:NSPasteboardTypeString];
	qunlock(&snarfl);

	if(s)
		return strdup((char*)[s UTF8String]);		
	else
		return nil;
}

void
putsnarf(char *s)
{
	NSArray *t;
	NSPasteboard *pb;
	NSString *str;

	if(strlen(s) >= SnarfSize)
		return;

	t = [NSArray arrayWithObject:NSPasteboardTypeString];
	pb = [NSPasteboard generalPasteboard];
	str = [[NSString alloc] initWithUTF8String:s];

	qlock(&snarfl);
	[pb declareTypes:t owner:nil];
	[pb setString:str forType:NSPasteboardTypeString];
	qunlock(&snarfl);

	[str release];
}

void
kicklabel(char *label)
{
	NSString *s;

	if(label == nil)
		return;

	s = [[NSString alloc] initWithUTF8String:label];
	[[NSApp mainWindow] setTitle:s];
	[[NSApp dockTile] setBadgeLabel:s];
	[s release];
}



