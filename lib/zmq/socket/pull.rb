# encoding: utf-8

class ZMQ::Socket::Pull
  TYPE_STR = "PULL"

  def type
    ZMQ::PULL
  end

  include ZMQ::ReceiveSocket
end