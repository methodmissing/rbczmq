ctx = ZMQ::Context.new
pull = ctx.socket(:PULL)
pull.connect(Runner::REMOTE_ENDPOINT)

messages, start_time = 0, nil
while (case Runner.encoding
  when :string
    pull.recv
  when :frame
    pull.recv_frame
  when :message
    pull.recv_message
  end) do
  start_time ||= Time.now
  messages += 1
  break if messages == Runner.process_msg_count
end

Runner.stats(start_time)