# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmqPoller < ZmqTestCase

  def test_alloc
    assert_instance_of ZMQ::Poller, ZMQ::Poller.new
  end

  def test_poll
    ctx = ZMQ::Context.new
    poller = ZMQ::Poller.new
    assert_equal 0, poller.poll
    rep = ctx.socket(:REP)
    rep.linger = 0
    rep.bind("inproc://test.poll")
    req = ctx.socket(:REQ)
    req.linger = 0
    req.connect("inproc://test.poll")

    assert_raises TypeError do
      poller.poll :invalid
    end

    assert_equal 0, poller.poll_nonblock

    assert poller.register_readable(rep)
    sleep 0.2
    assert req.send("request")
    sleep 0.1

    assert_equal 1, poller.poll(1)
    assert_equal [rep], poller.readables
    assert_equal [], poller.writables
    rep.recv

    assert poller.register(req)
    sleep 0.2
    assert rep.send("reply")
    sleep 0.1

    assert_equal 1, poller.poll(1)
    assert_equal [req], poller.readables
    assert_equal [], poller.writables
  ensure
    ctx.destroy
  end

  def test_register
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, 'inproc://test.poller-register')
    req = ctx.connect(:REQ, 'inproc://test.poller-register')
    poller = ZMQ::Poller.new
    assert poller.register(rep, ZMQ::POLLIN)
    assert !poller.register(req, 0)
    unbound = ctx.socket(:REP)
    assert_raises ZMQ::Error do
      poller.register(unbound, ZMQ::POLLIN)
    end
  ensure
    ctx.destroy
  end

  def test_register_readable
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, 'inproc://test.poller-register_readable')
    poller = ZMQ::Poller.new
    assert poller.register_readable(rep)
  ensure
    ctx.destroy
  end

  def test_register_writable
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, 'inproc://test.poller-register_writable')
    poller = ZMQ::Poller.new
    assert poller.register_writable(rep)
  ensure
    ctx.destroy
  end

  def test_remove
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, 'inproc://test.poller-remove')
    req = ctx.connect(:REQ, 'inproc://test.poller-remove')
    poller = ZMQ::Poller.new
    assert poller.register(req)
    assert poller.remove(req)
    assert !poller.remove(rep)
  ensure
    ctx.destroy
  end

  def test_readables
    poller = ZMQ::Poller.new
    assert_equal [], poller.readables
  end

  def test_writables
    poller = ZMQ::Poller.new
    assert_equal [], poller.writables
  end
end