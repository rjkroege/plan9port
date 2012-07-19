
enum {
    WebFindOptionsCaseInsensitive = 1 << 0,
    WebFindOptionsAtWordStarts = 1 << 1,
    WebFindOptionsTreatMedialCapitalAsWordStart = 1 << 2,
    WebFindOptionsBackwards = 1 << 3,
    WebFindOptionsWrapAround = 1 << 4,
    WebFindOptionsStartInSelection = 1 << 5
};
typedef NSUInteger WebFindOptions;


/*
 * From WebKit source. Note that some of these might not work.
 */
@interface WebView (WebPendingPublic)

- (void)scheduleInRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode;
- (void)unscheduleFromRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode;

- (BOOL)findString:(NSString *)string options:(WebFindOptions)options;
- (DOMRange *)DOMRangeOfString:(NSString *)string relativeTo:(DOMRange *)previousRange options:(WebFindOptions)options;

- (void)setMainFrameDocumentReady:(BOOL)mainFrameDocumentReady;

- (void)setTabKeyCyclesThroughElements:(BOOL)cyclesElements;
- (BOOL)tabKeyCyclesThroughElements;

- (void)scrollDOMRangeToVisible:(DOMRange *)range;

/*!
@method setScriptDebugDelegate:
@abstract Set the WebView's WebScriptDebugDelegate delegate.
@param delegate The WebScriptDebugDelegate to set as the delegate.
*/
- (void)setScriptDebugDelegate:(id)delegate;

/*!
@method scriptDebugDelegate
@abstract Return the WebView's WebScriptDebugDelegate.
@result The WebView's WebScriptDebugDelegate.
*/
- (id)scriptDebugDelegate;

/*!
    @method setHistoryDelegate:
    @abstract Set the WebView's WebHistoryDelegate delegate.
    @param delegate The WebHistoryDelegate to set as the delegate.
*/
- (void)setHistoryDelegate:(id)delegate;

/*!
    @method historyDelegate
    @abstract Return the WebView's WebHistoryDelegate delegate.
    @result The WebView's WebHistoryDelegate delegate.
*/
- (id)historyDelegate;

- (BOOL)shouldClose;

/*!
    @method aeDescByEvaluatingJavaScriptFromString:
    @param script The text of the JavaScript.
    @result The result of the script, converted to an NSAppleEventDescriptor, or nil for failure.
*/
- (NSAppleEventDescriptor *)aeDescByEvaluatingJavaScriptFromString:(NSString *)script;

// Support for displaying multiple text matches.
// These methods might end up moving into a protocol, so different document types can specify
// whether or not they implement the protocol. For now we'll just deal with HTML.
// These methods are still in flux; don't rely on them yet.
- (BOOL)canMarkAllTextMatches;
- (NSUInteger)countMatchesForText:(NSString *)string options:(WebFindOptions)options highlight:(BOOL)highlight limit:(NSUInteger)limit markMatches:(BOOL)markMatches;
- (NSUInteger)countMatchesForText:(NSString *)string inDOMRange:(DOMRange *)range options:(WebFindOptions)options highlight:(BOOL)highlight limit:(NSUInteger)limit markMatches:(BOOL)markMatches;
- (void)unmarkAllTextMatches;
- (NSArray *)rectsForTextMatches;

// Support for disabling registration with the undo manager. This is equivalent to the methods with the same names on NSTextView.
- (BOOL)allowsUndo;
- (void)setAllowsUndo:(BOOL)flag;

/*!
    @method setPageSizeMultiplier:
    @abstract Change the zoom factor of the page in views managed by this webView.
    @param multiplier A fractional percentage value, 1.0 is 100%.
*/
- (void)setPageSizeMultiplier:(float)multiplier;

/*!
    @method pageSizeMultiplier
    @result The page size multipler.
*/
- (float)pageSizeMultiplier;

// Commands for doing page zoom.  Will end up in WebView (WebIBActions) <NSUserInterfaceValidations>
- (BOOL)canZoomPageIn;
- (IBAction)zoomPageIn:(id)sender;
- (BOOL)canZoomPageOut;
- (IBAction)zoomPageOut:(id)sender;
- (BOOL)canResetPageZoom;
- (IBAction)resetPageZoom:(id)sender;

// Sets a master volume control for all media elements in the WebView. Valid values are 0..1.
- (void)setMediaVolume:(float)volume;
- (float)mediaVolume;

// Add visited links
- (void)addVisitedLinks:(NSArray *)visitedLinks;

@end

