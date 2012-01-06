# encoding: utf-8

require 'rubygems' unless defined?(Gem)
require 'rake' unless defined?(Rake)

# Prefer compiled Rubinius bytecode in .rbx/
ENV["RBXOPT"] = "-Xrbc.db"

require 'rake/extensiontask'
require 'rake/testtask'
begin
require 'rdoc/task'
rescue LoadError # fallback to older 1.8.7 rubies
require 'rake/rdoctask'
end

gemspec = eval(IO.read('rbczmq.gemspec'))

task :clobber => [:clobber_zeromq, :clobber_czmq]

Gem::PackageTask.new(gemspec) do |pkg|
end

# XXX fallbacks specific to Darwin for JRuby (does not set these values in RbConfig::CONFIG)
LIBEXT = RbConfig::CONFIG['LIBEXT'] || 'a'
DLEXT = RbConfig::CONFIG['DLEXT'] || 'bundle'

Rake::ExtensionTask.new('rbczmq', gemspec) do |ext|
  ext.name = 'rbczmq_ext'
  ext.ext_dir = 'ext/rbczmq'
  ext.lib_dir = File.join('lib', 'zmq')

  CLEAN.include "#{ext.ext_dir}/libzmq.#{LIBEXT}"
  CLEAN.include "#{ext.ext_dir}/libczmq.#{LIBEXT}"
  CLEAN.include "#{ext.lib_dir}/*.#{DLEXT}"
end

task :clobber_zeromq do
  sh "rm -Rf ext/zeromq" if File.directory?("ext/zeromq")
end

task :clobber_czmq do
  sh "rm -Rf ext/czmq" if File.directory?("ext/czmq")
end

Rake::RDocTask.new do |rd|
  files = FileList["README.rdoc", "lib/**/*.rb", "ext/rbczmq/*.c"]
  rd.title = "rbczmq - binding for the high level ZeroMQ C API"
  rd.main = "README.rdoc"
  rd.rdoc_dir = "doc"
  rd.options << "--promiscuous"
  rd.rdoc_files.include(files)
end

desc 'Run rbczmq tests'
Rake::TestTask.new(:test) do |t|
  t.test_files = Dir.glob("test/**/test_*.rb")
  t.verbose = true
  t.warning = true
end

namespace :debug do
  desc "Run the test suite under gdb"
  task :gdb do
    system "gdb --args ruby rake"
  end

  desc "Run the test suite under valgrind"
  task :valgrind do
    valgrind_opts = "--num-callers=5 --error-limit=no --partial-loads-ok=yes --undef-value-errors=no --leak-check=full"
    system %[valgrind #{valgrind_opts} ruby -w -I"lib" -I"/Users/lourens/.rvm/gems/ruby-1.8.7-p352/gems/rake-0.9.2/lib" "/Users/lourens/.rvm/gems/ruby-1.8.7-p352/gems/rake-0.9.2/lib/rake/rake_test_loader.rb" "test/test_context.rb" "test/test_frame.rb" "test/test_loop.rb" "test/test_message.rb" "test/test_socket.rb" "test/test_threading.rb" "test/test_timer.rb" "test/test_zmq.rb"]
  end
end

desc 'Clobber Rubinius .rbc files'
task :clobber_rbc do
  sh 'find . -name *.rbc -print0 | xargs -0 rm'
end

task :test => :compile
task :default => :test
