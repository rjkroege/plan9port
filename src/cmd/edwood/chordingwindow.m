
#import <Cocoa/Cocoa.h>
#import "graphedge.h"
#import "chordingwindow.h"
#import "transitiondictionary.h"

/**
 * Wraps the NSWindow class to intercept events.  The general idea is that
 * WebKit is not capable of implementing the desired mouse handling behaviour
 * for drag and chord events. Instead, we intercept all events for a given window in
 * this class and reproces them.
 * 
 * Event processing is based on an finite state machine: each state corresponds to
 * the mouse buttons currently depressed.  The graph is represented by a hash table
 * of edges keyed by the two-tuple of (state, input event) and each edge has properties
 * that specify how the event should be handled, the result state and any handlers
 * that we may wish to execute.
 */
@implementation ChordingWindow

/**
 * Does the default event handling for events that modify positions in the 
 * event FSM.
 */
- (void)mouseEventHandler:(NSEvent *)event {
	NSEventType nset = [event type];
	GraphEdge *edge = [m_transitions objectForKey:[NSNumber numberWithInt:(m_state << 8) | nset]];
	NSString *status = @"inactive";
	
	if (edge != nil) {			// A non-existent edge is ignored.
		if ([edge hasPreHandler]) {
			NSLog(@"should fire handler %@", [edge preHandler]);
			// status = [webview stringByEvaluatingJavaScriptFromString:[edge preHandler]];
			if (![status isEqualToString:@"ok"]) {
				NSLog(@"Terminating -- preHandler: <%@> returned invalid status: <%@>", [edge preHandler], status);
				// @throw @"invalid postHandler status";
			}
		}

		event = [edge eventForEdge: event];
		if (event != nil) {
			[super sendEvent: event];
		}

		if ([edge hasPostHandler]) {
			NSLog(@"should fire handler %@", [edge postHandler]);		
			// status = [webview stringByEvaluatingJavaScriptFromString:[edge postHandler]];
			if (![status isEqualToString:@"ok"]) {
				NSLog(@"Terminating -- postHandler: <%@> returned invalid status: <%@>", [edge postHandler], status);
				// @throw @"invalid postHandler status";
			}
		}
		m_state = [edge nextState];
	} else {
		NSLog(@"swallowing event without edge: from %d via %d", m_state, nset);
	}
}


/**
  * Processes each event that are sent to this window. Because we are only interested in mouse
 * events, this capture point will swallow all of the events of interest.
 */
- (void)sendEvent:(NSEvent *)event {
	NSEventType nset = [event type];
	
	switch (nset) {
		case NSOtherMouseDown:
		case NSOtherMouseUp:
		case NSLeftMouseUp:
		case NSLeftMouseDown:
		case NSRightMouseDown:
		case NSRightMouseUp:
		case NSOtherMouseDragged:			
		case NSLeftMouseDragged:
		case NSRightMouseDragged:
			// NSLog(@"processing a mouse event with mouseEventHandler\n");
			[self mouseEventHandler: event];
			break;
		default:
			[super sendEvent:event]; 
			break;
	}
}	

// Need a constructor...
// Which one do we actually end up calling? 
// Or do I have to create all of them?
- (id)init {
	if ((self = [super init])) {
		NSLog(@"called the window constructor in init\n");		
	}
	return self;
}


#define EK(_s, _e) [NSNumber numberWithUnsignedInt: ((_s << 8)|_e)]
#define MK [GraphEdge alloc]

/**
 * Common code to construct a transition dictonary: one instance per
 * edge.  Each edge can have two JavaScript handlers associated with it: 
 * the preHandler is executed before the event is dispatched to WebKit, and
 * the postHandler is executed after the event is displatched to WebKit.  Entry
 * points follow a consistent naming scheme: '_' followed by pre|post as
 * appropriate followed by the sequence of events needed to reach this state.
 * 
 * A valid handler should return the string 'ok' to indicate that it exists.  This
 * permits the event dispatch code above to log that we have attempted to
 * exected a handler that doesn't exist.
 */
