#ifndef NET_COMMON_H
#define NET_COMMON_H

#if defined(__arm__) || defined(__aarch64__) || defined(__i386__) || defined(__x86_64__)
#define HAVE_BSD_SOCKET 1
#else
#define HAVE_BSD_SOCKET 0
#endif

#if defined(__riscv) // 
#define HAVE_LWIP_SOCKET 1
#else
#define HAVE_LWIP_SOCKET 0
#endif

#if HAVE_BSD_SOCKET
#include <sys/types.h>  // for AF_INET SOCK_STREAM
#include <sys/socket.h> // for socket
#include <netinet/in.h> // for sockaddr_in
#include <arpa/inet.h> // for inet_pton
#elif HAVE_LWIP_SOCKET
#include "lwip/sockets.h"
#ifndef close
#define close(fd) lwip_close(fd)
#endif
#else
#error "Unknow platform!"
#endif

#endif  // NET_COMMON_H