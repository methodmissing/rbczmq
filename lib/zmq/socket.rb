# encoding: utf-8

class ZMQ::Socket
  def self.unsupported_api(*methods)
    methods.each do |m|
      class_eval <<-"evl", __FILE__, __LINE__
        def #{m}(*args); raise(ZMQ::Error, "API #{m} not supported for #{const_get(:TYPE_STR)} sockets!");  end
      evl
    end
  end

  # Determines if this socket is in a readable state.
  #
  # socket.readable? => true
  #
  def readable?
    events | ZMQ::POLLIN
  end

  # Determines if this socket is in a writable state.
  #
  # socket.writable? => true
  #
  def writable?
    events | ZMQ::POLLOUT
  end

  # Generates a string representation of this socket type
  #
  # socket = ctx.socket(:PUB)
  # socket.type_str => "PUB"
  #
  def type_str
    self.class.const_get(:TYPE_STR)
  end

  # Generates a string representation of the current socket state
  #
  # socket = ctx.bind(:PUB, "tcp://127.0.0.1:5000")
  # socket.to_s => "PUB socket bound to tcp://127.0.0.1:5000"
  #
  def to_s
    case state
    when BOUND
      "#{type_str} socket bound to #{endpoint}"
    when CONNECTED
      "#{type_str} socket connected to #{endpoint}"
    else
      "#{type_str} socket"
    end
  end
end

module ZMQ::ReceiveSocket
  # An interface for sockets that can only receive (read) data
  #

  def self.included(sock)
    sock.unsupported_api :send, :sendm, :send_frame, :send_message
  end

  def poll_readable?
    true
  end

  def poll_writable?
    false
  end
end

module ZMQ::SendSocket
  # An interface for sockets that can only send (write) data
  #

  def self.included(sock)
    sock.unsupported_api :recv, :recv_nonblock, :recv_frame, :recv_frame_nonblock, :recv_message
  end

  def poll_readable?
    false
  end

  def poll_writable?
    true
  end
end

module ZMQ::BiDirectionalSocket
  # An interface for sockets that can both send and receive data as per the ZMQ spec.
  #

  def poll_readable?
    true
  end

  def poll_writable?
    true
  end
end

require "zmq/socket/pub"
require "zmq/socket/sub"
require "zmq/socket/push"
require "zmq/socket/pull"
require "zmq/socket/pair"
require "zmq/socket/req"
require "zmq/socket/rep"
require "zmq/socket/router"
require "zmq/socket/dealer"
