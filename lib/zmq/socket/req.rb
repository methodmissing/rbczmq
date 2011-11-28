# encoding: utf-8

class ZMQ::Socket::Req
  TYPE_STR = "REQ"

  def type
    ZMQ::REQ
  end

  include ZMQ::BiDirectionalSocket
end