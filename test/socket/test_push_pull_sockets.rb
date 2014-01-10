# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestPushPullSockets < ZmqTestCase
  def test_flow
    ctx = ZMQ::Context.new
    push = ctx.bind(:PUSH, "inproc://test.push-pull-flow")
    pull = ctx.connect(:PULL, "inproc://test.push-pull-flow")
    push.send("a")
    assert_equal "a", pull.recv
  ensure
    ctx.destroy
  end

  def test_transfer
    ctx = ZMQ::Context.new
    push = ctx.bind(:PUSH, "inproc://test.push-pull-transfer")
    pull = ctx.connect(:PULL, "inproc://test.push-pull-transfer")
    push.send("message")
    assert_equal "message", pull.recv

    push.sendm("me")
    push.sendm("ss")
    push.send("age")
    assert_equal "me", pull.recv
    assert_equal "ss", pull.recv
    assert_equal "age", pull.recv

    push.send_frame ZMQ::Frame("frame")
    assert_equal ZMQ::Frame("frame"), pull.recv_frame

    5.times do |i|
      frame = ZMQ::Frame("m#{i}")
      push.send_frame(frame, ZMQ::Frame::MORE)
    end
    push.send_frame(ZMQ::Frame("m5"))
    expected, frames = %w(m0 m1 m2 m3 m4 m5), []
    5.times do
      frames << pull.recv_frame.data
    end
    frames << pull.recv_frame.data
    assert_equal expected, frames

    msg = ZMQ::Message.new
    msg.push ZMQ::Frame("header")

    assert_nil push.send_message(msg)

    recvd_msg = pull.recv_message
    assert_instance_of ZMQ::Message, recvd_msg
    assert_equal ZMQ::Frame("header"), recvd_msg.pop
  ensure
    ctx.destroy
  end

  def test_distribution
    ctx = ZMQ::Context.new
    push = ctx.bind(:PUSH, "inproc://test.push-pull-distribution")
    threads = []
    5.times do |i|
      threads << Thread.new do
        pull = ctx.connect(:PULL, "inproc://test.push-pull-distribution")
        msg = pull.recv
        pull.close
        msg
      end
    end

    sleep 0.5 # "slow joiner" syndrome
    messages = %w(a b c d e f)
    messages.each do |m|
      push.send m
    end

    threads.each{|t| t.join }
    assert threads.all?{|t| messages.include?(t.value) }
  ensure
    ctx.destroy
  end
end