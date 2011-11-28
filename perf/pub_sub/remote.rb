ctx = ZMQ::Context.new
pub = ctx.socket(:PUB);
pub.bind(Runner::REMOTE_ENDPOINT);

msg = Runner.payload

Runner.msg_count.times do
  case Runner.encoding
  when :string
    pub.send(msg)
  when :frame
    pub.send_frame(ZMQ::Frame(msg))
  when :message
    m =  ZMQ::Message.new
    m.pushstr "header"
    m.pushstr msg
    m.pushstr "body"
    pub.send_message(m)
  end
end

puts "Sent #{Runner.msg_count} messages ..."