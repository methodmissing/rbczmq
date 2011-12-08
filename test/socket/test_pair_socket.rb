# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestPairSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PAIR)
    assert_equal ZMQ::PAIR, sock.type
    assert_equal "PAIR socket", sock.to_s
  ensure
    ctx.destroy
  end
end