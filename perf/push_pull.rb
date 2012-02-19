# encoding: utf-8

$:.unshift('.')
require File.join(File.dirname(__FILE__), 'runner')

$runner = ProcessRunner.new(ENV["MSG_COUNT"], ENV["MSG_SIZE"], ENV["MSG_ENCODING"], ENV["PROCESSES"])
$runner.start(:push_pull)