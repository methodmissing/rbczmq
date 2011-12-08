# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestPullSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PULL)
    assert_equal ZMQ::PULL, sock.type
    assert_equal "PULL socket", sock.to_s
  ensure
    ctx.destroy
  end
end