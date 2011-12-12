# encoding: utf-8

class ZMQ::Socket::Req

# == ZMQ::Socket::Req
#
# A socket of type ZMQ::Socket::Req is used by a client to send requests to and receive replies from a service. This socket type allows
# only an alternating sequence of ZMQ::Socket#send and subsequent ZMQ::Socket#recv calls. Each request sent is load-balanced
# among all services, and each reply received is matched with the last issued request.
#
# When a ZMQ::Socket::Req socket enters an exceptional state due to having reached the high water mark for all services, or if there
# are no services at all, then any ZMQ::Socket#send operations on the socket shall block until the exceptional state ends or at
# least one service becomes available for sending; messages are not discarded.
#
# === Summary of ZMQ::Socket::Req characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Rep
# [Direction] Bidirectional
# [Send/receive pattern] Send, Receive, Send, Receive, …
# [Outgoing routing strategy] Load-balanced
# [Incoming routing strategy] Last peer
# [ZMQ::Socket#hwm option action] Block

  TYPE_STR = "REQ"

  def type
    ZMQ::REQ
  end

  unsupported_api :bind
end