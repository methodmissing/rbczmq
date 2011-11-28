ctx = ZMQ::Context.new
pair = ctx.socket(:PAIR);
pair.bind(Runner::REMOTE_ENDPOINT);

msg = Runner.payload

start_time = Time.now
Runner.msg_count.times do
  case Runner.encoding
  when :string
    pair.send(msg)
  when :frame
    pair.send_frame(ZMQ::Frame(msg))
  when :message
    m =  ZMQ::Message.new
    m.pushstr "header"
    m.pushstr msg
    m.pushstr "body"
    pair.send_message(m)
  end
end

puts "Sent #{Runner.msg_count} messages in %ss ..." % (Time.now - start_time)