# encoding: utf-8

ctx = ZMQ::Context.new
sub = ctx.socket(:SUB)
sub.subscribe("")
sub.connect($runner.endpoint)

messages, start_time = 0, nil
while (case $runner.encoding
  when :string
    sub.recv
  when :frame
    sub.recv_frame
  when :message
    sub.recv_message
  end) do
  start_time ||= Time.now
  messages += 1
  break if messages == $runner.msg_count
end

$runner.stats(start_time)