# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmqPollitem < ZmqTestCase
  def test_alloc_io
    pollitem = ZMQ::Pollitem.new(STDIN, ZMQ::POLLIN)
    assert_equal STDIN, pollitem.pollable
    assert_equal ZMQ::POLLIN, pollitem.events
  end

  def test_alloc_without_events
    pollitem = ZMQ::Pollitem.new(STDIN)
    assert pollitem.events & ZMQ::POLLIN
    assert pollitem.events & ZMQ::POLLOUT
  end

  def test_alloc_socket
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, 'inproc://test.pollitem-alloc_socket')
    pollitem = ZMQ::Pollitem.new(rep, ZMQ::POLLIN)
    assert_equal rep, pollitem.pollable
    assert_equal ZMQ::POLLIN, pollitem.events
  ensure
    ctx.destroy
  end

  def test_alloc_ruby_socket
    srv = TCPServer.new("127.0.0.1", 0)
    pollitem = ZMQ::Pollitem.new(srv, ZMQ::POLLIN)
    assert_equal srv, pollitem.pollable
    assert_equal ZMQ::POLLIN, pollitem.events
  ensure
    srv.close if srv
  end

  def test_verbose
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, 'inproc://test.pollitem-verbose')
    pollitem = ZMQ::Pollitem.new(rep, ZMQ::POLLIN)
    pollitem.verbose = true
  ensure
    ctx.destroy
  end

  def test_handler
    pollitem = ZMQ::Pollitem.new(STDIN, ZMQ::POLLIN)
    assert_nil pollitem.handler
    handler = Module.new
    pollitem.handler = handler
    assert_equal handler, pollitem.handler
  end

  def test_coerce
    pollitem = ZMQ::Pollitem.new(STDIN)
    assert_equal pollitem, ZMQ::Pollitem.coerce(pollitem)
    coerced = ZMQ::Pollitem.coerce(STDIN)
    assert_instance_of ZMQ::Pollitem, coerced
    assert_equal STDIN, coerced.pollable
  end

  def test_alloc_failures
    ctx = ZMQ::Context.new
    rep = ctx.socket(:REP)
    assert_raises ZMQ::Error do
      ZMQ::Pollitem.new(rep, ZMQ::POLLIN)
    end
    rep.bind('inproc://test.pollitem-alloc_failures')
    assert_raises ZMQ::Error do
      ZMQ::Pollitem.new(rep, 200)
    end
    assert_raises TypeError do
      ZMQ::Pollitem.new(:invalid, ZMQ::POLLIN)
    end
  ensure
    ctx.destroy
  end
end