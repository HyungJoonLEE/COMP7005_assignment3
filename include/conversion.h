#ifndef PROJECT_CONVERSION_H
#define PROJECT_CONVERSION_H

#include <netinet/in.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>


in_port_t parse_port(const char *buff, int radix);


#endif //PROJECT_CONVERSION_H
