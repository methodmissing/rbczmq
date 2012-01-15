# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestReqSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REQ)
    assert_equal ZMQ::REQ, sock.type
    assert_equal "REQ socket", sock.to_s
  ensure
    ctx.destroy
  end

  def test_flow
    ctx = ZMQ::Context.new
    rep = ctx.socket(:REP)
    rep.bind("inproc://test.req-sock-flow")
    sock = ctx.socket(:REQ)
    sock.connect("inproc://test.req-sock-flow")
    assert_raises ZMQ::Error do
      sock.send_frame(ZMQ::Frame("frame"), ZMQ::Frame::MORE)
    end
  ensure
    ctx.destroy
  end
end