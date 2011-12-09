# encoding: utf-8

class ZMQ::Socket::Sub
  TYPE_STR = "SUB"

  def type
    ZMQ::SUB
  end

  include ZMQ::DownstreamSocket
end