# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestPubSubSockets < ZmqTestCase
  def test_flow
    ctx = ZMQ::Context.new
    pub = ctx.bind(:PUB, "inproc://test.pub-sub-flow")
    sub = ctx.connect(:SUB, "inproc://test.pub-sub-flow")
    sub.subscribe("")
    pub.send("a")
    assert_equal "a", sub.recv
  ensure
    ctx.destroy
  end

  def test_transfer
    ctx = ZMQ::Context.new
    pub = ctx.bind(:PUB, "inproc://test.pub-sub-transfer")
    sub = ctx.connect(:SUB, "inproc://test.pub-sub-transfer")
    sub.subscribe("")
    pub.send("message")
    assert_equal "message", sub.recv

    pub.sendm("me")
    pub.sendm("ss")
    pub.send("age")
    assert_equal "me", sub.recv
    assert_equal "ss", sub.recv
    assert_equal "age", sub.recv

    pub.send_frame ZMQ::Frame("frame")
    assert_equal ZMQ::Frame("frame"), sub.recv_frame

    5.times do |i|
      frame = ZMQ::Frame("m#{i}")
      pub.send_frame(frame, ZMQ::Frame::MORE)
    end
    pub.send_frame(ZMQ::Frame("m5"))
    expected, frames = %w(m0 m1 m2 m3 m4 m5), []
    5.times do
      frames << sub.recv_frame.data
    end
    frames << sub.recv_frame.data
    assert_equal expected, frames

    msg = ZMQ::Message.new
    msg.push ZMQ::Frame("header")

    assert_nil pub.send_message(msg)

    recvd_msg = sub.recv_message
    assert_instance_of ZMQ::Message, recvd_msg
    assert_equal ZMQ::Frame("header"), recvd_msg.pop
  ensure
    ctx.destroy
  end

  def test_distribution
    ctx = ZMQ::Context.new
    pub = ctx.bind(:PUB, "inproc://test.pub-sub-distribution")
    threads = []
    5.times do |i|
      threads << Thread.new do
        sub = ctx.connect(:SUB, "inproc://test.pub-sub-distribution")
        sub.subscribe("")
        messages = []
        5.times do
          messages << sub.recv
        end
        sub.close
        messages
      end
    end

    sleep 0.5 # "slow joiner" syndrome
    messages = %w(a b c d e)
    messages.each do |m|
      pub.send m
    end

    threads.each{|t| t.join }
    assert threads.all?{|t| t.value == messages }
  ensure
    ctx.destroy
  end
end