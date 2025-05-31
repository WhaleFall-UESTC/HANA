#ifndef __CONTEXT_H__
#define __CONTEXT_H__

struct context {
	uint64 ra;
	uint64 sp;
	uint64 s[10];
	uint64 prmd;
	uint64 era;
};

#endif // __CONTEXT_H__