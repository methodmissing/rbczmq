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

  CLEAN.include "#{ext.lib_dir}/*.#{RbConfig::CONFIG['DLEXT']}"
end

task :clobber_zeromq do
  Dir.chdir "ext/zeromq" do
    sh "make clean"
  end
end

task :build_zeromq do
  Dir.chdir "ext/zeromq" do
    sh "./configure && make"
  end unless File.exist?("ext/zeromq/src/.libs/libzmq.#{RbConfig::CONFIG['LIBEXT']}")
end

task :clobber_czmq do
  Dir.chdir "ext/czmq" do
    sh "make clean"
  end
end

task :build_czmq do
  Dir.chdir "ext/czmq" do
    libzmq = File.join(File.dirname(__FILE__), 'ext', 'zeromq')
    sh "./configure --with-libzmq=#{libzmq} && make"
  end unless File.exist?("ext/czmq/src/.libs/libczmq.#{RbConfig::CONFIG['LIBEXT']}")
end

RDOC_FILES = FileList["README.rdoc", "ext/rbczmq/rbczmq_ext.c", "lib/zmq.rb"]

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
task :test => :compile

task :default => :test
