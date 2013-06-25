#ifndef __UNTILS__H__
#define __UNTILS__H__

/**
 * check given string whether a valid host address
 *
 * @return 0  not a valid host address
 *         1  yes
 */
unsigned int check_host(const char* host);

/**
 * check the given string whether a valid ip address
 *
 * @return 0 not a valid ip address
 *         1 yes
 */
unsigned int check_ip(const char*ip);

#endif
