# encoding: utf-8

require File.join(File.dirname(__FILE__), 'runner')

Runner.start(:pair, ENV["MSG_COUNT"], ENV["MSG_SIZE"], ENV["MSG_ENCODING"])