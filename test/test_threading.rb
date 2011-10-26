# encoding: utf-8

require 'test/unit'
require 'zmq'

Thread.abort_on_exception = true

class TestZmqThreading < Test::Unit::TestCase
  def test_threaded
    ctx = ZMQ::Context.new
    rep = ctx.socket ZMQ::PAIR
    req = ctx.socket ZMQ::PAIR
    port, threads = nil, []
    10.times{|i| threads << Thread.new{ sleep(0.5); i } }
    Thread.new{ port = rep.bind('tcp://127.0.0.1:*') }.join
    req.connect("tcp://127.0.0.1:#{port}")
    expected, received = "threaded message via port #{port}", nil
    Thread.new{ req.send(expected) }
    Thread.pass
    Thread.new{ received = rep.recv }
    Thread.pass
    sleep 0.1
    assert_equal received, expected
    threads.map!{|t| t.join; t.value }
    assert_equal Array(0..9), threads
  end
end