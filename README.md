# Node-Clone

This is a Node.js clone Javascript runtime, built one top of [Duktape JS Engine](https://github.com/svaarala/duktape) and a simple implementation of an event loop. Node-Clone supports a handful of non-blocking IO functions which can yield the result using callbacks. This is by no means a production ready environment and its only purpose is a minimal demonstration of how Node.js runtime works internally.

## Build

to build Node-Clone you need to have CMake Installed:
``` shell
mkdir target
cd target
cmake ..
make
```
the output is the `target/nodec` executable.

## Running

Node-Clone does not provide a REPL interface like Node.js. In order to use Node-Clone you need to provide a .js file as input:

``` shell
./nodec index.js
```

## Samples

There are sample scripts provided which can be run using Node-Clone.