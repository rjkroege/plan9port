/*
 * Cocoa's event loop must be in main thread.
 */


// Makesure that we make sending unhandled messages a compile-time error
// if possible.

#define Cursor OSXCursor
#define Point OSXPoint
#define Rect OSXRect

#import <Cocoa/Cocoa.h>

#undef Cursor
#undef Point
#undef Rect

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

AUTOFRAMEWORK(Cocoa)

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

// FIXME: Move this to a different file
@interface AppDelegate : NSObject {
	int needflush;
	NSView* contentview;
	NSCursor* cursor;
}

- (id) init;

@property int needflush;
@property (retain) NSCursor* cursor;
@property (retain) NSView* contentview;
@property (retain) NSBitmapImageRep* img;

// From in.
@property (retain) NSCursor* bigarrow;
@property int kalting;
@property int kbuttons;
@property int mbuttons;
@property NSPoint mpos;
@property int mscroll;
@property int undoflag;
@property int touchevent;


- (void)applicationDidFinishLaunching:(id)arg;
- (void)windowDidBecomeKey:(NSNotification*)notification;
- (void)windowDidResize:(NSNotification*)notification;
- (void)windowDidEndLiveResize:(id)arg;
- (void)windowDidChangeScreen:(NSNotification*)notification;
- (BOOL)windowShouldZoom:(NSWindow*)window toFrame:(NSRect)r;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(id)arg;
- (void)windowDidEnterFullScreen:(NSNotification*)notification;
- (void)windowWillExitFullScreen:(NSNotification*)notification;
- (void)windowDidExitFullScreen:(NSNotification*)notification;
- (void)calltogglefs:(id)arg;

- (void) orKbuttons: (int)k;

- (void)diagnosticMessage;

@end

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

static void hidebars(int, NSWindow*);
static void drawimg(NSRect, NSWindow*);
static void flushwin(NSWindow*);
static void followzoombutton(NSRect, NSWindow*);
static void getmousepos(NSWindow*);
static void makeicon(void);
static void makemenu(void);
static void makewin(char*);
static void sendmouse(void);
static void setcursor0(Cursor*);
static void togglefs(NSWindow*);

static NSCursor* makecursor(Cursor*);

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
@end // AppDelegate


static Memimage* initimg(void);

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

@interface appwin : NSWindow @end
@interface contentview : NSView @end

@implementation appwin
- (NSTimeInterval)animationResizeTime:(NSRect)r
{
	return 0;
}
- (BOOL)canBecomeKeyWindow
{
	return YES;	/* else no keyboard for old fullscreen */
}
@end

enum
{
	Winstyle = NSTitledWindowMask
		| NSClosableWindowMask
		| NSMiniaturizableWindowMask
		| NSResizableWindowMask
};

