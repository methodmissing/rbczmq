# encoding: utf-8

class ZMQ::Socket::Sub

# == ZMQ::Socket::Sub
#
# A socket of type ZMQ::Socket::Sub is used by a subscriber to subscribe to data distributed by a publisher. Initially a
# ZMQ::Socket::Sub socket is not subscribed to any messages, use ZMQ::Socket#subscribe to specify which messages to
# subscribe to. The ZMQ::Socket#send function is not implemented for this socket type.
#
# === Summary of ZMQ::Socket::Sub characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Pub
# [Direction] Unidirectional
# [Send/receive pattern] Receive only
# [Incoming routing strategy] Fair-queued
# [Outgoing routing strategy] N/A
# [ZMQ::Socket#hwm option action] Drop

  TYPE_STR = "SUB"

  def type
    ZMQ::SUB
  end

  include ZMQ::DownstreamSocket
end