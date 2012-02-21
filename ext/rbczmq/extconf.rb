# encoding: utf-8

require 'mkmf'
require 'pathname'

def sys(cmd, err_msg)
  p cmd
  system(cmd) || fail(err_msg)
end

def fail(msg)
  STDERR.puts msg
  exit(1)
end

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

# XXX fallbacks specific to Darwin for JRuby (does not set these values in RbConfig::CONFIG)
LIBEXT = RbConfig::CONFIG['LIBEXT'] || 'a'
DLEXT = RbConfig::CONFIG['DLEXT'] || 'bundle'

cwd = Pathname(File.expand_path(File.dirname(__FILE__)))
dst_path = cwd + 'dst'
libs_path = dst_path + 'lib'
vendor_path = cwd + '..'
zmq_path = vendor_path + 'zeromq'
czmq_path = vendor_path + 'czmq'
zmq_include_path = zmq_path + 'include'
czmq_include_path = czmq_path + 'include'

# Fail early if we don't meet the following dependencies.

# Present on OS X and BSD systems, package install required on Linux
fail("package uuid-dev required (apt-get install uuid-dev)") unless have_header('uuid/uuid.h')

# Courtesy of EventMachine and @tmm1
def check_libs libs = [], fatal = false
  libs.all? { |lib| have_library(lib) || (abort("could not find library: #{lib}") if fatal) }
end

def check_heads heads = [], fatal = false
  heads.all? { |head| have_header(head) || (abort("could not find header: #{head}") if fatal)}
end

case RUBY_PLATFORM
when /mswin32/, /mingw32/, /bccwin32/
  check_heads(%w[windows.h winsock.h], true)
  check_libs(%w[kernel32 rpcrt4 gdi32], true)

  if GNU_CHAIN
    CONFIG['LDSHARED'] = "$(CXX) -shared -lstdc++"
  else
    $defs.push "-EHs"
    $defs.push "-GR"
  end

when /solaris/
  add_define 'OS_SOLARIS8'

  if CONFIG['CC'] == 'cc' and `cc -flags 2>&1` =~ /Sun/ # detect SUNWspro compiler
    # SUN CHAIN
    add_define 'CC_SUNWspro'
    $preload = ["\nCXX = CC"] # hack a CXX= line into the makefile
    $CFLAGS = CONFIG['CFLAGS'] = "-KPIC"
    CONFIG['CCDLFLAGS'] = "-KPIC"
    CONFIG['LDSHARED'] = "$(CXX) -G -KPIC -lCstd"
  else
    # GNU CHAIN
    # on Unix we need a g++ link, not gcc.
    CONFIG['LDSHARED'] = "$(CXX) -shared"
  end

when /openbsd/
  # OpenBSD branch contributed by Guillaume Sellier.

  # on Unix we need a g++ link, not gcc. On OpenBSD, linking against libstdc++ have to be explicitly done for shared libs
  CONFIG['LDSHARED'] = "$(CXX) -shared -lstdc++ -fPIC"
  CONFIG['LDSHAREDXX'] = "$(CXX) -shared -lstdc++ -fPIC"

when /darwin/
  # on Unix we need a g++ link, not gcc.
  # Ff line contributed by Daniel Harple.
  CONFIG['LDSHARED'] = "$(CXX) " + CONFIG['LDSHARED'].split[1..-1].join(' ')

when /aix/
  CONFIG['LDSHARED'] = "$(CXX) -shared -Wl,-G -Wl,-brtl"

else
  # on Unix we need a g++ link, not gcc.
  CONFIG['LDSHARED'] = "$(CXX) -shared"
end

# extract dependencies
unless File.directory?(zmq_path) && File.directory?(czmq_path)
  fail "The 'tar' (creates and manipulates streaming archive files) utility is required to extract dependencies" if `which tar`.strip.empty?
  Dir.chdir(vendor_path) do
    sys "tar xvzf zeromq.tar.gz", "Could not extract the ZeroMQ archive!"
    sys "tar xvzf czmq.tar.gz", "Could not extract the CZMQ archive!"
  end
end

# build libzmq
lib = libs_path + "libzmq.#{LIBEXT}"
Dir.chdir zmq_path do
  sys "./autogen.sh", "ZeroMQ autogen failed!" unless File.exist?(zmq_path + 'configure')
  sys "./configure --prefix=#{dst_path} --without-documentation --enable-shared && make && make install", "ZeroMQ compile error!"
end unless File.exist?(lib)

# build libczmq
lib = libs_path + "libczmq.#{LIBEXT}"
Dir.chdir czmq_path do
  sys "./autogen.sh", "CZMQ autogen failed!" unless File.exist?(czmq_path + 'configure')
  sys "./configure LDFLAGS=-L#{libs_path} --prefix=#{dst_path} --with-libzmq=#{dst_path} --disable-shared && make all && make install", "CZMQ compile error!"
end unless File.exist?(lib)

dir_config('rbczmq')

have_func('rb_thread_blocking_region')

$INCFLAGS << " -I#{zmq_include_path}" if find_header("zmq.h", zmq_include_path)
$INCFLAGS << " -I#{czmq_include_path}" if find_header("czmq.h", czmq_include_path)

$LIBPATH << libs_path.to_s

# Special case to prevent Rubinius compile from linking system libzmq if present
if defined?(RUBY_ENGINE) && RUBY_ENGINE =~ /rbx/
  CONFIG['LDSHARED'] = "#{CONFIG['LDSHARED']} -Wl,-rpath=#{libs_path.to_s}"
end

fail "Error compiling and linking libzmq" unless have_library("zmq")
fail "Error compiling and linking libczmq" unless have_library("czmq")

$defs << "-pedantic"

$CFLAGS << ' -Wall -funroll-loops'
$CFLAGS << ' -Wextra -O0 -ggdb3' if ENV['DEBUG']

create_makefile('rbczmq_ext')
