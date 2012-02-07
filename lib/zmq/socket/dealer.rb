# encoding: utf-8

class ZMQ::Socket::Dealer

# == ZMQ::Socket::Dealer
#
# A socket of type ZMQ::Socket::Dealer is an advanced pattern used for extending request/reply sockets. Each message sent is
# load-balanced among all connected peers, and each message received is fair-queued from all connected peers.
#
# Previously this socket was called ZMQ_XREQ and that name remains available for backwards compatibility.
#
# When a ZMQ::Socket::Dealer socket enters an exceptional state due to having reached the high water mark for all peers, or if
# there are no peers at all, then any ZMQ::Socket#send operations on the socket shall block until the exceptional state ends
# or at least one peer becomes available for sending; messages are not discarded.
#
# When a ZMQ::Socket::Dealer socket is connected to a ZMQ::Socket::Rep socket each message sent must consist of an empty message part,
# the delimiter, followed by one or more body parts.
#
# === Summary of ZMQ::Socket::Dealer characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Router, ZMQ::Socket::Request, ZMQ::Socket::Reply
# [Direction] Bidirectional
# [Send/receive pattern] Unrestricted
# [Outgoing routing strategy] Load-balanced
# [Incoming routing strategy] Fair-queued
# [ZMQ_HWM option action] Block

  TYPE_STR = "DEALER"

  def type
    ZMQ::DEALER
  end
end