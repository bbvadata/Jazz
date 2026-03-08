# Creating ONNX Pipelines

Creating ONNX files from scratch involves working with the ONNX model format, which is defined using Protocol Buffers (protobuf).

This folder contains all the fundamental third-party sources required to create ONNX files from scratch.

The content in `onnx_proto/examples` is a simple example of how to compile and write to a file.


## Integration with Jazz server

This is done by running `./config.sh` in the root directory of the project. That produces the necessary makefile to build `opcodes.h`
and `opcodes.cpp` (in `server/src/jazz_bebop`).
