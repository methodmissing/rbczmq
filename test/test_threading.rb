# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

Thread.abort_on_exception = true

class TestZmqThreading < ZmqTestCase
  def test_threaded
    ctx = ZMQ::Context.new
    rep, req, threads = nil, nil, []
    # Spawn a few threads in sleep state
    5.times{|i| threads << Thread.new{ sleep(0.5); i } }
    # Spawn a few CPU bound threads
    5.times{|i| threads << Thread.new{ 500_000.times{} } }
    expected = "threaded message"
    threads << Thread.new do 
      rep = ctx.socket ZMQ::PAIR
      rep.bind('inproc://test.threaded')
      sleep 0.5
      rep.recv
    end
    sleep 0.3
    threads << Thread.new do
      req = ctx.socket ZMQ::PAIR
      req.connect('inproc://test.threaded')
      req.send(expected)
    end
    Thread.pass
    sleep 0.3
    threads.map!{|t| t.value }
    thread_vals = [0, 1, 2, 3, 4, 500_000, 500_000, 500_000, 500_000, 500_000, expected, true]
    assert_equal thread_vals, threads
  end
end