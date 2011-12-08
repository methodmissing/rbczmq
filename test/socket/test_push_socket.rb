# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestPushSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PUSH)
    assert_equal ZMQ::PUSH, sock.type
    assert_equal "PUSH socket", sock.to_s
  ensure
    ctx.destroy
  end
end