# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestRouterSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:ROUTER)
    assert_equal ZMQ::ROUTER, sock.type
    assert_equal "ROUTER socket", sock.to_s
  ensure
    ctx.destroy
  end
end