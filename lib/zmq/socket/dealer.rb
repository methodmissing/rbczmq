# encoding: utf-8

class ZMQ::Socket::Dealer
  TYPE_STR = "DEALER"

  def type
    ZMQ::DEALER
  end
end