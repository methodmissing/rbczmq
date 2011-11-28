# encoding: utf-8

class ZMQ::Socket::Push
  TYPE_STR = "PUSH"

  def type
    ZMQ::PUSH
  end

  include ZMQ::SendSocket
end