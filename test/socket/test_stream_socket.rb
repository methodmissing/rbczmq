# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)
require 'socket'
require 'timeout'

class TestStreamSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:STREAM)
    assert_equal ZMQ::STREAM, sock.type
    assert_equal "STREAM socket", sock.to_s
  ensure
    ctx.destroy
  end

  def test_recv_and_send
    ctx = ZMQ::Context.new
    sock = ctx.socket(:STREAM)
    port = sock.bind("tcp://127.0.0.1:*")

    tcp = TCPSocket.new('127.0.0.1', port)
    tcp.write("hello")

    Timeout.timeout(5) do
      # The first message we receive is a connection message:
      # The first frame is an identity frame.
      # The second frame is an empty frame.
      # see: https://github.com/zeromq/libzmq/pull/1487
      msg = sock.recv_message
      assert_equal 2, msg.size
      assert_equal "", msg.last.to_s

      msg = sock.recv_message

      # Messages received from a STREAM socket are in two parts:
      # The first frame is an identiy frame.
      # The second frame is the data received over the TCP socket.
      assert_equal 2, msg.size

      # first frame is the identity frame.
      identity = msg.pop
      assert_equal "hello", msg.first.to_s

      sock.sendm(identity)
      sock.send("goodbye")

      reply = tcp.recv(100)
      assert_equal "goodbye", reply
    end

  ensure
    tcp.close if tcp
    ctx.destroy
  end

  def test_recv_no_notify
    ctx = ZMQ::Context.new
    sock = ctx.socket(:STREAM)
    sock.stream_notify = false
    port = sock.bind("tcp://127.0.0.1:*")

    tcp = TCPSocket.new('127.0.0.1', port)
    tcp.write("hello")

    Timeout.timeout(5) do
      msg = sock.recv_message

      # Messages received from a STREAM socket are in two parts:
      # The first frame is an identiy frame.
      # The second frame is the data received over the TCP socket.
      assert_equal 2, msg.size

      # first frame is the identity frame.
      identity = msg.pop
      assert_equal "hello", msg.first.to_s
    end

  ensure
    tcp.close if tcp
    ctx.destroy
  end

  def test_notify_connect_disconnect
    ctx = ZMQ::Context.new
    sock = ctx.socket(:STREAM)
    sock.stream_notify = true
    port = sock.bind("tcp://127.0.0.1:*")

    tcp = TCPSocket.new('127.0.0.1', port)

    Timeout.timeout(5) do
      # The first message we receive is a connection message:
      # The first frame is an identity frame.
      # The second frame is an empty frame.
      # see: https://github.com/zeromq/libzmq/pull/1487
      msg = sock.recv_message
      assert_equal 2, msg.size
      assert_equal "", msg.last.to_s

      tcp.close

      msg = sock.recv_message
      assert_equal 2, msg.size
      assert_equal "", msg.last.to_s
    end

  ensure
    ctx.destroy
  end

  # This test should work, but may be a bug in zmq. leaving out for now:
  # def test_close_tcp_connection
  #   ctx = ZMQ::Context.new
  #   sock = ctx.socket(:STREAM)
  #   sock.stream_notify = false
  #   port = sock.bind("tcp://127.0.0.1:*")
  #
  #   tcp = TCPSocket.new('127.0.0.1', port)
  #   tcp.write("hello")
  #
  #   Timeout.timeout(5) do
  #     msg = sock.recv_message
  #     identity = msg.pop
  #
  #     # Close the connection by sending a zero-length message.
  #     sock.sendm(identity)
  #     sock.send("")
  #
  #     # receiving a zero length string is a TCP end of stream = closed normally.
  #     assert_equal "", tcp.recv(500)
  #   end
  #
  # ensure
  #   tcp.close if tcp
  #   ctx.destroy
  # end

end
