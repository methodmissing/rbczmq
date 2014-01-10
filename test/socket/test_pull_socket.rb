# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestPullSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PULL)
    assert_equal ZMQ::PULL, sock.type
    assert_equal "PULL socket", sock.to_s
    assert_raises ZMQ::Error do
      sock.send("message")
    end
  ensure
    ctx.destroy
  end
end