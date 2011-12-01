# encoding: utf-8

require 'test/unit'
require 'zmq'
require 'stringio'
require 'socket'

class ZmqTestCase < Test::Unit::TestCase
  if true#ENV['STRESS_GC']
    def setup
      GC.stress = true
    end

    def teardown
      GC.stress = false
    end
  end
end