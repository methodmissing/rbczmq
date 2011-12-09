# encoding: utf-8

class ZMQ::Socket::Pair
  TYPE_STR = "PAIR"

  def type
    ZMQ::PAIR
  end
end