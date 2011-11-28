$:.unshift(".")
require File.join(File.dirname(__FILE__), 'runner')

Runner.start(:req_rep, ENV["MSG_COUNT"], ENV["MSG_SIZE"], ENV["MSG_ENCODING"])
