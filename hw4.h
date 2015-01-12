#ifndef _HW4_H_
#define _HW4_H_

#include <linux/ioctl.h>

#define HW4_MAGIC '4'
#define HW4_SET_KEY _IOW(HW4_MAGIC,0,int)

#endif
