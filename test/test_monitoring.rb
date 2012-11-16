# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestMonitor
  attr_reader :listening, :closed

  def on_listening(addr, fd)
    @listening = true
  end

  def on_closed(addr, fd)
    @closed = true
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
    sock.bind("tcp://0.0.0.0:5555")
    sleep 1
    assert cb.listening
    sock.close
    sleep 1
    assert cb.closed
  ensure
    ctx.destroy
  end
end