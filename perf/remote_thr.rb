$:.unshift('.')
$:.unshift(File.expand_path(File.dirname(__FILE__)) + '/../lib')
require 'zmq'

if ARGV.length != 3
  puts "usage: remote_thr <connect-to> <message-size> <message-count>"
  Process.exit
end
    
connect_to = ARGV[0]
message_size = ARGV[1].to_i
message_count = ARGV[2].to_i

ctx = ZMQ::Context.new
s = ctx.socket(ZMQ::PUB);

s.connect(connect_to);

msg = "#{'0'*message_size}"

message_count.times do
  s.send(msg)
end