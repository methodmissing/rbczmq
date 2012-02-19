# encoding: utf-8

$:.unshift('.')
require File.join(File.dirname(__FILE__), 'runner')

$runner = ProcessRunner.new(ENV["MSG_COUNT"], ENV["MSG_SIZE"], ENV["MSG_ENCODING"])
$runner.start(:req_rep)