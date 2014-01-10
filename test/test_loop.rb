# encoding: utf-8

require File.expand_path("../helper.rb", __FILE__)

class TestZmqLoop < ZmqTestCase
  def test_alloc
    lp = ZMQ::Loop.new
    assert_instance_of ZMQ::Loop, lp
  end

  def test_destroyed
    lp = ZMQ::Loop.new
    lp.destroy
    assert_raises ZMQ::Error do
      lp.start
    end
  end

  def test_run_and_stop
    ctx = ZMQ::Context.new
    ZMQ::Loop.run do
      assert !ZL.running?
      ZL.add_oneshot_timer(0.2) do
        ZL.stop
      end
      ZL.add_oneshot_timer(0.1) do
        assert ZL.running?
      end
    end
  ensure
    ctx.destroy
  end

  def test_callback_stops_event_loop
    ctx = ZMQ::Context.new
    ret = ZMQ::Loop.run do
      ZL.add_oneshot_timer(0.2){ false }
    end
    assert_equal(-1, ret)
  ensure
    ctx.destroy
  end

  def test_callback_error_stops_event_loop
    ctx = ZMQ::Context.new
    assert_raises RuntimeError do
      ret = ZMQ::Loop.run do
        ZL.add_oneshot_timer(0.2){ raise("stop!") }
      end
      assert_equal(-1, ret)
    end
  ensure
    ctx.destroy
  end

  def test_add_periodic_timer
    ctx = ZMQ::Context.new
    fired = 0
    ZMQ::Loop.run do
      ZL.add_periodic_timer(0.1) do 
        fired += 1
        fired < 3
      end
    end
    assert_equal 3, fired
  ensure
    ctx.destroy
  end

  def test_add_timer
    ctx = ZMQ::Context.new
    fired, timer = 0, nil
    ZMQ::Loop.run do
      timer = ZL.add_timer(0.1, 5) do
        fired += 1
        fired < 5
      end
    end
    assert_instance_of ZMQ::Timer, timer
    assert_equal 5, fired
  ensure
    ctx.destroy
  end

  def test_cancel_timer
    ctx = ZMQ::Context.new
    fired = 0
    ZMQ::Loop.run do
      timer = ZL.add_timer(0.1, 5) do
        fired += 1
        fired < 5
      end
      assert ZL.cancel_timer(timer)
      ZL.add_oneshot_timer(0.1){ false }
    end
    assert_equal 0, fired
  ensure
    ctx.destroy
  end

  def test_cancel_timer_instance
    ctx = ZMQ::Context.new
    fired = 0
    timer = ZMQ::Timer.new(0.1, 2){ fired += 1 }
    ZMQ::Loop.run do
      ZL.register_timer(timer)
      ZL.add_oneshot_timer(0.3){ false }
      timer.cancel
    end
    assert_equal 0, fired
  ensure
    ctx.destroy
  end

  def test_add_oneshot_timer
    ctx = ZMQ::Context.new
    ret = ZMQ::Loop.run do
      ZL.add_oneshot_timer(0.2){ false }
    end
    assert_equal(-1, ret)
  ensure
    ctx.destroy
  end

  def test_run_in_thread
    ctx = ZMQ::Context.new
    t = Thread.new do
      ZMQ::Loop.run do
        ZL.add_oneshot_timer(0.2){ false }
      end
      :done
    end
    assert_equal :done, t.value
  ensure
    ctx.destroy
  end

  class LoopBreaker < ZMQ::Handler
    def on_readable
      p :on_readable
      false
    end

    def on_writable
    end
  end

  def test_register_sockets
    ctx = ZMQ::Context.new
    ret = ZMQ::Loop.run do
      s1 = ctx.socket(:PAIR)
      s2 = ctx.socket(:PAIR)
      s3 = ctx.socket(:PAIR)
      ZL.bind(s1, "inproc://test.loop-register", LoopBreaker)
      ZL.connect(s2, "inproc://test.loop-register")
      s2.send("message")
      assert_raises ZMQ::Error do
        ZL.register(s3)
      end
    end
    assert_equal(-1, ret)
  ensure
    ctx.destroy
  end

  def test_register_ios
    r, w = IO.pipe
    ret = ZMQ::Loop.run do
      ZL.register_readable(r, LoopBreaker)
      ZL.register_writable(w)
      w.write("message")
    end
    assert_equal(-1, ret)
  end

  def test_register_ruby_sockets
    server = TCPServer.new("127.0.0.1", 0)
    port = server.addr[1]
    client = TCPSocket.new("127.0.0.1", port)
    s = server.accept
    ret = ZMQ::Loop.run do
      ZL.register_readable(s, LoopBreaker)
      ZL.register_writable(client)
      client.send("message", 0)
    end
    assert_equal(-1, ret)
  end

  class FailHandler < ZMQ::Handler
    def on_readable
      p :on_readable
      raise "fail"
    end

    def on_writable
    end
  end

  def test_raise_from_socket_callback
    ctx = ZMQ::Context.new
    assert_raises RuntimeError do
      ret = ZMQ::Loop.run do
        s1 = ctx.socket(:PAIR)
        s2 = ctx.socket(:PAIR)
        ZL.bind(s1, "inproc://test.loop-raise_from_socket_callback", FailHandler)
        ZL.connect(s2, "inproc://test.loop-raise_from_socket_callback")
        s2.send("message")
      end
      assert_equal(-1, ret)
    end
  ensure
    ctx.destroy
  end

  def test_raise_from_io_callback
    assert_raises RuntimeError do
      ret = ZMQ::Loop.run do
        r, w = IO.pipe
        ZL.register_readable(r, FailHandler)
        ZL.register_writable(w)
        w.write("message")
      end
      assert_equal(-1, ret)
    end
  end

  def test_raise_on_invalid_handler
    ctx = ZMQ::Context.new
    assert_raises RuntimeError do
      ret = ZMQ::Loop.run do
        s1 = ctx.socket(:PAIR)
        s2 = ctx.socket(:PAIR)
        ZL.bind(s1, "inproc://test.loop-raise_on_invalid_handler", FailHandler)
        ZL.connect(s2, "inproc://test.loop-raise_on_invalid_handler")
        s2.send("message")
      end
      assert_equal(-1, ret)
    end
  ensure
    ctx.destroy
  end

  def test_double_stop
    ctx = ZMQ::Context.new
    assert_raises ZMQ::Error do
      lp = ZMQ::Loop.new
      lp.stop
    end
  ensure
    ctx.destroy
  end
end
