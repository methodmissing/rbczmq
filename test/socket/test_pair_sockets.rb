# encoding: utf-8

require File.join(File.dirname(__FILE__), '..', 'helper')

class TestPairSockets < ZmqTestCase
  def test_flow
    ctx = ZMQ::Context.new
    a = ctx.bind(:PAIR, "inproc://test.pair-flow")
    b = ctx.connect(:PAIR, "inproc://test.pair-flow")
    a.send("a")
    b.send("b")
    assert_equal "b", a.recv
    assert_equal "a", b.recv
  ensure
    ctx.destroy
  end

  def test_transfer
    ctx = ZMQ::Context.new
    a = ctx.bind(:PAIR, "inproc://test.pair-transfer")
    b = ctx.connect(:PAIR, "inproc://test.pair-transfer")
    a.send("message")
    assert_equal "message", b.recv

    a.sendm("me")
    a.sendm("ss")
    a.send("age")
    assert_equal "me", b.recv
    assert_equal "ss", b.recv
    assert_equal "age", b.recv

    a.send_frame ZMQ::Frame("frame")
    assert_equal ZMQ::Frame("frame"), b.recv_frame

    5.times do |i|
      frame = ZMQ::Frame("m#{i}")
      a.send_frame(frame, ZMQ::Frame::MORE)
    end
    a.send_frame(ZMQ::Frame("m5"))
    expected, frames = %w(m0 m1 m2 m3 m4 m5), []
    5.times do
      frames << b.recv_frame.data
    end
    frames << b.recv_frame.data
    assert_equal expected, frames

    msg = ZMQ::Message.new
    msg.push ZMQ::Frame("header")

    assert_nil a.send_message(msg)

    recvd_msg = b.recv_message
    assert_instance_of ZMQ::Message, recvd_msg
    assert_equal ZMQ::Frame("header"), recvd_msg.pop
  ensure
    ctx.destroy
  end

  def test_distribution
    ctx = ZMQ::Context.new
    a = ctx.bind(:PAIR, "inproc://test.pair-distribution")
    thread = Thread.new do
      b = ctx.connect(:PAIR, "inproc://test.pair-distribution")
      frame = b.recv_frame
      b.close
      frame
    end

    a.send_frame ZMQ::Frame("message")
    assert_equal ZMQ::Frame("message"), thread.value
  ensure
    ctx.destroy
  end
end