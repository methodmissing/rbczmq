# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestPushSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PUSH)
    assert_equal ZMQ::PUSH, sock.type
    assert_equal "PUSH socket", sock.to_s
    assert_raises ZMQ::Error do
      sock.recv
    end
  ensure
    ctx.destroy
  end
end