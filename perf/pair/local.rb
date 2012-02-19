# encoding: utf-8

ctx = ZMQ::Context.new
pair = ctx.socket(:PAIR)
sleep 2
pair.connect($runner.endpoint)

messages, start_time = 0, nil
while (case $runner.encoding
  when :string
    pair.recv
  when :frame
    pair.recv_frame
  when :message
    pair.recv_message
  end) do
  start_time ||= Time.now
  messages += 1
  break if messages == $runner.msg_count
end

$runner.stats(start_time)