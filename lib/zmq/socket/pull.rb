# encoding: utf-8

class ZMQ::Socket::Pull

# == ZMQ::Socket::Pull
#
# A socket of type ZMQ::Socket::Pull is used by a pipeline node to receive messages from upstream pipeline nodes. Messages
# are fair-queued from among all connected upstream nodes. The ZMQ::Socket#send function is not implemented for this
# socket type.
#
# Deprecated alias: ZMQ_UPSTREAM.
#
# === Summary of ZMQ::Socket::Pull characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Push
# [Direction] Unidirectional
# [Send/receive pattern] Receive only
# [Incoming routing strategy] Fair-queued
# [Outgoing routing strategy] N/A
# [ZMQ::Socket#hwm option action] N/A

  TYPE_STR = "PULL"

  def type
    ZMQ::PULL
  end

  include ZMQ::DownstreamSocket
end