#ifndef graphedge_h
#define graphedge_h

typedef enum {
	EGS_START = 0,
	EGS_Ld = 1,
	EGS_LdMd = 2,
	EGS_LdRd = 3,
	EGS_LdMdLu = 4,
	EGS_LdRdLu = 5,
	EGS_Md = 6,
	EGS_MdLd = 7,
	EGS_MdLdu = 8,
	EGS_MdRd = 9,
	EGS_MdRdMu = 10,
	EGS_MdLdMu = 11,
	EGS_Rd = 12,
	EGS_LdMdMu = 13,
	EGS_LdRdRu = 14,
	EGS_MdLdLu = 15,
	EGS_MdRdRu = 16,
	EGS_RdLd = 17,
	EGS_RdLdLu = 18,
	EGS_RdLdRu  = 19,
	EGS_RdMd = 20,
	EGS_RdMdMu = 21,
	EGS_RdMdRu = 22
} GraphStates;


typedef enum {
	EM_SWALLOW, 
	EM_LITERAL_FORWARD, 
	EM_LEFTIED_FORWARD,
	EM_LEFTUP_SYNTHETIC
} EventMode;

@interface GraphEdge : NSObject {
	NSString *m_prehandler;
	NSString *m_posthandler;
	GraphStates m_nextstate;
	EventMode m_eventmode;
}

/**
  * Creates a new Edge object
 */

// Need a bunch of constructors of various kinds.
// + (EventGraphEdge)edgeWithNextState:(GraphStates)s handledBy:(NSString)h forwardLiteral: (BOOL)fl forwardSynthetic:(BOOL)fs;

- (id) init;
- (id) initNextState: (GraphStates)s eventMode: (EventMode)m;
- (id) initNextState: (GraphStates)s eventMode: (EventMode)m preHandler: (NSString*)preh;
- (id) initNextState: (GraphStates)s eventMode: (EventMode)m postHandler: (NSString*)poh;
- (id) initNextState: (GraphStates)s eventMode: (EventMode)m preHandler: (NSString*)preh postHandler: (NSString*)poh;

// FIXME: use function pointers.
- (NSString *)preHandler;
- (NSString *)postHandler;
- (BOOL)hasPreHandler;
- (BOOL)hasPostHandler;
- (NSEvent*)eventForEdge:(NSEvent*)event;
- (GraphStates)nextState;


@end

#endif
