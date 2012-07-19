#ifndef chordingwindow_h
#define chordingwindow_h

@interface ChordingWindow : NSWindow {
	GraphStates m_state;
	NSDictionary* m_transitions;
	// WebView* webview;
}

- (void)sendEvent:(NSEvent *)event;
- (void)dealloc;

// - (void)setWebView:(WebView*)wv;


@end

#endif
