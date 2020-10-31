#ifndef __MACROS_H__
#define __MACROS_H__

inline bool IS_VALID_TID(int tid){
	return tid >= 1 && tid <= DEFAULT_SIZE_OF_TABLES;
}
	
#endif