#ifndef NetCoreExport_h__
#define NetCoreExport_h__

////////////////////////////////////////////////////////////////////////////////

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined( __BCPLUSPLUS__)  || defined( __MWERKS__)
#   ifdef NETCORE
#      define NETCORE_EXPORT __declspec(dllexport)
#   else
#      define NETCORE_EXPORT __declspec(dllimport)
#   endif
#else
#   ifdef NETCORE
#      define NETCORE_EXPORT __attribute__ ((visibility("default")))
#   else
#      define NETCORE_EXPORT
#   endif
#endif

////////////////////////////////////////////////////////////////////////////////

#endif // NetCoreExport_h__
