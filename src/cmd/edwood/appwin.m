/*
 * Cocoa's event loop must be in main thread.
 */

// Order of this matters so do it always here.
#import <Cocoa/Cocoa.h>

#import "appwin.h"

@implementation AppWin
- (NSTimeInterval)animationResizeTime:(NSRect)r
{
	return 0;
}

- (BOOL)canBecomeKeyWindow
{
	return YES;	/* else no keyboard for old fullscreen */
}
@end
