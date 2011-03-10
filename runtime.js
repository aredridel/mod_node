var apache = require('apache');
var fs = require('fs');
var util = require('util');

apache.process.critical("critical");
apache.process.warn("warn");
apache.process.log("info");

apache.process.on("connection", function(req) {
	console.log("Ow!");
	apache.process.warn("Got a connection")
	apache.process.write("Hi mom!");
})

process.on('exit', function() {
	apache.process.warn('shutting down node')
})
