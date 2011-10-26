# encoding: utf-8

require 'test/unit'
require 'zmq'

class TestZmqLoop < Test::Unit::TestCase
  def test_alloc
    ctx = ZMQ::Context.new
    assert_raises TypeError do
      ZMQ::Loop.new(nil)
    end
    
  ensure
    ctx.destroy
  end

  def test_run_and_stop
    ctx = ZMQ::Context.new
    ZMQ::Loop.run(ctx) do |lp|
      assert !lp.running?
      lp.add_oneshot_timer(0.2) do
        lp.stop
      end
      lp.add_oneshot_timer(0.1) do
        assert lp.running?
      end
    end
  ensure
    ctx.destroy
  end

  def test_callback_stops_event_loop
    ctx = ZMQ::Context.new
    ret = ZMQ::Loop.run(ctx) do |lp|
      lp.add_oneshot_timer(0.2){ false }
    end
    assert_equal(-1, ret)
  ensure
    ctx.destroy
  end

  def test_callback_error_stops_event_loop
    ctx = ZMQ::Context.new
    assert_raises RuntimeError do
      ret = ZMQ::Loop.run(ctx) do |lp|
        lp.add_oneshot_timer(0.2){ raise("stop!") }
      end
      assert_equal(-1, ret)
    end
  ensure
    ctx.destroy
  end

  def test_add_periodic_timer
    ctx = ZMQ::Context.new
    fired = 0
    ZMQ::Loop.run(ctx) do |lp|
      lp.add_periodic_timer(0.1) do 
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
    fired = 0
    ZMQ::Loop.run(ctx) do |lp|
      timer = lp.add_timer(0.1, 5) do
        fired += 1
        fired < 5
      end
      assert_instance_of ZMQ::Timer, timer
    end
    assert_equal 5, fired
  ensure
    ctx.destroy
  end

  def test_cancel_timer
    ctx = ZMQ::Context.new
    fired = 0
    ZMQ::Loop.run(ctx) do |lp|
      timer = lp.add_timer(0.1, 5) do
        fired += 1
        fired < 5
      end
      lp.cancel_timer(timer)
      lp.add_oneshot_timer(0.1){ false }
    end
    assert_equal 0, fired
  ensure
    ctx.destroy
  end

  def test_add_oneshot_timer
    ctx = ZMQ::Context.new
    ret = ZMQ::Loop.run(ctx) do |lp|
      lp.add_oneshot_timer(0.2){ false }
    end
    assert_equal(-1, ret)
  ensure
    ctx.destroy
  end

  def test_run_in_thread
    ctx = ZMQ::Context.new
    t = Thread.new do
      ZMQ::Loop.run(ctx) do |lp|
        lp.add_oneshot_timer(0.2){ false }
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

  def test_register_socket
    ctx = ZMQ::Context.new
    ret = ZMQ::Loop.run(ctx) do |lp|
      s1 = ctx.socket(:PAIR)
      s2 = ctx.socket(:PAIR)
      s3 = ctx.socket(:PAIR)
      lp.bind(s1, "inproc://test_register_socket", LoopBreaker)
      lp.connect(s2, "inproc://test_register_socket")
      s2.send("message")
      assert_raises ZMQ::Error do
        lp.register_socket(s3, ZMQ::POLLIN)
      end
    end
    assert_equal(-1, ret)
  ensure
    ctx.destroy
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
      ret = ZMQ::Loop.run(ctx) do |lp|
        s1 = ctx.socket(:PAIR)
        s2 = ctx.socket(:PAIR)
        lp.bind(s1, "inproc://test_raise_from_socket_callback", FailHandler)
        lp.connect(s2, "inproc://test_raise_from_socket_callback")
        s2.send("message")
      end
      assert_equal(-1, ret)
    end
  ensure
    ctx.destroy
  end

  def test_raise_on_invalid_handler
    ctx = ZMQ::Context.new
    assert_raises RuntimeError do
      ret = ZMQ::Loop.run(ctx) do |lp|
        s1 = ctx.socket(:PAIR)
        s2 = ctx.socket(:PAIR)
        lp.bind(s1, "inproc://test_raise_on_invalid_handler", FailHandler)
        lp.connect(s2, "inproc://test_raise_on_invalid_handler")
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
      lp = ZMQ::Loop.new(ctx)
      lp.stop
    end
  ensure
    ctx.destroy
  end
end