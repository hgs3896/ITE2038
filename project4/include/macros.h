#ifndef __MACROS_H__
#define __MACROS_H__

//#define COPY(dest, src) (memcpy((dest), (src), PAGESIZE))
#define IS_VALID_TID(tid) \
	(\
		(tid) >= 1 && \
		(tid) <= DEFAULT_SIZE_OF_TABLES \
	)
	
#endif