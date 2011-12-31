# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestReqRepSockets < ZmqTestCase
  def test_flow
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "inproc://req-rep-flow.test")
    req = ctx.connect(:REQ, "inproc://req-rep-flow.test")
    begin
      rep.send("message")
    rescue => e
      assert_match(/Operation cannot be accomplished in current state/, e.message)
    end
  ensure
    ctx.destroy
  end

  def test_transfer
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "inproc://req-rep-transfer.test")
    req = ctx.connect(:REQ, "inproc://req-rep-transfer.test")
    req.send("message")
    assert_equal "message", rep.recv
  ensure
    ctx.destroy
  end

  def test_distribution
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "inproc://req-rep-distribution.test")
    thread = Thread.new do
      req = ctx.connect(:REQ, "inproc://req-rep-distribution.test")
      req.send_frame ZMQ::Frame("message")
      req.close
    end
    thread.join
    assert_equal ZMQ::Frame("message"), rep.recv_frame
  ensure
    ctx.destroy
  end
end