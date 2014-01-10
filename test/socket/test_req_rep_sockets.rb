# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestReqRepSockets < ZmqTestCase
  def test_flow
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "inproc://test.req-rep-flow")
    ctx.connect(:REQ, "inproc://test.req-rep-flow")
    begin
      rep.send("message")
    rescue ZMQ::Error => e
      assert_match(/REP sockets allows only an alternating sequence of receive and subsequent send calls. Please assert that you're not sending \/ receiving out of band data when using the REQ \/ REP socket pairs./, e.message)
    end
  ensure
    ctx.destroy
  end

  def test_transfer
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "inproc://test.req-rep-transfer")
    req = ctx.connect(:REQ, "inproc://test.req-rep-transfer")
    req.send("message")
    assert_equal "message", rep.recv
  ensure
    ctx.destroy
  end

  def test_distribution
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "inproc://test.req-rep-distribution")
    thread = Thread.new do
      req = ctx.connect(:REQ, "inproc://test.req-rep-distribution")
      req.send_frame ZMQ::Frame("message")
      req.close
    end
    thread.join
    assert_equal ZMQ::Frame("message"), rep.recv_frame
  ensure
    ctx.destroy
  end
end