# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestPairSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PAIR)
    assert_equal ZMQ::PAIR, sock.type
    assert_equal "PAIR socket", sock.to_s
  ensure
    ctx.destroy
  end

  def test_inproc_only_transport
    ctx = ZMQ::Context.new
    sock = ctx.socket(:PAIR)
    assert_raises ZMQ::Error do
      sock.bind("tcp://127.0.0.1:*")
    end
  ensure
    ctx.destroy
  end
end