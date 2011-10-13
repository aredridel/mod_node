var apache = require('apache');
var fs = require('fs');
var util = require('util');

apache.process.log(apache.APLOG_EMERG, "APLOG_EMERG");
apache.process.log(apache.APLOG_ALERT, "APLOG_ALERT");
apache.process.log(apache.APLOG_CRIT, "APLOG_CRIT");
apache.process.log(apache.APLOG_ERR, "APLOG_ERR");
apache.process.log(apache.APLOG_WARNING, "APLOG_WARNING");
apache.process.log(apache.APLOG_INFO, "APLOG_INFO");
apache.process.log(apache.APLOG_NOTICE, "APLOG_NOTICE");
apache.process.log(apache.APLOG_DEBUG, "APLOG_DEBUG");

/*
apache.process.on("connection", function(req) {
	apache.process.warn("Got a connection")
	apache.process.write("Hi mom!");
})

process.on('exit', function() {
	apache.process.warn('shutting down node')
})

*/

apache.process.onrequest = function(req) {
    apache.process.log(apache.APLOG_NOTICE, process.version)
    req.write('Boo')
    req.end()
    apache.process.log(apache.APLOG_NOTICE, req.headers_in.Host)
    console.log("req", req)
    console.log("this", this)
    console.log("args", arguments)
}
