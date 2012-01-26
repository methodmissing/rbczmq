# encoding: utf-8

require File.join(File.dirname(__FILE__), 'helper')

class TestZmq < ZmqTestCase
  def test_interrupted_p
    assert !ZMQ.interrupted?
  end

  def test_version
    assert_equal [2,1,11], ZMQ.version
  end

  def test_now
    assert_instance_of Fixnum, ZMQ.now
  end

  def test_log
    assert_nil ZMQ.log("log message")
  end

  def test_error
    expected = [ZMQ::Error, NilClass]
    assert expected.any?{|c| c === ZMQ.error }
  end
end