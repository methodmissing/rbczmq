# encoding: utf-8

class ZMQ::Socket::Rep
  TYPE_STR = "REP"

  def type
    ZMQ::REP
  end

  unsupported_api :connect
end