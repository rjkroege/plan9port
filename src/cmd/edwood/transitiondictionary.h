
#ifndef transitiondictionary_h
#define transitiondictonary_h
@interface NSMutableDictionary (TransitionDictionary)

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns eventMode:(EventMode)mo preHandler:(NSString*)preh postHandler: (NSString*)posth;

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns eventMode:(EventMode)mo;

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns;

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns eventMode:(EventMode)mo preHandler:(NSString*)preh;

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns eventMode:(EventMode)mo postHandler: (NSString*)posth;

@end

#endif
