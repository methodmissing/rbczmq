# encoding: utf-8

class ZMQ::Socket::Pair

# == ZMQ::Socket::Pair
#
# A socket of type ZMQ::Socket::Pair can only be connected to a single peer at any one time. No message routing or filtering
# is performed on messages sent over a ZMQ::Socket::Pair socket.
#
# When a ZMQ::Socket::Pair socket enters an exceptional state due to having reached the high water mark for the connected
# peer, or if no peer is connected, then any ZMQ::Socket#send operations on the socket shall block until the peer becomes
# available for sending; messages are not discarded.
#
# === Summary of ZMQ::Socket::Pair characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Pair
# [Direction] Bidirectional
# [Send/receive pattern] Unrestricted
# [Incoming routing strategy] N/A
# [Outgoing routing strategy] N/A
# [ZMQ::Socket#hwm option action] Block

  TYPE_STR = "PAIR"
  REXP_INPROC = /inproc:\/\//

  def bind(endpoint)
    raise(ZMQ::Error, "PAIR sockets can only listen using the inproc:// transport") unless endpoint =~ REXP_INPROC
    super
  end

  def connect(endpoint)
    raise(ZMQ::Error, "PAIR sockets can only connect using the inproc:// transport") unless endpoint =~ REXP_INPROC
    super
  end

  def type
    ZMQ::PAIR
  end
end