# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmq < ZmqTestCase
  def test_interrupted_p
    assert !ZMQ.interrupted?
  end

  def test_version
    assert_equal [2,1,11], ZMQ.version
  end

  def test_now
    assert_instance_of Fixnum, ZMQ.now
  end

  def test_log
    assert_nil ZMQ.log("log message")
  end

  def test_error
    expected = [ZMQ::Error, NilClass]
    assert expected.any?{|c| c === ZMQ.error }
  end

  def test_device
    ctx = ZMQ::Context.new
    frontend = ctx.socket(:PUSH)
    frontend.bind('inproc://test.device-frontend')
    backend = ctx.socket(:PULL)
    backend.bind('inproc://test.device-backend')
    assert_raises TypeError do
      ZMQ.device(:inval, backend, frontend)
    end
    assert_raises ZMQ::Error do
      ZMQ.device(200, backend, frontend)
    end
    assert_raises TypeError do
      ZMQ.device(ZMQ::QUEUE, :backend, frontend)
    end
  ensure
    ctx.destroy
  end

  def test_streamer
    ctx = ZMQ::Context.new
    bport, fport = nil, nil
    Thread.new do
      frontend = ctx.socket(:PUSH)
      fport = frontend.bind('tcp://127.0.0.1:*')
      backend = ctx.socket(:PULL)
      bport = backend.bind('tcp://127.0.0.1:*')
      ZMQ.streamer(backend, frontend)
    end
    sleep 0.5

    push = ctx.socket(:PUSH)
    push.connect("tcp://127.0.0.1:#{bport}")
    pull = ctx.socket(:PULL)
    pull.connect("tcp://127.0.0.1:#{fport}")

    push.send("message")
    sleep 0.5
    assert_equal "message", pull.recv_nonblock
  ensure
    ctx.destroy
  end

  def test_queue
    ctx = ZMQ::Context.new
    bport, fport = nil, nil
    Thread.new do
      frontend = ctx.socket(:DEALER)
      fport = frontend.bind('tcp://127.0.0.1:*')
      backend = ctx.socket(:ROUTER)
      bport = backend.bind('tcp://127.0.0.1:*')
      ZMQ.queue(backend, frontend)
    end
    sleep 0.5

    rep = ctx.socket(:REP)
    rep.connect("tcp://127.0.0.1:#{fport}")
    req = ctx.socket(:REQ)
    req.connect("tcp://127.0.0.1:#{bport}")

    req.send("message")
    sleep 0.5
    assert_equal "message", rep.recv_nonblock
  ensure
    ctx.destroy
  end

  def test_forwarder
    ctx = ZMQ::Context.new
    bport, fport = nil, nil
    Thread.new do
      frontend = ctx.socket(:PUB)
      fport = frontend.bind('tcp://127.0.0.1:*')
      backend = ctx.socket(:SUB)
      bport = backend.bind('tcp://127.0.0.1:*')
      ZMQ.forwarder(backend, frontend)
    end
    sleep 0.5

    pub = ctx.socket(:PUB)
    pub.connect("tcp://127.0.0.1:#{bport}")
    sub = ctx.socket(:SUB)
    sub.connect("tcp://127.0.0.1:#{fport}")

    pub.send("message")
    sleep 0.5
    assert_equal "message", sub.recv_nonblock
  ensure
    ctx.destroy
  end
end