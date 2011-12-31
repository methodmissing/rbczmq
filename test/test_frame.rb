# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmqFrame < ZmqTestCase
  def test_alloc
    frame = ZMQ::Frame("message")
    assert_instance_of ZMQ::Frame, frame
    assert_nil frame.destroy
  end

  def test_destroyed_frame
    frame = ZMQ::Frame("message")
    frame.destroy
    assert_raises ZMQ::Error do
      frame.data
    end
  end

  def test_alloc_empty
    frame = ZMQ::Frame.new
    assert_equal 0, frame.size
    frame.destroy
  end

  def test_size
    assert_equal 7, ZMQ::Frame("message").size
  end

  def test_data
    frame =  ZMQ::Frame("message")
    assert_equal "message", frame.data
    assert_equal "message", frame.to_s
    assert_equal "message", frame.to_str
  end

  def test_dup
    frame =  ZMQ::Frame("message")
    dup_frame = frame.dup
    assert_not_equal dup_frame.object_id, frame.object_id
    assert_equal frame.data, dup_frame.data
  end

  def test_strhex
    frame =  ZMQ::Frame("message")
    assert_equal "6D657373616765", frame.strhex
  end

  def test_data_equals
    frame =  ZMQ::Frame("message")
    assert frame.data_equals?("message")
    assert !frame.data_equals?("msg")
  end

  def test_equals
    frame =  ZMQ::Frame("message")
    assert_equal frame, ZMQ::Frame("message")
    assert_not_equal frame, ZMQ::Frame("msg")
  end

  def test_compare
    frame =  ZMQ::Frame("message")
    assert frame > ZMQ::Frame("msg")
    assert ZMQ::Frame("msg") < ZMQ::Frame("message")
  end

  def test_print
    frame =  ZMQ::Frame("message")
    assert_nil frame.print
    assert_nil frame.print("prefix")
  end

  def test_reset
    frame =  ZMQ::Frame("message")
    frame.reset("msg")
    assert_equal "msg", frame.data
  end
end