#!/bin/bash
# Set the source file and output executable names
SOURCE_FILE="example_write_onnx.cpp"
OUTPUT_FILE="example_write_onnx"

# Set the include directories for ONNX and protobuf
INCLUDE_DIRS="-I.. -I/home/jadmin/kaalam.github/Jazz/server"

# Set the library directories for ONNX and protobuf
LIB_DIRS="-L/path/to/protobuf/lib"

# Set the libraries to link against
LIBS="-lprotobuf"

# Compile the source file with the ONNX and protobuf files
g++ $SOURCE_FILE ../onnx.pb.cc $INCLUDE_DIRS $LIB_DIRS $LIBS -o $OUTPUT_FILE

# Check if the compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Executable created: $OUTPUT_FILE"
else
    echo "Compilation failed."
fi
