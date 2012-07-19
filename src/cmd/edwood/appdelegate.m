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
#import <WebKit/DOMRange.h>

#import "WebViewExtra.h"

#undef Cursor
#undef Point
#undef Rect

#include "eventaugmentation.h"
#include "appdelegate.h"
#include "graphedge.h"
#include "chordingwindow.h"

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
#include "bigarrow.h"
#include "glendapng.h"

// FIXME: sort this.
static void makemenu(void);
static void makewin(char*);
static void setcursor0(Cursor*);
static NSCursor* makecursor(Cursor*);
static void sendmouse(void);
static void getmousepos(NSWindow*);
static void followzoombutton(NSRect, NSWindow*);
static void hidebars(int, NSWindow*);
static void togglefs(NSWindow*);
static void drawimg(NSRect, NSWindow*);
static void flushwin(NSWindow*);
static void makeicon(void);
static uint msec(void);

enum
{
	Winstyle = NSTitledWindowMask
		| NSClosableWindowMask
		| NSMiniaturizableWindowMask
		| NSResizableWindowMask
};

@implementation AppDelegate
- (id) init
{
	if (self = [super init]) {
		needflush = 0;
	}
	return self;
}

- (void)diagnosticMessage
{
	fprint(2, "diagnostic message\n");
}
@synthesize needflush;
@synthesize cursor;
@synthesize contentview;
@synthesize img;

// from in
@synthesize  bigarrow;
@synthesize  kalting;
@synthesize  kbuttons;
@synthesize  mbuttons;
@synthesize mpos;
@synthesize  mscroll;
@synthesize  undoflag;
@synthesize  touchevent;

// FIXME: use bundle loading.
- (NSString*)stringValue
{
	return @"file:///usr/local/plan9/src/cmd/edwood/resources/default.html";
}

- (void) orKbuttons: (int) kb
{
	kbuttons |= kb;
}

