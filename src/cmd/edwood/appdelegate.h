
#ifndef appdelegate_h
#define appdelegate_h

/*
 * Interface file is pure cocoa.
*/
@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, EventAugmentation> {
	int needflush;
	WebView* contentview;
	NSCursor* cursor;
	NSSelectionAffinity m_selectionaffinity;
	DOMRange* m_leftselection;
}

- (id) init;

@property int needflush;
@property (retain) NSCursor* cursor;
@property (retain) WebView* contentview;
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

- (NSString*)stringValue;

/* From protocol EventAugmentation */
- (void) postLdMd;
- (void) postLdRd;
- (void) postMdLd;
- (void) postMdRd;
- (void) postMu;
- (void) postRdLd;
- (void) postRdMd;
- (void) postRu;
- (void) preLd;
- (void) preMd;
- (void) preRd;


@end

#endif
