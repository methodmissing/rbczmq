# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestRepSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REP)
    assert_equal ZMQ::REP, sock.type
    assert_equal "REP socket", sock.to_s
  ensure
    ctx.destroy
  end

  def test_flow
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REP)
    sock.bind("inproc://test.rep-sock-flow")
    assert_raises ZMQ::Error do
      sock.send_frame(ZMQ::Frame("frame"), ZMQ::Frame::MORE)
    end
  ensure
    ctx.destroy
  end
end