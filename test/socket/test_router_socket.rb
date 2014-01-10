# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

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