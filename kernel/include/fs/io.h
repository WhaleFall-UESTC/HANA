#ifndef __IO_H__
#define __IO_H__

#include <common.h>

struct iovec {
	void *iov_base;
	size_t iov_len;
};

#endif // __IO_H__