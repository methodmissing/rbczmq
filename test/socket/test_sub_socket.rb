# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestSubSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:SUB)
    assert_equal ZMQ::SUB, sock.type
    assert_equal "SUB socket", sock.to_s
    assert_raises ZMQ::Error do
      sock.send("message")
    end
  ensure
    ctx.destroy
  end
end