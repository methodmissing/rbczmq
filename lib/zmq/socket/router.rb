# encoding: utf-8

class ZMQ::Socket::Router

# == ZMQ::Socket::Router
# 
# A socket of type ZMQ::Socket::Router is an advanced pattern used for extending request/reply sockets. When receiving messages
# a ZMQ::Socket::Router socket shall prepend a message part containing the identity of the originating peer to the message before
# passing it to the application. Messages received are fair-queued from among all connected peers. When sending messages a
# ZMQ::Socket::Router socket shall remove the first part of the message and use it to determine the identity of the peer the message
# shall be routed to. If the peer does not exist anymore the message shall be silently discarded.
#
# Previously this socket was called ZMQ_XREP and that name remains available for backwards compatibility.
#
# When a ZMQ::Socket::Router socket enters an exceptional state due to having reached the high water mark for all peers, or if
# there are no peers at all, then any messages sent to the socket shall be dropped until the exceptional state ends. Likewise, any
# messages routed to a non-existent peer or a peer for which the individual high water mark has been reached shall also be dropped.
#
# When a ZMQ::Socket::Request socket is connected to a ZMQ::Socket::Router socket, in addition to the identity of the originating
# peer each message received shall contain an empty delimiter message part. Hence, the entire structure of each received message as
# seen by the application becomes: one or more identity parts, delimiter part, one or more body parts. When sending replies to a
# ZMQ::Socket::Request socket the application must include the delimiter part.
#
# === Summary of ZMQ_ROUTER characteristics
#
# [Compatible peer socket] ZMQ::Socket::Dealer, ZMQ::Socket::Req, ZMQ::Socket::Rep
# [Direction] Bidirectional
# [Send/receive pattern] Unrestricted
# [Outgoing routing strategy] See text
# [Incoming routing strategy] Fair-queued
# [ZMQ::Socket#hwm option action] Drop

  TYPE_STR = "ROUTER"

  def type
    ZMQ::ROUTER
  end
end