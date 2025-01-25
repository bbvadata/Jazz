# Version of the ONNX/Protobuf Toolchain

## 1. Protocol Buffers Compiler

```bash
sudo apt  install protobuf-compiler
protoc --version
```
  -> `libprotoc 3.21.12`

## 2. onnx.proto

We download from the main branch (Nov, 24). Apparently, this is the canonical way despite having branches each version. The same
version has minor differences in the branch (compared to main) including `// IR VERSION 10 published on TBD` becomes
`// IR VERSION 10 published on March 25, 2024` in the main branch.

```bash
curl -o onnx.proto https://raw.githubusercontent.com/onnx/onnx/main/onnx/onnx.proto
```
  -> `IR VERSION 10 published on March 25, 2024`

That is the highest version. The spec is compatible with the previous versions. The next version is not complete and shows as
`IR VERSION 11 published on TBD`.

## 3. onnx.pb.h and onnx.pb.cc

```bash
protoc --cpp_out=. onnx.proto
```
