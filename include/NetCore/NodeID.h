#ifndef NODE_ID__H
#define NODE_ID__H

namespace net {

////////////////////////////////////////////////////////////////////////////////

/**
 * Note: 0 is by convention the server, ascending integers from there for
 * successive clients.
 */
enum ReservedNodeIDs
{
   NODEID_INVALID   = -1,
   NODEID_BROADCAST = -2,
   NODEID_MULTICAST = -3
};
typedef int NodeID;

////////////////////////////////////////////////////////////////////////////////

} // namespace net

#endif // NODE_ID__H
