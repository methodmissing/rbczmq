# encoding: utf-8

require File.expand_path('../lib/zmq/version', __FILE__)

Gem::Specification.new do |s|
  s.name = "rbczmq"
  s.version = ZMQ::VERSION
  s.summary = "Ruby extension for CZMQ - High-level C Binding for ØMQ (http://czmq.zeromq.org)"
  s.description = "Ruby extension for CZMQ - High-level C Binding for ØMQ (http://czmq.zeromq.org)"
  s.authors = ["Lourens Naudé"]
  s.email = ["lourens@methodmissing.com"]
  s.homepage = "http://github.com/methodmissing/rbczmq"
  s.date = Time.now.utc.strftime('%Y-%m-%d')
  s.platform = Gem::Platform::RUBY
  s.extensions = "ext/rbczmq/extconf.rb"
  s.has_rdoc = true
  s.files = `git ls-files`.split("\n")
  s.test_files = `git ls-files test`.split("\n")
  s.rdoc_options = ["--charset=UTF-8"]
  s.require_paths = ["lib"]
  s.add_dependency('rake-compiler', '~> 0.8.0')
end