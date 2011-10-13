var binding = require('apache_binding')
var events = require('events')
var util = require('util')

module.exports = exports = binding

exports.ApacheServer = function ApacheServer() {
    events.EventEmitter.apply(this)
}

util.inherits(exports.ApacheServer, events.EventEmitter)

var server = new exports.ApacheServer()

exports.process.onrequest = function(req) {
    server.emit('request', req)
}

exports.createServer = function createServer(requestHandler) {
    server.on('request', requestHandler)
    return server
}
