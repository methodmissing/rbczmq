# encoding: utf-8

class ZMQ::Socket
  def to_s
    case state
    when PENDING
      "#{type_str} socket"
    when BOUND
      "#{type_str} socket bound to #{endpoint}"
    when CONNECTED
      "#{type_str} socket connected to #{endpoint}"
    end
  end
end
