# encoding: utf-8

class ZMQ::Socket::Rep
  TYPE_STR = "REP"

  def type
    ZMQ::REP
  end

  include ZMQ::BiDirectionalSocket
end