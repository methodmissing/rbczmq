# encoding: utf-8

class ZMQ::Socket
  PROTO_REXP = /^inproc|ipc|tcp|e?pgm:\/\//

  def self.unsupported_api(*methods)
    methods.each do |m|
      class_eval <<-"evl", __FILE__, __LINE__
        def #{m}(*args); raise(ZMQ::Error, "API #{m} not supported for #{const_get(:TYPE_STR)} sockets!");  end
      evl
    end
  end

  def self.handle_fsm_errors(error, *methods)
    methods.each do |m|
      class_eval <<-"evl", __FILE__, __LINE__
        def #{m}(*args);
          super
        rescue SystemCallError => e
          raise(ZMQ::Error, "#{error} Please assert that you're not sending / receiving out of band data when using the REQ / REP socket pairs.") if e.errno == ZMQ::EFSM
          raise
        end
      evl
    end
  end

  # Determines if there are one or more messages to read from this socket. Should be used in conjunction with the
  # ZMQ_FD socket option for edge-triggered notifications.
  #
  # socket.readable? => true
  #
  def readable?
    (events & ZMQ::POLLIN) == ZMQ::POLLIN
  end

  # Determines if this socket is in a writable state. Should be used in conjunction with the ZMQ_FD socket option for
  # edge-triggered notifications.
  #
  # socket.writable? => true
  #
  def writable?
    (events & ZMQ::POLLOUT) == ZMQ::POLLOUT
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

  # Poll all sockets for readbable states by default
  def poll_readable?
    true
  end

  # Poll all sockets for writable states by default
  def poll_writable?
    true
  end

  # Binds to a given endpoint. Attemps to resolve URIs without a protocol through DNS SRV records.
  #
  # socket = ctx.socket(:PUB)
  # socket.bind "tcp://127.0.0.1:9000"
  #
  # socket.bind "collector.domain.com" # resolves 10.0.0.2:9000
  #
  def bind(uri)
    uri = resolve(uri) if uri && uri !~ PROTO_REXP
    real_bind(uri)
  end

  # Connects to a given endpoint. Attemps to resolve URIs without a protocol through DNS SRV records.
  #
  # socket = ctx.socket(:PUB)
  # socket.connect "tcp://127.0.0.1:9000"
  #
  # socket.connect "collector.domain.com" # resolves 10.0.0.2:9000
  #
  def connect(uri)
    uri = resolve(uri) if uri && uri !~ PROTO_REXP
    real_connect(uri)
  end

  private
  # Attempt to resolve DNS SRV records ( http://en.wikipedia.org/wiki/SRV_record ). Respects priority and weight
  # to provide a combination of load balancing and backup.
  def resolve(uri)
    parts = uri.split('.')
    service = parts.shift
    domain = parts.join(".")
    # ZeroMQ does not yet support udp, but may look into possibly supporting [e]pgm here
    resources = ZMQ.resolver.getresources("_#{service}._tcp.#{domain}", Resolv::DNS::Resource::IN::SRV)
    # lowest-numbered priority value is preferred
    resources.sort!{|a,b| a.priority <=> b.priority }
    res = resources.first
    # detetermine if we should filter by weight as well (multiple records with equal priority)
    priority_peers = resources.select{|r| res.priority == r.priority }
    if priority_peers.size > 1
      # highest weight preferred
      res = priority_peers.sort{|a,b| a.weight <=> b.weight }.last
    end
    return uri unless res
    "tcp://#{Resolv.getaddress(res.target.to_s)}:#{res.port}"
  rescue
    uri
  end
end

module ZMQ::DownstreamSocket
  # An interface for sockets that can only receive (read) data
  #
  # === Behavior
  #
  # [Disabled methods] ZMQ::Socket#bind, ZMQ::Socket#send, ZMQ::Socket#sendm, ZMQ::Socket#send_frame,
  #                    ZMQ::Socket#send_message
  # [Socket types] ZMQ::Socket::Pull, ZMQ::Socket::Sub

  def self.included(sock)
    sock.unsupported_api :send, :sendm, :send_frame, :send_message
  end

  # Upstream sockets should never be polled for writable states
  def poll_writable?
    false
  end
end

module ZMQ::UpstreamSocket
  # An interface for sockets that can only send (write) data
  #
  # === Behavior
  #
  # [Disabled methods] ZMQ::Socket#connect, ZMQ::Socket#recv, ZMQ::Socket#recv_nonblock, ZMQ::Socket#recv_frame,
  #                    ZMQ::Socket#recv_frame_nonblock, ZMQ::Socket#recv_message
  # [Socket types] ZMQ::Socket::Push, ZMQ::Socket::Pub

  def self.included(sock)
    sock.unsupported_api :recv, :recv_nonblock, :recv_frame, :recv_frame_nonblock, :recv_message
  end

  # Upstream sockets should never be polled for readable states
  def poll_readable?
    false
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
