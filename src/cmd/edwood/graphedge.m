
#import <Cocoa/Cocoa.h>
#import "graphedge.h"


@implementation GraphEdge

- (SEL)preHandler {
	return m_prehandler;
}

- (SEL)postHandler {
	return m_posthandler;
}

- (BOOL)hasPreHandler {
	return m_prehandler != nil;
}

- (BOOL)hasPostHandler {
	return m_posthandler != nil;
}


/**
 * Creates a fake event for re-distribution to the underlying WindowFrame
 */
inline static NSEvent *makeFakeEvent(NSEventType type, NSEvent *event) {
	NSEvent *tweakedEvent;
	tweakedEvent = [NSEvent	mouseEventWithType:type 
																		location:[event locationInWindow] 
															 modifierFlags:[event modifierFlags] 
																	 timestamp:[event timestamp]
																windowNumber:[event windowNumber] 
																		 context:[event context]	
																 eventNumber:[event eventNumber] 
																	clickCount:[event clickCount] 
																		pressure:[event pressure]];
	return tweakedEvent;
}


- (NSEvent*)eventForEdge:(NSEvent*)event {
	NSEvent *ne;
	switch (m_eventmode) {
		case EM_SWALLOW:
			ne = nil;
			break;
		case EM_LITERAL_FORWARD:
			ne = event;
			break;
		case EM_LEFTIED_FORWARD:
			switch ([event type]) {
				case NSOtherMouseUp: case NSRightMouseUp:
					ne = makeFakeEvent(NSLeftMouseUp, event);
					break;
				case NSOtherMouseDown: case NSRightMouseDown:
					ne = makeFakeEvent(NSLeftMouseDown, event);
					break;
				case NSOtherMouseDragged: case NSRightMouseDragged:
					ne = makeFakeEvent(NSLeftMouseDragged, event);
					break;
				default:
					// Should never happen.
					@throw @"Should not reach in EM_LEFTIED_FORWARD";
			}
			break;
		case EM_LEFTUP_SYNTHETIC:
			ne = makeFakeEvent(NSLeftMouseUp, event);
			break;
	}
	return ne;
}

- (GraphStates)nextState {
	return m_nextstate;
}

- (id)init {
	if ((self = [super init])) {
		m_eventmode = EM_SWALLOW;
		m_nextstate = EGS_START;
		m_prehandler = nil;
		m_posthandler =  nil;
	}
	return self;
}

- (id)initNextState:(GraphStates)s eventMode:(EventMode)m {
	if ((self = [super init])) {
		m_eventmode = m;
		m_nextstate = s;
		m_prehandler = nil;
		m_posthandler =  nil;
	}
	return self;
}

- (id)initNextState:(GraphStates)s eventMode:(EventMode)m preHandler:(SEL)preh {
	if ((self = [super init])) {
		m_eventmode = m;
		m_nextstate = s;
		m_prehandler = preh;
		m_posthandler =  nil;
	}
	return self;
}

- (id)initNextState:(GraphStates)s eventMode:(EventMode)m postHandler:(SEL)poh {
	if ((self = [super init])) {
		m_eventmode = m;
		m_nextstate = s;
		m_prehandler = nil;
		m_posthandler =  poh;
	}
	return self;
}

- (id)initNextState:(GraphStates)s eventMode:(EventMode)m preHandler:(SEL)preh postHandler:(SEL)poh {
	if ((self = [super init])) {
		m_eventmode = m;
		m_nextstate = s;
		m_prehandler = preh;
		m_posthandler =  poh;
	}
	return self;
}

- (void)dealloc {
	NSLog(@"dealloc on EventGraphEdge got called\n");
	[super dealloc];
}


@end
