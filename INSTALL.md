Installation
============

Requirements
-----------

* Node.js 0.5.x
* Apache 2 (probably 2.2.x)

Building
-----------

You'll need to build a node.js shared library.

    cd node-0.5.9
    make dynamiclib 

Install the library and headers (headers are installed with a normal make install; the shared library is up to you.)

Then you can build this:

    cd mod_node
    make 

A demo
------

    make start

Open http://localhost:10888/ in a browser and you should see the demo script. Nothing much to look at but it works.