- (void)applicationDidFinishLaunching:(id)arg
{
	self.bigarrow = makecursor(&kBigArrow);
	// Change the icon.
	// makeicon();
	makemenu();
	fprint(2, "app finished loading\n");
	[NSApplication
		detachDrawingThread:@selector(callservep9p:)
		toTarget:[self class] withObject:nil];
	makewin(nil);
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
	NSWindow* window = [notification object];

	// my presumption is that this is unnecessary given
	// windowDidEnterFullScreen
	//if(win.isnfs || win.isofs)
	//	hidebars(1, window);

	[[NSApp delegate] setTouchevent: 0];

	getmousepos(window);
	sendmouse();
}
- (void)windowDidResize:(NSNotification*)notification
{
	NSWindow* window = [notification object];

	getmousepos(window);
	sendmouse();
}
- (void)windowDidEndLiveResize:(id)arg
{
	[[[NSApp delegate] contentview] display];
}
- (void)windowDidChangeScreen:(NSNotification*)notification
{
	NSWindow* window = [notification object];
	[window setFrame:[[window screen] frame] display:YES];
}
- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)r
{
	followzoombutton(r, window);
	return YES;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(id)arg
{
	return YES;
}
- (void)windowDidEnterFullScreen:(NSNotification*)notification{
	NSWindow* window = [notification object];
	hidebars(1, window);
}

- (void)windowWillExitFullScreen:(NSNotification*)notification{ 
	NSWindow* window = [notification object];
	hidebars(0, window);
}

- (void)windowDidExitFullScreen:(NSNotification*)notification
{
	NSWindow* window = [notification object];
	NSButton *b;

	b = [window standardWindowButton:NSWindowMiniaturizeButton];

	if([b isEnabled] == 0){
		[b setEnabled:YES];
		hidebars(0, window);
	}
}

+ (void)callservep9p:(id)arg
{
	servep9p();
	[NSApp terminate:self];
}

/*
 * Some sort of timer callback. It is conceivable that this could be removed.
 */
+ (void)callflushwin:(NSTimer*)timer {
	NSWindow* window = [NSApp mainWindow];
	if (window)
		flushwin(window);
}
- (void)calltogglefs:(id)arg{ togglefs([NSApp mainWindow]);}

+ (void)calldrawimg:(NSValue*)v{ drawimg([v rectValue], [NSApp mainWindow]);}
+ (void)callmakewin:(NSValue*)v{ makewin([v pointerValue]);}
+ (void)callsetcursor0:(NSValue*)v{ setcursor0([v pointerValue]);}

- (void) postLdMd {
    NSLog(@"delegate: postLdMd\n");
    [contentview cut: self];
}

- (void) postLdRd {
    NSLog(@"delegate: postLdRd\n");
    [contentview paste: self];
}

- (void) postMdLd { NSLog(@"delegate: postMdLd\n"); }
- (void) postMdRd { NSLog(@"delegate: postMdRd\n"); }

- (void) postMu
{
    NSLog(@"delegate: postMu\n");
    [contentview stringByEvaluatingJavaScriptFromString: @"redstyle.disabled = true;"];
    // Retrieve selection
    DOMRange* middleselection = [contentview selectedDOMRange];
    // FIXME: I need to know which window this is in.
    NSLog(@"FIXME: do something with middle selection: <<%@>>\n", [middleselection text]);
    // Replace selection with previous selection.
    [contentview setSelectedDOMRange: m_leftselection affinity: m_selectionaffinity];
    //[m_leftselection release];
}

- (void) postRdLd { NSLog(@"delegate: postRdLd\n"); }
- (void) postRdMd { NSLog(@"delegate: postRdMd\n"); }

- (void) postRu {
    NSLog(@"delegate: postRu\n"); 
    [contentview stringByEvaluatingJavaScriptFromString: @"greenstyle.disabled = true;"];
    // Retrieve selection
    DOMRange* rightselection = [contentview selectedDOMRange];
    // FIXME: I need to know which window this is in.
    // FIXME: I need to refactor this for middle and right
    // FIXME: I need to make the click use the existing selection.
    NSLog(@"FIXME: do something with right selection: <<%@>>\n", [rightselection text]);

    NSLog(@"hypothesis: a click should make a collapsed range?\n");
    // what does acme do when i right click on a non-selection?
    NSString* searchtarget = nil;
    DOMRange* startrange = nil;
    if (rightselection.collapsed) {
        NSLog(@"    >> collapsed\n");
        // maybe we have clicked on a selection.
        int sr = [m_leftselection compareBoundaryPoints: DOM_START_TO_START sourceRange: rightselection];
        int er = [m_leftselection compareBoundaryPoints: DOM_END_TO_END sourceRange: rightselection];
        NSLog(@"    >> sr: %d er %d\n", sr, er);
        if ((sr == -1 || sr == 0) && (er == 0 || er == 1)) {
            searchtarget = [m_leftselection text];
            startrange = m_leftselection;
        }
    } else {
        searchtarget = [rightselection text];
        startrange = rightselection;
    }
    NSLog(@"searchtarget %@\n", searchtarget);

    /* This implementation depends on a newer version of WebKit. */
    if (searchtarget) {
        DOMRange* newrange = [contentview DOMRangeOfString: searchtarget relativeTo: startrange options: WebFindOptionsWrapAround];
        [contentview setSelectedDOMRange: newrange affinity: m_selectionaffinity];
        // FIXME: these are invalid coordinates. Rectify.
        // Point warppoint = { 10, 10 };
        // setmouse(warppoint);
        // FIXME: warp cursor to a position just inside the range.
    } else {
        [contentview setSelectedDOMRange: m_leftselection affinity: m_selectionaffinity];
    }
}

- (void) preLd { NSLog(@"delegate: preLd\n"); }

- (void) preMd {
    NSLog(@"delegate: preMd\n");

   // Let's proceed in stages.
    // 1. learn / note how to do the task in JS
    // 2. learn / note how to the task from Objective C
    [m_leftselection autorelease];
    m_leftselection = [[contentview selectedDOMRange] retain];
    m_selectionaffinity = [contentview selectionAffinity];
    [contentview stringByEvaluatingJavaScriptFromString: @"redstyle.disabled = false;"];
}

- (void) preRd {
    NSLog(@"delegate: preRd\n");
   [m_leftselection autorelease];
    m_leftselection = [[contentview selectedDOMRange] retain];
    m_selectionaffinity = [contentview selectionAffinity];
    [contentview stringByEvaluatingJavaScriptFromString: @"greenstyle.disabled = false;"];
}

// FIXME: Need a dealloc function.


@end // AppDelegate

static uint
msec(void)
{
	return nsec()/1000000;
}

static void
drawimg(NSRect dr, NSWindow* window) // AppDelegate
{
	static int first = 1;
	NSRect sr;

	if(first){
		[NSTimer scheduledTimerWithTimeInterval:0.033
			target:[AppDelegate class]
			selector:@selector(callflushwin:) userInfo:nil
			repeats:YES];
		first = 0;
	}
	sr =  [[[NSApp delegate] contentview] convertRect:dr fromView:nil];

	if([[[NSApp delegate] contentview] lockFocusIfCanDraw]){

		/*
		 * To round the window's bottom corners, we can use
		 * "NSCompositeSourceIn", but this slows down
		 * trackpad scrolling considerably in Acme.  Else we
		 * can use "bezierPathWithRoundedRect" with "addClip",
		 * but it's still too slow for wide Acme windows.
		 */
		[[[NSApp delegate] img] drawInRect:dr fromRect:sr
//			operation:NSCompositeSourceIn fraction:1
			operation:NSCompositeCopy fraction:1
			respectFlipped:YES hints:nil];

		[[[NSApp delegate] contentview] unlockFocus];
		[[NSApp delegate] setNeedflush: 1];
	}
}

static void
flushwin(NSWindow* window) // AppDelegate
{
	if([NSApp needflush]){
		[window flushWindow];
		[[NSApp delegate] setNeedflush: 0];
	}
}


static void
togglefs(NSWindow* window)
{
	[window toggleFullScreen:nil];
	return;
}

enum
{
	Autohiddenbars = NSApplicationPresentationAutoHideDock
		| NSApplicationPresentationAutoHideMenuBar,

	Hiddenbars = NSApplicationPresentationHideDock
		| NSApplicationPresentationHideMenuBar,
};

static void
hidebars(int set, NSWindow* window)  // AppDelegate
{
	NSScreen *s,*s0;
	uint old, opt;

	s = [window screen];
	s0 = [[NSScreen screens] objectAtIndex:0];
	old = [NSApp presentationOptions];

	if(set && s==s0)
		opt = (old & ~Autohiddenbars) | Hiddenbars;
	else
		opt = old & ~(Autohiddenbars | Hiddenbars);

	if(opt != old)
		[NSApp setPresentationOptions:opt];
}



static void
followzoombutton(NSRect r, NSWindow* window) // AppDelegate
{
	NSRect wr;
	Point p;

	wr = [window frame];
	wr.origin.y += wr.size.height;
	r.origin.y += r.size.height;

	getmousepos(window);
	p.x = (r.origin.x - wr.origin.x) + [[NSApp delegate] mpos].x;
	p.y = -(r.origin.y - wr.origin.y) + [[NSApp delegate] mpos].y;
	setmouse(p);
}

static void
makewin(char *s)
{
	NSRect r, sr;
	ChordingWindow *w;
	Rectangle wr;
	int set;
	WebView* myview;
	AppDelegate* delegate;

	sr = [[NSScreen mainScreen] frame];

	if(s && *s){
		if(parsewinsize(s, &wr, &set) < 0)
			sysfatal("%r");
	}else{
		wr = Rect(0, 0, sr.size.width*2/3, sr.size.height*2/3);
		set = 0;
	}

	/*
	 * The origin is the left bottom corner for Cocoa.
	 */
	r.origin.y = sr.size.height-wr.max.y;
	r = NSMakeRect(wr.min.x, r.origin.y, Dx(wr), Dy(wr));
	r = [NSWindow contentRectForFrameRect:r
		styleMask:Winstyle];

	w = [[ChordingWindow alloc]
		initWithContentRect:r
		styleMask:Winstyle
		backing:NSBackingStoreBuffered defer:NO];
	if(!set)
		[w center];
#if OSX_VERSION >= 100700
	[w setCollectionBehavior:
		NSWindowCollectionBehaviorFullScreenPrimary];
#endif
	[w setContentMinSize:NSMakeSize(128,128)];

	// win.ofs[0] = w;
	delegate = [NSApp delegate];

	// Can I use property access on apple's classes?
	[w setAcceptsMouseMovedEvents:YES];
	[w setDelegate:delegate];
	[w setEventDelegate: delegate];
	[w setDisplaysWhenScreenProfileChanges:NO];

	// win.isofs = 0;
	myview = [[WebView alloc] initWithFrame: r frameName: nil groupName: nil];

	// Config for webview.
	[myview takeStringURLFrom: delegate];
	[myview setSmartInsertDeleteEnabled: false];
	[myview setMaintainsBackForwardList: false];

	delegate.contentview = myview; // Property setter...
	[w setContentView:myview];
	[w makeKeyAndOrderFront:nil];
	[w makeFirstResponder: myview];
}


static void
makemenu(void)
{
	NSMenu *m;
	NSMenuItem *i,*i0;

	m = [NSMenu new];
	i0 = [NSMenuItem new];
	[m addItem:i0];
	[NSApp setMainMenu:m];
	[m release];

	m = [NSMenu new];

	i = [[NSMenuItem alloc] initWithTitle:@"Full Screen"
		action:@selector(calltogglefs:)
		keyEquivalent:@"f"];
	[m addItem:i];
	[i release];

	i = [[NSMenuItem alloc] initWithTitle:@"Quit"
		action:@selector(terminate:)
		keyEquivalent:@"q"];
	[m addItem:i];
	[i release];

	[i0 setSubmenu:m];
	[i0 release];
	[m release];
}

void
setcursor(Cursor *c)
{
	/*
	 * No cursor change unless in main thread.
	 */
	[AppDelegate
		performSelectorOnMainThread:@selector(callsetcursor0:)
		withObject:[NSValue valueWithPointer:c]
		waitUntilDone:YES];
}

static void
setcursor0(Cursor *c)
{
	NSCursor* nsc = makecursor(c);
	if(c)
		[[NSApp delegate] setCursor:nsc];
	else
		[[NSApp delegate] setCursor:nil];
}

static NSCursor*
makecursor(Cursor *c)
{
	NSBitmapImageRep *r;
	NSCursor *d;
	NSImage *i;
	NSPoint p;
	int b;
	uchar *plane[5];

	r = [[NSBitmapImageRep alloc]
		initWithBitmapDataPlanes:nil
		pixelsWide:16
		pixelsHigh:16
		bitsPerSample:1
		samplesPerPixel:2
		hasAlpha:YES
		isPlanar:YES
		colorSpaceName:NSDeviceBlackColorSpace
		bytesPerRow:2
		bitsPerPixel:1];

	[r getBitmapDataPlanes:plane];

	for(b=0; b<2*16; b++){
		plane[0][b] = c->set[b];
		plane[1][b] = c->clr[b];
	}
	p = NSMakePoint(-c->offset.x, -c->offset.y);
	i = [NSImage new];
	[i addRepresentation:r];
	[r release];

	d = [[NSCursor alloc] initWithImage:i hotSpot:p];
	[i release];
	return d;
}

static void
getmousepos(NSWindow* window)
{
	NSPoint p, q;

	p = [window mouseLocationOutsideOfEventStream];
	p = [[[NSApp delegate] contentview] convertPoint:p fromView:nil];

	// This is a disaster for DIP yes?
	q.x = round(p.x);
	q.y = round(p.y);

	[[NSApp delegate] setMpos: q];
	updatecursor(nil);
}

static void
sendmouse(void)
{
	NSSize size;
	int b;

	size = [[[NSApp delegate] contentview] bounds].size;
	mouserect = Rect(0, 0, size.width, size.height);

	b = [[NSApp delegate] kbuttons] | [[NSApp delegate] mbuttons] | [[NSApp delegate] mscroll];
	mousetrack([[NSApp delegate] mpos].x, [[NSApp delegate] mpos].y, b, msec());
	[[NSApp delegate] setMscroll: 0];
}

/*
 * Builds an icon from a png file.
 * Could perhaps use a bundle scheme instead?
 */
// FIXME: not currently in use.
#if 0
static void
makeicon(void)
{
	NSData *d;
	NSImage *i;

	d = [[NSData alloc]
		initWithBytes:glenda_png
		length:(sizeof glenda_png)];

	i = [[NSImage alloc] initWithData:d];
	[NSApp setApplicationIconImage:i];
	[[NSApp dockTile] display];
	[i release];
	[d release];
}
#endif

