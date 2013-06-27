# encoding: utf-8

require File.expand_path('../lib/zmq/version', __FILE__)

Gem::Specification.new do |s|
  s.name = "rbczmq"
  s.version = ZMQ::VERSION
  s.summary = "Ruby extension for CZMQ - High-level C Binding for ØMQ (http://czmq.zeromq.org)"
  s.description = "Ruby extension for CZMQ - High-level C Binding for ØMQ (http://czmq.zeromq.org)"
  s.authors = ["Lourens Naudé", "James Tucker", "Matt Connolly"]
  s.email = ["lourens@methodmissing.com", "jftucker@gmail.com", "matt.connolly@me.com"]
  s.homepage = "http://github.com/methodmissing/rbczmq"
  s.date = Time.now.utc.strftime('%Y-%m-%d')
  s.platform = Gem::Platform::RUBY
  s.extensions = "ext/rbczmq/extconf.rb"
  s.has_rdoc = true
  s.files = `git ls-files`.split("\n")
  s.test_files = `git ls-files test`.split("\n")
  s.rdoc_options = ["--charset=UTF-8"]
  s.require_paths = ["lib"]
  s.add_development_dependency('rake-compiler', '~> 0.8.0')

  # get an array of submodule dirs by executing 'pwd' inside each submodule
  gem_dir = File.expand_path(File.dirname(__FILE__)) + "/"
  `git submodule --quiet foreach pwd`.split($\).each do |submodule_path|
    Dir.chdir(submodule_path) do
      submodule_relative_path = submodule_path.sub gem_dir, ""
      # issue git ls-files in submodule's directory and
      # prepend the submodule path to create absolute file paths
      `git ls-files`.split($\).map do |filename|
        s.files << "#{submodule_relative_path}/#{filename}"
      end
    end
  end
end