# encoding: utf-8

class ZMQ::Socket::Rep

# == ZMQ::Socket::Rep
#
# A socket of type ZMQ::Socket::Rep is used by a service to receive requests from and send replies to a client. This socket type
# allows only an alternating sequence of ZMQ::Socket#recv and subsequent ZMQ::Socket#send calls. Each request received is
# fair-queued from among all clients, and each reply sent is routed to the client that issued the last request. If the
# original requester doesn't exist any more the reply is silently discarded.
#
# When a ZMQ::Socket::Rep socket enters an exceptional state due to having reached the high water mark for a client, then any replies
# sent to the client in question shall be dropped until the exceptional state ends.
#
# === Summary of ZMQ::Socket#rep characteristics
#
# [Compatible peer sockets] ZMQ::Socket::Rep
# [Direction] Bidirectional
# [Send/receive pattern] Receive, Send, Receive, Send, …
# [Incoming routing strategy] Fair-queued
# [Outgoing routing strategy] Last peer
# [ZMQ::Socket#hwm option action] Drop

  TYPE_STR = "REP"

  def type
    ZMQ::REP
  end

  def send_frame(frame, flags = 0)
    raise ZMQ::Error, "cannot send multiple frames on REP sockets" if flags == ZMQ::Frame::MORE
    super
  end

  unsupported_api :connect, :sendm
end