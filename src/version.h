#include <zephyr/toolchain.h>
#ifndef VERSION_H
#define VERSION_H

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0

#define TOSTRING(x)   STRINGIFY(x)

#define VERSION                 \
	TOSTRING(VERSION_MAJOR) \
	"." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH)

#endif
