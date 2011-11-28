# encoding: utf-8

class ZMQ::Socket::Router
  TYPE_STR = "ROUTER"

  def type
    ZMQ::ROUTER
  end

  include ZMQ::BiDirectionalSocket
end