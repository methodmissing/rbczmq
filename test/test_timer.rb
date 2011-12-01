require File.join(File.dirname(__FILE__), 'helper')

class TestZmqTimer < ZmqTestCase

  def test_alloc
    cb = Proc.new{ :done }
    assert_raises ArgumentError do
      ZMQ::Timer.new(1, 2)
    end
    assert_instance_of ZMQ::Timer, ZMQ::Timer.new(1, 2, cb)
    assert_instance_of ZMQ::Timer, ZMQ::Timer.new(1, 2){ :fired }
  end

  def test_fire
    timer = ZMQ::Timer.new(1, 2){ :fired }
    assert_equal :fired, timer.fire
    assert_equal :fired, timer.call
  end

  def test_on_error
    timer = ZMQ::Timer.new(1, 2){ :fired }
    err = StandardError.new("oops")
    assert_raises StandardError do
      timer.on_error(err)
    end
  end

  def test_cancel
    timer = ZMQ::Timer.new(1, 2){ :fired }
    timer.cancel
    assert_raises ZMQ::Error do
      timer.call
    end
  end
end