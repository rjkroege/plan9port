#ifndef chordingwindow_h
#define chordingwindow_h

@interface ChordingWindow : NSWindow {
	GraphStates m_state;
	NSDictionary* m_transitions;
	id <EventAugmentation, NSObject, NSObject> m_eventdelegate;
}

- (void)sendEvent:(NSEvent *)event;
- (void)dealloc;

- (id <EventAugmentation, NSObject>) eventDelegate;
- (void) setEventDelegate: (id <EventAugmentation, NSObject>)d;

// - (void)setWebView:(WebView*)wv;


@end

#endif
