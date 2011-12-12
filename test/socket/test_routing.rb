# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestRouting < ZmqTestCase
  def test_flow
    ctx = ZMQ::Context.new
    router = ctx.bind(:ROUTER, "inproc://routing-flow.test")
    dealer = ctx.socket(:DEALER)
    dealer.identity = "xyz"
    dealer.connect("inproc://routing-flow.test")

    router.sendm("xyz")
    router.send("request")
    assert_equal "request", dealer.recv

    dealer.send("reply")
    assert_equal "xyz", router.recv
    assert_equal "reply", router.recv
  ensure
    ctx.destroy
  end

  def test_transfer
    ctx = ZMQ::Context.new
    router = ctx.bind(:ROUTER, "inproc://routing-transfer.test")
    dealer = ctx.socket(:DEALER)
    dealer.identity = "xyz"
    dealer.connect("inproc://routing-transfer.test")

    router.send_frame ZMQ::Frame("xyz"), ZMQ::Frame::MORE
    router.send_frame ZMQ::Frame("request")

    assert_equal ZMQ::Frame("request"), dealer.recv_frame

    req = ZMQ::Message.new
    req.push ZMQ::Frame("request")
    req.push ZMQ::Frame("xyz")
    router.send_message req

    msg = dealer.recv_message
    assert_equal ZMQ::Frame("request"), msg.pop
  ensure
    ctx.destroy
  end

  def test_distribution
    ctx = ZMQ::Context.new
    router = ctx.bind(:ROUTER, "inproc://routing-distribution.test")
    thread = Thread.new do
      dealer = ctx.socket(:DEALER)
      dealer.identity = "xyz"
      dealer.connect("inproc://routing-distribution.test")
      frame = dealer.recv_frame
      dealer.close
      frame
    end

    sleep 0.5 # "slow joiner" syndrome
    router.send_frame ZMQ::Frame("xyz"), ZMQ::Frame::MORE
    router.send_frame ZMQ::Frame("message")
    assert_equal ZMQ::Frame("message"), thread.value
  ensure
    ctx.destroy
  end
end