#ifndef NET_ASSERT__H
#define NET_ASSERT__H

////////////////////////////////////////////////////////////////////////////////

// set as 1 and rebuild for break-on-fail network debugging
#define ENABLE_NET_ASSERT 0

#if ENABLE_NET_ASSERT
# include <cassert>
# define netassert(expr) assert(expr)
#else
# define netassert(expr) do {} while (0)
#endif

////////////////////////////////////////////////////////////////////////////////

#endif // NET_ASSERT__H
