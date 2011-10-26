$:.unshift('.')
$:.unshift(File.expand_path(File.dirname(__FILE__)) + '/../lib')
require 'zmq'

if ARGV.length != 3
  puts "usage: local_thr <bind-to> <message-size> <message-count>"
  Process.exit
end

bind_to = ARGV[0]
message_size = ARGV[1].to_i
message_count = ARGV[2].to_i

ctx = ZMQ::Context.new
s = ctx.socket(ZMQ::SUB);
s.bind(bind_to);

messages = 0
while msg = s.recv do
  start_time ||= Time.now
  messages += 1
  break if messages == message_count
end

end_time = Time.now
elapsed = (end_time.to_f - start_time.to_f) * 1000000
elapsed = 1 if elapsed == 0

throughput = message_count * 1000000 / elapsed
megabits = throughput * message_size * 8 / 1000000

puts "message size: %i [B]" % message_size
puts "message count: %i" % message_count
puts "mean throughput: %i [msg/s]" % throughput
puts "mean throughput: %.3f [Mb/s]" % megabits

