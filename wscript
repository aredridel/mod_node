import  subprocess
srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cc')
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cc')
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  conf.check_cfg(path='apr-1-config', args='--includes', package='', uselib_store='APR')
  conf.check_cfg(path='apu-1-config', args='--includes', package='', uselib_store='APU')
  conf.env.CPPPATH_HTTPD = [subprocess.check_output(['apxs', '-q', 'INCLUDEDIR']).strip()]
#  conf.env.CXXFLAGS = subprocess.check_output(['apxs', '-q', 'CFLAGS']).strip().split(' ')
  conf.check(header_name='httpd.h', compile_mode='cc', uselib=['HTTPD', 'APR', 'APU'], mandatory=True)
  conf.check(lib='node')
  conf.check(lib='v8')

def build(bld):
  node_apache = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  node_apache.target = 'apache'
  node_apache.source = 'apache.cc'
  node_apache.uselib = ['HTTPD', 'APU', 'APR']
  mod_node = bld.new_task_gen('cxx', 'shlib')
  mod_node.target = 'mod_node'
  mod_node.source = 'mod_node.cc'
  mod_node.uselib = ['HTTPD', 'APU', 'APR', 'NODE']
