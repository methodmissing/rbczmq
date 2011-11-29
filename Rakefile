# encoding: utf-8

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

task :compile => [:build_zeromq, :build_czmq]
task :clobber => [:clobber_zeromq, :clobber_czmq]

Rake::ExtensionTask.new('rbczmq', gemspec) do |ext|
  ext.name = 'rbczmq_ext'
  ext.ext_dir = 'ext/rbczmq'
  ext.lib_dir = File.join('lib', 'zmq')

  CLEAN.include "#{ext.ext_dir}/libzmq.#{RbConfig::CONFIG['LIBEXT']}"
  CLEAN.include "#{ext.ext_dir}/libczmq.#{RbConfig::CONFIG['LIBEXT']}"
  CLEAN.include "#{ext.lib_dir}/*.#{RbConfig::CONFIG['DLEXT']}"
end

task :clobber_zeromq do
  Dir.chdir "ext/zeromq" do
    sh "make clean"
  end
end

task :build_zeromq do
  lib = "ext/zeromq/src/.libs/libzmq.#{RbConfig::CONFIG['LIBEXT']}"
  Dir.chdir "ext/zeromq" do
    sh "./autogen.sh" unless File.exist?("ext/zeromq/configure")
    sh "./configure && make"
  end unless File.exist?(lib)
  cp lib, "ext/rbczmq"
end

task :clobber_czmq do
  Dir.chdir "ext/czmq" do
    sh "make clean"
  end
end

task :build_czmq do
  lib = "ext/czmq/src/.libs/libczmq.#{RbConfig::CONFIG['LIBEXT']}"
  Dir.chdir "ext/czmq" do
    sh "./autogen.sh" unless File.exist?("ext/czmq/configure")
    libzmq = File.join(File.dirname(__FILE__), 'ext', 'zeromq')
    sh "./configure --with-libzmq=#{libzmq} && make all"
  end unless File.exist?(lib)
  cp lib, "ext/rbczmq"
end

RDOC_FILES = FileList["README.rdoc", "ext/rbczmq/*.c", "lib/**/*.rb"]

Rake::RDocTask.new do |rd|
  rd.title = ""
  rd.main = "README.rdoc"
  rd.rdoc_dir = "doc"
  rd.rdoc_files.include(RDOC_FILES)
end

desc 'Run rbczmq tests'
Rake::TestTask.new(:test) do |t|
  t.test_files = Dir.glob("test/test_*.rb")
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