static NSDictionary* makeTransitionDictionary() {
	NSMutableDictionary *td = [[NSMutableDictionary alloc] init];

	// Basic left mouse.
	[td state:EGS_START	event:NSLeftMouseDown			newstate:EGS_Ld				eventMode:EM_LITERAL_FORWARD
			preHandler:@"_preLd();"];
	[td state:EGS_Ld			event:NSLeftMouseUp				newstate:EGS_START		eventMode:EM_LITERAL_FORWARD];
	[td state:EGS_Ld			event:NSLeftMouseDragged		newstate:EGS_Ld				eventMode:EM_LITERAL_FORWARD];

	// Basic middle mouse.
	[td state:EGS_START	event:NSOtherMouseDown		newstate:EGS_Md				eventMode:EM_LEFTIED_FORWARD
			preHandler:@"_preMd();"];
	[td state:EGS_Md			event:NSOtherMouseDragged	newstate:EGS_Md				eventMode:EM_LEFTIED_FORWARD];
	[td state:EGS_Md			event:NSOtherMouseUp				newstate:EGS_START		eventMode:EM_LEFTIED_FORWARD
			postHandler:@"_postMu();"];

	// Basic right mouse.
	[td state:EGS_START	event:NSRightMouseDown			newstate:EGS_Rd				eventMode:EM_LEFTIED_FORWARD
			preHandler:@"_preRd();"];
	[td state:EGS_Rd			event:NSRightMouseDragged	newstate:EGS_Rd				eventMode:EM_LEFTIED_FORWARD];
	[td state:EGS_Rd			event:NSRightMouseUp				newstate:EGS_START		eventMode:EM_LEFTIED_FORWARD
			postHandler:@"_postRu();"];
	
	// Left-middle chords
	[td state:EGS_Ld			event:NSOtherMouseDown		newstate:EGS_LdMd			eventMode:EM_LEFTUP_SYNTHETIC
			postHandler:@"_postLdMd();"];

	[td state:EGS_LdMd		event:NSOtherMouseUp				newstate:EGS_LdMdMu];
	[td state:EGS_LdMd		event:NSLeftMouseUp				newstate:EGS_LdMdLu];
	[td state:EGS_LdMdMu	event:NSLeftMouseUp				newstate:EGS_START];
	[td state:EGS_LdMdMu	event:NSOtherMouseDown		newstate:EGS_LdMd];
	[td state:EGS_LdMdLu	event:NSOtherMouseUp				newstate:EGS_START];
	
	[td state:EGS_LdMdMu	event:NSRightMouseDown			newstate:EGS_LdRd			eventMode:EM_SWALLOW
			postHandler:@"_postLdRd();"];
	
	// Left-right chords
	[td state:EGS_Ld			event:NSRightMouseDown			newstate:EGS_LdRd			eventMode:EM_LEFTUP_SYNTHETIC
			postHandler:@"_postLdRd();"];
	
	[td state:EGS_LdRd		event:NSRightMouseUp				newstate:EGS_LdRdRu];
	[td state:EGS_LdRd		event:NSLeftMouseUp				newstate:EGS_LdMdLu];
	[td state:EGS_LdRdRu	event:NSLeftMouseUp				newstate:EGS_START];
	[td state:EGS_LdRdRu	event:NSRightMouseDown			newstate:EGS_LdRd];
	[td state:EGS_LdRdLu	event:NSOtherMouseUp				newstate:EGS_START];
	
	[td state:EGS_LdRdRu	event:NSOtherMouseDown		newstate:EGS_LdMd			eventMode:EM_SWALLOW
			postHandler:@"_postLdMd();"];
	
	// Middle-left chords (execute with context-selection)
	[td state:EGS_Md			event:NSLeftMouseDown			newstate:EGS_MdLd			eventMode:EM_LEFTUP_SYNTHETIC
			postHandler:@"_postMdLd();"];
	
	[td state:EGS_MdLd		event:NSOtherMouseUp				newstate:EGS_MdLdMu];
	[td state:EGS_MdLd		event:NSLeftMouseUp				newstate:EGS_MdLdLu];
	[td state:EGS_MdLdMu	event:NSLeftMouseUp				newstate:EGS_START];
	[td state:EGS_MdLdLu	event:NSOtherMouseUp				newstate:EGS_START];
	
	// Middle-right chords (execute/cancel)
	[td state:EGS_Md			event:NSRightMouseDown			newstate:EGS_MdRd			eventMode:EM_LEFTUP_SYNTHETIC
			postHandler:@"_postMdRd();"];
	
	[td state:EGS_MdRd		event:NSOtherMouseUp				newstate:EGS_MdRdMu];
	[td state:EGS_MdRd		event:NSRightMouseUp				newstate:EGS_MdRdRu];
	[td state:EGS_MdRdMu	event:NSRightMouseUp				newstate:EGS_START];
	[td state:EGS_MdRdRu	event:NSOtherMouseUp				newstate:EGS_START];
	
	// Right-middle (search cancel)
	[td state:EGS_Rd			event:NSOtherMouseDown		newstate:EGS_RdMd			eventMode:EM_LEFTUP_SYNTHETIC
			postHandler:@"_postRdMd();"];
	
	[td state:EGS_RdMd		event:NSOtherMouseUp				newstate:EGS_RdMdMu];
	[td state:EGS_RdMd		event:NSRightMouseUp				newstate:EGS_RdMdRu];
	[td state:EGS_RdMdMu	event:NSRightMouseUp				newstate:EGS_START];
	[td state:EGS_RdMdRu	event:NSOtherMouseUp				newstate:EGS_START];

	// Right-left (panning mode someday?)
	[td state:EGS_Rd			event:NSLeftMouseDown			newstate:EGS_RdLd			eventMode:EM_LEFTUP_SYNTHETIC
			postHandler:@"_postRdLd();"];
	
	[td state:EGS_RdLd		event:NSRightMouseUp				newstate:EGS_RdLdRu];
	[td state:EGS_RdLd		event:NSLeftMouseUp				newstate:EGS_RdLdLu];
	[td state:EGS_RdLdRu	event:NSLeftMouseUp				newstate:EGS_START];
	[td state:EGS_RdLdLu	event:NSRightMouseUp				newstate:EGS_START];
	
	return [[NSDictionary alloc] initWithDictionary:td];
}


/**
  * Initializer for the window object.  This one is actually invoked when we are created from
  * the NIB file.
 */
- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int)windowStyle 
									backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation {
	if ((self = [super initWithContentRect:contentRect	styleMask:windowStyle 
																 backing:bufferingType defer:deferCreation])) {
		NSLog(@"called the window constructor in initWithContentRect\n");
		m_state = EGS_START;
		m_transitions = makeTransitionDictionary();
	}
	return self;
}

/**
  * Setup the window object.
  */
- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int	)windowStyle 
									backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation screen:(NSScreen *)screen {
	if ((self = [super initWithContentRect:contentRect styleMask:windowStyle 
																 backing:bufferingType defer:deferCreation screen:screen])) {
		NSLog(@"called the window constructor in longer initWithContentRect\n");
		m_state = EGS_START;
		m_transitions = makeTransitionDictionary();
	}
	return self;
}

- (void)dealloc {
	// Call free here on the object in question...
	NSLog(@"dealloc got called in suprawindow...\n");
	[m_transitions release];
//	[webview release];
	[super dealloc];
}

#if 0
- (void)setWebView:(WebView*)wv {
	webview = wv;
	[webview retain];
}
#endif



@end
