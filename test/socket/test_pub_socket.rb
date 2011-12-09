# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestPubSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PUB)
    assert_equal ZMQ::PUB, sock.type
    assert_equal "PUB socket", sock.to_s
    assert_raises ZMQ::Error do
      sock.recv
    end
    assert_raises ZMQ::Error do
      sock.connect("tcp://127.0.0.1:5000")
    end
  ensure
    ctx.destroy
  end
end