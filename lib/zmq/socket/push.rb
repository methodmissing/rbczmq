# encoding: utf-8

class ZMQ::Socket::Push

# == ZMQ::Socket::Push
#
# A socket of type ZMQ::Socket::Push is used by a pipeline node to send messages to downstream pipeline nodes. Messages are
# load-balanced to all connected downstream nodes. The ZMQ::Socket#recv function is not implemented for this socket type.
#
# When a ZMQ::Socket::Push socket enters an exceptional state due to having reached the high water mark for all downstream
# nodes, or if there are no downstream nodes at all, then any ZMQ::Socket#send operations on the socket shall block until
# the exceptional state ends or at least one downstream node becomes available for sending; messages are not discarded.
#
# Deprecated alias: ZMQ_DOWNSTREAM.
#
# === Summary of ZMQ::Socket::Push characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Pull
# [Direction] Unidirectional
# [Send/receive pattern] Send only
# [Incoming routing strategy] N/A
# [Outgoing routing strategy] Load-balanced
# [ZMQ::Socket#hwm option action] Block

  TYPE_STR = "PUSH"

  def type
    ZMQ::PUSH
  end

  include ZMQ::UpstreamSocket
end