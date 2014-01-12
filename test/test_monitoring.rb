# encoding: utf-8

require File.expand_path("../helper.rb", __FILE__)

class TestMonitor
  attr_reader :listening, :closed, :accepted

  def on_listening(addr, fd)
    @listening = true
  end

  def on_closed(addr, fd)
    @closed = true
  end

  def on_accepted(addr, fd)
    @accepted = true
  end

  def on_bind_failed(addr, fd)
    @bind_failed = true
  end
end

class TestZmqMonitoring < ZmqTestCase
  def test_monitoring
    ctx = ZMQ::Context.new
    sock = ctx.socket(:REP)
    assert_raises TypeError do
      sock.monitor(:invalid)
    end

    assert_raises TypeError do
      sock.monitor("inproc://monitor.rep", nil, :invalid)
    end

    cb = TestMonitor.new

    assert !sock.monitor("tcp://0.0.0.0:5000")
    assert sock.monitor("inproc://monitor.rep", cb)
    sleep 1
    port = sock.bind("tcp://127.0.0.1:*")
    sleep 1
    assert cb.listening
    client = ctx.socket(:REQ)
    client.connect("tcp://127.0.0.1:#{port}")
    sleep 1
    assert cb.accepted
    sock.close
    sleep 1
    assert cb.closed
  ensure
    ctx.destroy
  end
end
