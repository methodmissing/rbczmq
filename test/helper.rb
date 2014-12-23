# encoding: utf-8

require 'zmq'
require 'minitest/autorun'
require 'stringio'

Thread.abort_on_exception = true

class ZmqTestCase < Minitest::Test
  if ENV['STRESS_GC']
    def setup
      GC.stress = true
    end

    def teardown
      GC.stress = false
    end
  end
end