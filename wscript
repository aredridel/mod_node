import  subprocess
srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cc')
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('config_c')
  conf.check_tool('compiler_cc')
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  conf.link_add_flags()
  conf.check_cfg(path='apu-1-config', args='--includes', package='', uselib_store='APU')
  conf.check_cfg(path='apu-1-config', args='--link-ld --libs', package='', uselib_store='APU')
  conf.check_cfg(path='apr-1-config', args='--includes', package='', uselib_store='APR')
  conf.check_cfg(path='apr-1-config', args='--link-ld --libs', package='', uselib_store='APR')
  conf.env.CPPPATH_HTTPD = [subprocess.Popen(['apxs', '-q', 'INCLUDEDIR'], stdout=subprocess.PIPE).communicate()[0].strip()]
  conf.env.CXXFLAGS_HTTPD = subprocess.Popen(['apxs', '-q', 'CFLAGS'], stdout=subprocess.PIPE).communicate()[0].strip().split(' ')
  ldflags=['-Wl,-undefined','-Wl,dynamic_lookup']
  conf.check(fragment='int main(void) { return 0; }\n',
             execute=0,
             ldflags=ldflags,
             msg="Checking linker accepts %s" % ldflags,
             define_name='USE_UNDEFINED_DYNAMIC')
  conf.check(header_name='httpd.h', compile_mode='cc', uselib=['HTTPD', 'APR', 'APU'], mandatory=True)
  conf.check(lib='node')
  conf.check(lib='v8')

def build(bld):
  node_apache = bld.new_task_gen('cxx', 'cshlib', 'node_addon')
  node_apache.target = 'apache'
  node_apache.source = 'apache.cc'
  node_apache.uselib = ['HTTPD', 'APU', 'APR']
  if bld.env.USE_UNDEFINED_DYNAMIC:
    node_apache.linkflags = ['-Wl,-undefined', '-Wl,dynamic_lookup']
  mod_node = bld.new_task_gen('cxx', 'cshlib')
  mod_node.target = 'mod_node'
  mod_node.source = 'mod_node.cc'
  mod_node.uselib = ['HTTPD', 'APU', 'APR', 'NODE']
  if bld.env.USE_UNDEFINED_DYNAMIC:
    mod_node.env.append_value('LINKFLAGS', ['-undefined', 'dynamic_lookup'])
