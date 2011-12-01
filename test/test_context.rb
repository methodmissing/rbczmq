require File.join(File.dirname(__FILE__), 'helper')

class TestZmqContext < ZmqTestCase
  def test_context
    ctx = ZMQ::Context.new
    assert_instance_of ZMQ::Context, ctx
    assert_raises(ZMQ::Error) do
      ZMQ::Context.new
    end
    assert_equal ctx, ZMQ.context
  ensure
    assert_nil ctx.destroy
  end

  def test_context_with_iothreads
    ctx = ZMQ::Context.new(2)
    assert_instance_of ZMQ::Context, ctx
    assert_raises(ZMQ::Error) do
      ZMQ::Context.new
    end
    assert_equal ctx, ZMQ.context
  ensure
    assert_nil ctx.destroy
  end

  def test_iothreads
    ctx = ZMQ::Context.new
    assert_raises TypeError do
      ctx.iothreads = :invalid  
    end
    ctx.iothreads = 2
  ensure
    ctx.destroy
  end

  def test_linger
    ctx = ZMQ::Context.new
    assert_raises TypeError do
      ctx.linger = :invalid  
    end
    ctx.linger = 10
  ensure
    ctx.destroy
  end

  def test_bind_connect
    ctx = ZMQ::Context.new
    rep = ctx.bind(:REP, "inproc://test.bind_connect")
    req = ctx.connect(:REQ, "inproc://test.bind_connect")
    req.send('success')
    assert_equal 'success', rep.recv
  ensure
    ctx.destroy
  end

  def test_socket
    ctx = ZMQ::Context.new
    assert_raises TypeError do
      ctx.socket("invalid")
    end
    socket = ctx.socket(ZMQ::REP)
    assert_instance_of ZMQ::Socket::Rep, socket
    assert_nil socket.close
    socket = ctx.socket(:REP)
    assert_instance_of ZMQ::Socket::Rep, socket
    assert_nil socket.close
  ensure
    ctx.destroy
  end
end