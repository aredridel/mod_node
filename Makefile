WAF=node-waf

all: mod_node

mod_node: mod_node.cc mod_node.h ApacheProcess.cc ApacheProcess.h ApacheRequest.cc ApacheRequest.h ApacheServer.cc ApacheServer.h build/c4che/build.config.py
	${WAF} build

build/c4che/build.config.py:
	${WAF} configure

start: mod_node conf/example.conf
	env NODE_PATH=.:`pwd`/build/default/ httpd -d `pwd` -f conf/example.conf -k start

stop:
	env NODE_PATH=.:`pwd`/build/default/ httpd -d `pwd` -f conf/example.conf -k stop

debug: mod_node conf/example.conf
	env NODE_PATH=.:`pwd`/build/default/ gdb --args httpd -d `pwd` -f conf/example.conf -k start -DNO_DETACH -DFOREGROUND -DDEBUG -DONE_PROCESS

conf/example.conf: conf/example.conf.in
	cat $< | sed -e 's/%SOEXT%/so/' > $@

clean:
	${WAF} clean
