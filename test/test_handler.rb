# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmqHandler < ZmqTestCase
  def test_expects_socket
    assert_raises TypeError do
      ZMQ::Handler.new(:socket)
    end
  end

  def test_readable_writable_contracts
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PAIR)
    handler = ZMQ::Handler.new(sock)
    assert_raises NotImplementedError do
      handler.on_readable
    end
    assert_raises NotImplementedError do
      handler.on_writable
    end
  ensure
    ctx.destroy
  end

  def test_error
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PAIR)
    handler = ZMQ::Handler.new(sock)
    assert_raises StandardError do
      handler.on_error(StandardError.new)
    end
  ensure
    ctx.destroy
  end
end