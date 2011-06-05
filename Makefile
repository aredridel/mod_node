WAF = waf

all:
	${WAF} build

start:
	env NODE_PATH=`pwd`/build/default/ httpd -d `pwd` -f conf/example.conf -k start

stop:
	env NODE_PATH=`pwd`/build/default/ httpd -d `pwd` -f conf/example.conf -k stop
