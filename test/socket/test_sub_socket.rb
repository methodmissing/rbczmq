# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestSubSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:SUB)
    assert_equal ZMQ::SUB, sock.type
    assert_equal "SUB socket", sock.to_s
    assert_raises ZMQ::Error do
      sock.send("message")
    end
    assert_raises ZMQ::Error do
      sock.bind("tcp://127.0.0.1:*")
    end
  ensure
    ctx.destroy
  end
end