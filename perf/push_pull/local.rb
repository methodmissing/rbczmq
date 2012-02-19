# encoding: utf-8

ctx = ZMQ::Context.new
pull = ctx.socket(:PULL)
pull.connect($runner.endpoint)

messages, start_time = 0, nil
while (case $runner.encoding
  when :string
    pull.recv
  when :frame
    pull.recv_frame
  when :message
    pull.recv_message
  end) do
  start_time ||= Time.now
  messages += 1
  break if messages == $runner.process_msg_count
end

$runner.stats(start_time)