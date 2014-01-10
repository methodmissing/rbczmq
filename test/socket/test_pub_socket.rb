# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestPubSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PUB)
    assert_equal ZMQ::PUB, sock.type
    assert_equal "PUB socket", sock.to_s
    assert_raises ZMQ::Error do
      sock.recv
    end
  ensure
    ctx.destroy
  end
end