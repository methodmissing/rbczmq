# encoding: utf-8

class ZMQ::Socket::Pub
  TYPE_STR = "PUB"

  def type
    ZMQ::PUB
  end

  include ZMQ::SendSocket
  unsupported_api :connect
end