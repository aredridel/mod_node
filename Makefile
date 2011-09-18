WAF = waf

all: mod_node

mod_node:
	${WAF} build

start: mod_node conf/example.conf
	env NODE_PATH=`pwd`/build/default/ httpd -d `pwd` -f conf/example.conf -k start

stop:
	env NODE_PATH=`pwd`/build/default/ httpd -d `pwd` -f conf/example.conf -k stop

debug: mod_node conf/example.conf
	env NODE_PATH=`pwd`/build/default/ gdb --args httpd -d `pwd` -f conf/example.conf -k start -DNO_DETACH -DFOREGROUND

conf/example.conf: conf/example.conf.in
	cat $< | sed -e 's/%SOEXT%/dylib/' > $@
