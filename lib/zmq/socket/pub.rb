# encoding: utf-8

class ZMQ::Socket::Pub

# == ZMQ::Socket::Pub
#
# A socket of type ZMQ::Socket::Pub is used by a publisher to distribute data. Messages sent are distributed in a fan out fashion
# to all connected peers. The ZMQ::Socket#recv function is not implemented for this socket type.
#
# When a ZMQ::Socket::Pub socket enters an exceptional state due to having reached the high water mark for a subscriber, then
# any messages that would be sent to the subscriber in question shall instead be dropped until the exceptional state ends. The
# ZMQ::Socket#send function shall never block for this socket type.
#
# === Summary of ZMQ::Socket::Pub characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Sub
# [Direction] Unidirectional
# [Send/receive pattern] Send only
# [Incoming routing strategy] N/A
# [Outgoing routing strategy] Fan out
# [ZMQ::Socket#hwm option action] Drop

  TYPE_STR = "PUB"

  def type
    ZMQ::PUB
  end

  include ZMQ::UpstreamSocket
end