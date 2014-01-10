# encoding: utf-8

require File.expand_path("../helper.rb", __FILE__)

class TestZmqHandler < ZmqTestCase
  def test_expects_pollitem
    assert_raises TypeError do
      ZMQ::Handler.new(:socket)
    end
  end

  def test_readable_writable_contracts
    handler = ZMQ::Handler.new(ZMQ::Pollitem(STDIN))
    assert_raises NotImplementedError do
      handler.on_readable
    end
    assert_raises NotImplementedError do
      handler.on_writable
    end
  end

  def test_error
    handler = ZMQ::Handler.new(ZMQ::Pollitem(STDIN))
    assert_raises StandardError do
      handler.on_error(StandardError.new)
    end
  end
end
