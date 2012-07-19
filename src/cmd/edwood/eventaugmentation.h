#ifndef eventaugmentation_h
#define eventaugmentation_h


@protocol EventAugmentation

/*
 * Method calls that are fired as part of the
 * chording state machine on the chording
 * delegate. pre* are dispatched before sending
 * the event to WebKit. post* are dispatched after
 * the event may have been sent to WebKit.
 */
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
