# encoding: utf-8

require File.expand_path("../../helper.rb", __FILE__)

class TestDealerSocket < ZmqTestCase
  def test_behavior
    ctx = ZMQ::Context.new
    sock = ctx.socket(:DEALER)
    assert_equal ZMQ::DEALER, sock.type
    assert_equal "DEALER socket", sock.to_s
  ensure
    ctx.destroy
  end
end