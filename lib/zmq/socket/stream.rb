# encoding: utf-8

class ZMQ::Socket::Stream

# == ZMQ::Socket::Stream
# A socket of type ZMQ::Socket::Stream is used to send and receive TCP data from a non-ØMQ peer,
# when using the tcp:// transport. A ZMQ::Socket::Stream socket can act as client and/or server,
# sending and/or receiving TCP data asynchronously.
#
# When receiving TCP data, a ZMQ::Socket::Stream socket shall prepend a message part containing
# the identity of the originating peer to the message before passing it to the application.
# Messages received are fair-queued from among all connected peers.
#
# When sending TCP data, a ZMQ::Socket::Stream socket shall remove the first part of the message
# and use it to determine the identity of the peer the message shall be routed to, and unroutable
# messages shall cause an EHOSTUNREACH or EAGAIN error.
#
# To open a connection to a server, use the zmq_connect call, and then fetch the socket identity
# using the ZMQ_IDENTITY zmq_getsockopt call.
#
# To close a specific client connection, as a server, send the identity frame followed by a zero-
# length message (see EXAMPLE section).
#
# The ZMQ_MSGMORE flag is ignored on data frames. You must send one identity frame followed by
# one data frame.
#
# Also, please note that omitting the ZMQ_MSGMORE flag will prevent sending further data (from
# any client) on the same socket.
#
# === Summary of ZMQ_STREAM characteristics
#
# [Compatible peer sockets]  none.
# [Direction] Bidirectional
# [Send/receive pattern] Unrestricted
# [Outgoing routing strategy] See text
# [Incoming routing strategy] Fair-queued
# [ZMQ::Socket#hwm option action] EAGAIN

  TYPE_STR = "STREAM"

  def type
    ZMQ::STREAM
  end
end
