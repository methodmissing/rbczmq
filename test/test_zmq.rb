# encoding: utf-8

require 'test/unit'
require 'zmq'

class TestZmq < Test::Unit::TestCase
  def test_interrupted_p
    assert !ZMQ.interrupted?
  end

  def test_version
    assert_equal [2,1,9], ZMQ.version
  end

  def test_now
    assert_instance_of Fixnum, ZMQ.now
  end

  def test_log
    assert_nil ZMQ.log("log message")
  end

  def test_error
    assert_instance_of String, ZMQ.error
  end
end