static void
makewin(char *s)
{
	NSRect r, sr;
	NSWindow *w;
	Rectangle wr;
	int set;
	NSView* myview;
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

	w = [[appwin alloc]
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
	[w setDisplaysWhenScreenProfileChanges:NO];

	// win.isofs = 0;
	myview = [contentview new];
	delegate.contentview = myview; // Property setter...
	[w setContentView:myview];
	[w makeKeyAndOrderFront:nil];
}

static Memimage*
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

static void drawresizehandle(NSRect, NSWindow*);

/*
 * Called from application delegate.
 */
static void
drawimg(NSRect dr, NSWindow* window)
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
flushwin(NSWindow* window)
{
	if([NSApp needflush]){
		[window flushWindow];
		[[NSApp delegate] setNeedflush: 0];
	}
}

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

static void
resizeimg()
{
	[[[NSApp delegate] img] release];
	_drawreplacescreenimage(initimg());
	mouseresized = 1;
	sendmouse();
}

static void getgesture(NSEvent*);
static void getkeyboard(NSEvent*);
static void getmouse(NSEvent*, NSWindow*);
static void gettouch(NSEvent*, int);
static void updatecursor(NSCursor*);

@implementation contentview

- (void)drawRect:(NSRect)r
{
	static int first = 1;

	if([[self window] inLiveResize])
		return;

	if(first)
		first = 0;
	else
		resizeimg();

	/* We should wait for P9P's image here. */
}
- (BOOL)isFlipped
{
	return YES;	/* to make the content's origin top left */
}
- (BOOL)acceptsFirstResponder
{
	return YES;	/* else no keyboard */
}
- (id)initWithFrame:(NSRect)r
{
	[super initWithFrame:r];
	[self setAcceptsTouchEvents:YES];
	return self;
}
- (void)cursorUpdate:(NSEvent*)e{ updatecursor(nil);}

- (void)mouseMoved:(NSEvent*)e{
	// fprint(2, "got a mousemotion event\n");
	getmouse(e, [e window]);
}

- (void)mouseDown:(NSEvent*)e{
	[[NSApp delegate] diagnosticMessage];
	getmouse(e, [e window]);
}
- (void)mouseDragged:(NSEvent*)e{
	getmouse(e, [e window]);
}
- (void)mouseUp:(NSEvent*)e{ getmouse(e, [e window]);}
- (void)otherMouseDown:(NSEvent*)e{ getmouse(e, [e window]);}
- (void)otherMouseDragged:(NSEvent*)e{ getmouse(e, [e window]);}
- (void)otherMouseUp:(NSEvent*)e{ getmouse(e, [e window]);}
- (void)rightMouseDown:(NSEvent*)e{ getmouse(e, [e window]);}
- (void)rightMouseDragged:(NSEvent*)e{ getmouse(e, [e window]);}
- (void)rightMouseUp:(NSEvent*)e{ getmouse(e, [e window]);}
- (void)scrollWheel:(NSEvent*)e{ getmouse(e, [e window]);}

- (void)keyDown:(NSEvent*)e{ getkeyboard(e);}
- (void)flagsChanged:(NSEvent*)e{ getkeyboard(e);}

- (void)swipeWithEvent:(NSEvent*)e{ getgesture(e);}
- (void)magnifyWithEvent:(NSEvent*)e{ getgesture(e);}

- (void)touchesBeganWithEvent:(NSEvent*)e
{
	gettouch(e, NSTouchPhaseBegan);
}
- (void)touchesMovedWithEvent:(NSEvent*)e
{
	gettouch(e, NSTouchPhaseMoved);
}
- (void)touchesEndedWithEvent:(NSEvent*)e
{
	gettouch(e, NSTouchPhaseEnded);
}
- (void)touchesCancelledWithEvent:(NSEvent*)e
{
	gettouch(e, NSTouchPhaseCancelled);
}
@end

static int keycvt[] =
{
	[QZ_IBOOK_ENTER] '\n',
	[QZ_RETURN] '\n',
	[QZ_ESCAPE] 27,
	[QZ_BACKSPACE] '\b',
	[QZ_LALT] Kalt,
	[QZ_LCTRL] Kctl,
	[QZ_LSHIFT] Kshift,
	[QZ_F1] KF+1,
	[QZ_F2] KF+2,
	[QZ_F3] KF+3,
	[QZ_F4] KF+4,
	[QZ_F5] KF+5,
	[QZ_F6] KF+6,
	[QZ_F7] KF+7,
	[QZ_F8] KF+8,
	[QZ_F9] KF+9,
	[QZ_F10] KF+10,
	[QZ_F11] KF+11,
	[QZ_F12] KF+12,
	[QZ_INSERT] Kins,
	[QZ_DELETE] 0x7F,
	[QZ_HOME] Khome,
	[QZ_END] Kend,
	[QZ_KP_PLUS] '+',
	[QZ_KP_MINUS] '-',
	[QZ_TAB] '\t',
	[QZ_PAGEUP] Kpgup,
	[QZ_PAGEDOWN] Kpgdown,
	[QZ_UP] Kup,
	[QZ_DOWN] Kdown,
	[QZ_LEFT] Kleft,
	[QZ_RIGHT] Kright,
	[QZ_KP_MULTIPLY] '*',
	[QZ_KP_DIVIDE] '/',
	[QZ_KP_ENTER] '\n',
	[QZ_KP_PERIOD] '.',
	[QZ_KP0] '0',
	[QZ_KP1] '1',
	[QZ_KP2] '2',
	[QZ_KP3] '3',
	[QZ_KP4] '4',
	[QZ_KP5] '5',
	[QZ_KP6] '6',
	[QZ_KP7] '7',
	[QZ_KP8] '8',
	[QZ_KP9] '9',
};

static void
getkeyboard(NSEvent *e)
{
	NSString *s;
	char c;
	int k, m;
	uint code;

	m = [e modifierFlags];

	switch([e type]){
	case NSKeyDown:
		[[NSApp delegate] setKalting: 0];

		s = [e characters];
		c = [s UTF8String][0];

		if(m & NSCommandKeyMask){
			if(' '<=c && c<='~')
				keystroke(Kcmd+c);
			break;
		}
		k = c;
		code = [e keyCode];
		if(code<nelem(keycvt) && keycvt[code])
			k = keycvt[code];
		if(k==0)
			break;
		if(k>0)
			keystroke(k);
		else
			keystroke([s characterAtIndex:0]);
		break;

	case NSFlagsChanged:
		if([[NSApp delegate] mbuttons] || [[NSApp delegate] kbuttons]){
			[[NSApp delegate] setKbuttons: 0];
			if(m & NSAlternateKeyMask)
				[[NSApp delegate] orKbuttons: 2];
			if(m & NSCommandKeyMask)
				[[NSApp delegate] orKbuttons: 4];
			sendmouse();
		}else
		if(m & NSAlternateKeyMask){
			[[NSApp delegate] setKalting: 1];
			keystroke(Kalt);
		}
		break;

	default:
		panic("getkey: unexpected event type");
	}
}

/*
 * Devdraw does not use NSTrackingArea, that often
 * forgets to update the cursor on entering and on
 * leaving the area, and that sometimes stops sending
 * us MouseMove events, at least on OS X Lion.
 */
static void
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
getmouse(NSEvent *e, NSWindow* window)
{
	float d;
	int b, m;

	if([window isKeyWindow] == 0)
		return;

	getmousepos(window);

	switch([e type]){
	case NSLeftMouseDown:
	case NSLeftMouseUp:
	case NSOtherMouseDown:
	case NSOtherMouseUp:
	case NSRightMouseDown:
	case NSRightMouseUp:
		b = [NSEvent pressedMouseButtons];
		b = b&~6 | (b&4)>>1 | (b&2)<<1;
		b = mouseswap(b);

		if(b == 1){
			m = [e modifierFlags];
			if(m & NSAlternateKeyMask){
				b = 2;
				// Take the ALT away from the keyboard handler.
				if([[NSApp delegate] kalting]){
					[[NSApp delegate] setKalting: 0];
					keystroke(Kalt);
				}
			}else
			if(m & NSCommandKeyMask)
				b = 4;
		}
		[[NSApp delegate] setMbuttons: b];
		break;

	case NSScrollWheel:
#if OSX_VERSION >= 100700
		d = [e scrollingDeltaY];
#else
		d = [e deltaY];
#endif
		if(d>0)
			[[NSApp delegate] setMscroll: 8];
		else
		if(d<0)
			[[NSApp delegate] setMscroll: 16];
		break;

	case NSMouseMoved:
	case NSLeftMouseDragged:
	case NSRightMouseDragged:
	case NSOtherMouseDragged:
		break;

	default:
		panic("getmouse: unexpected event type");
	}
	sendmouse();
}

#define Minpinch	0.050

enum
{
	Left		= -1,
	Right	= +1,
	Up		= +2,
	Down	= -2,
};

static int
getdir(int dx, int dy)
{
	return dx + 2*dy;
}

static void interpretswipe(int);

static void
getgesture(NSEvent *e)
{
	static float sum;
	int dir;

	if(usegestures == 0)
		return;

	switch([e type]){

	case NSEventTypeMagnify:
		sum += [e magnification];
		if(fabs(sum) > Minpinch){
			sum = 0;
		}
		break;

	case NSEventTypeSwipe:
		dir = getdir(-[e deltaX], [e deltaY]);

		if([[NSApp delegate] touchevent])
			if(dir==Up || dir==Down)
				break;
		interpretswipe(dir);
		break;
	}
}

static void sendclick(int);
static void sendchord(int, int);
static void sendcmd(int);

static uint
msec(void)
{
	return nsec()/1000000;
}

#define Inch		72
#define Cm		Inch/2.54

#define Mindelta	0.0*Cm
#define Xminswipe	0.5*Cm
#define Yminswipe	0.1*Cm

enum
{
	Finger = 1,
	Msec = 1,

	Maxtap = 400*Msec,
	Maxtouch = 3*Finger,
};

static void
gettouch(NSEvent *e, int type)
{
	static NSPoint delta;
	static NSTouch *toucha[Maxtouch];
	static NSTouch *touchb[Maxtouch];
	static int done, ntouch, odir, tapping;
	static uint taptime;
	NSArray *a;
	NSPoint d;
	NSSet *set;
	NSSize s;
	int dir, i, p;

	if(usegestures == 0)
		return;

	switch(type){

	case NSTouchPhaseBegan:
		[[NSApp delegate] setTouchevent: 1];
		p = NSTouchPhaseTouching;
		set = [e touchesMatchingPhase:p inView:nil];
		if(set.count == 3){
			tapping = 1;
			taptime = msec();
		}else
		if(set.count > 3)
			tapping = 0;
		return;

	case NSTouchPhaseMoved:
		p = NSTouchPhaseMoved;
		set = [e touchesMatchingPhase:p inView:nil];
		a = [set allObjects];
		if(set.count > Maxtouch)
			return;
		if(ntouch==0){
			ntouch = set.count;
			for(i=0; i<ntouch; i++){
//				assert(toucha[i] == nil);
				toucha[i] = [[a objectAtIndex:i] retain];
			}
			return;
		}
		if(ntouch != set.count)
			break;
		if(done)
			return;

		d = NSMakePoint(0,0);
		for(i=0; i<ntouch; i++){
//			assert(touchb[i] == nil);
			touchb[i] = [a objectAtIndex:i];
			d.x += touchb[i].normalizedPosition.x;
			d.y += touchb[i].normalizedPosition.y;
			d.x -= toucha[i].normalizedPosition.x;
			d.y -= toucha[i].normalizedPosition.y;
		}
		s = toucha[0].deviceSize;
		d.x = d.x/ntouch * s.width;
		d.y = d.y/ntouch * s.height;
		if(fabs(d.x)>Mindelta || fabs(d.y)>Mindelta){
			tapping = 0;
			if(ntouch != 3){
				done = 1;
				goto Return;
			}
			delta = NSMakePoint(delta.x+d.x, delta.y+d.y);
			d = NSMakePoint(fabs(delta.x), fabs(delta.y));
			if(d.x>Xminswipe || d.y>Yminswipe){
				if(d.x > d.y)
					dir = delta.x>0? Right : Left;
				else
					dir = delta.y>0? Up : Down;
				if(dir != odir){
//					if(ntouch == 3)
						if(dir==Up || dir==Down)
							interpretswipe(dir);
					odir = dir;
				}
				goto Return;
			}
			for(i=0; i<ntouch; i++){
				[toucha[i] release];
				toucha[i] = [touchb[i] retain];
			}
		}
Return:
		for(i=0; i<ntouch; i++)
			touchb[i] = nil;
		return;

	case NSTouchPhaseEnded:
		p = NSTouchPhaseTouching;
		set = [e touchesMatchingPhase:p inView:nil];
		if(set.count == 0){
			if(tapping && msec()-taptime<Maxtap)
				sendclick(2);
			odir = 0;
			tapping = 0;
			[[NSApp delegate] setUndoflag: 0];
			[[NSApp delegate] setTouchevent: 0];
		}
		break;

	case NSTouchPhaseCancelled:
		break;

	default:
		panic("gettouch: unexpected event type");
	}
	for(i=0; i<ntouch; i++){
		[toucha[i] release];
		toucha[i] = nil;
	}
	delta = NSMakePoint(0,0);
	done = 0;
	ntouch = 0;
}

static void
interpretswipe(int dir)
{
	if(dir == Left)
		sendcmd('x');
	else
	if(dir == Right)
		sendcmd('v');
	else
	if(dir == Up)
		sendcmd('c');
	else
	if(dir == Down)
		sendchord(2,1);
}

static void
sendcmd(int c)
{
	if([[NSApp delegate] touchevent] && (c=='x' || c=='v')){
		if([[NSApp delegate] undoflag])
			c = 'z';
		[[NSApp delegate] setUndoflag: ! [[NSApp delegate] undoflag]];
	}
	keystroke(Kcmd+c);
}

static void
sendclick(int b)
{
	[[NSApp delegate] setMbuttons: b];
	sendmouse();
	[[NSApp delegate] setMbuttons: 0];
	sendmouse();
}

static void
sendchord(int b1, int b2)
{
	[[NSApp delegate] setMbuttons: b1];
	sendmouse();
	[[NSApp delegate] setMbuttons: [[NSApp delegate] mbuttons] | b2];
	sendmouse();
	[[NSApp delegate] setMbuttons: 0];
	sendmouse();
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

static void
followzoombutton(NSRect r, NSWindow* window)
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
hidebars(int set, NSWindow* window)
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

// Construct a menu dynamically.
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
	updatecursor(nsc);
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
