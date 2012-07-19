
#import <Cocoa/Cocoa.h>
#import "graphedge.h"
#import "transitiondictionary.h"


@implementation NSMutableDictionary (TransitionDictionary)

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns eventMode:(EventMode)mo preHandler:(NSString*)preh postHandler: (NSString*)posth {
	
	NSNumber *n = [NSNumber numberWithUnsignedInt: (s << 8) | ev];
	GraphEdge *ege = [GraphEdge alloc];
	[ege initNextState:ns eventMode:mo preHandler:preh postHandler:posth];
	[self setObject:ege forKey:n];
}

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns eventMode:(EventMode)mo {
	[self state:s event:ev newstate:ns eventMode:mo preHandler:nil postHandler:nil];
}

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns eventMode:(EventMode)mo preHandler:(NSString*)preh {
	[self state:s event:ev newstate:ns eventMode:mo preHandler:preh postHandler:nil];	
}

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns eventMode:(EventMode)mo postHandler: (NSString*)posth {
	[self state:s event:ev newstate:ns eventMode:mo preHandler:nil postHandler:posth];
}

- (void) state:(GraphStates)s event:(NSEventType)ev newstate:(GraphStates)ns {
	[self state:s event:ev newstate:ns eventMode:EM_SWALLOW];
}



@end
