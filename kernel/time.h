#include "types.h"

typedef struct {
	uint64 sec;  // 自 Unix 纪元起的秒数
	uint64 usec;	// 微秒，也就是除了秒的那点零头
}TimeVal;