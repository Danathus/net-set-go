#ifndef NETPLATFORM_H
#define NETPLATFORM_H

////////////////////////////////////////////////////////////////////////////////
// platform detection

#define NET_PLATFORM_WINDOWS  1
#define NET_PLATFORM_MAC      2
#define NET_PLATFORM_UNIX     3

#if defined(_WIN32)
#   define NET_PLATFORM NET_PLATFORM_WINDOWS
#elif defined(__APPLE__)
#   define NET_PLATFORM NET_PLATFORM_MAC
#else
#   define NET_PLATFORM NET_PLATFORM_UNIX
#endif

#ifndef NET_PLATFORM
#   error unknown platform!
#endif

////////////////////////////////////////////////////////////////////////////////

#endif // NETPLATFORM_H
