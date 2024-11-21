#include <iostream>
#include <fstream>
#include "src/onnx_proto/onnx.pb.h"

int main() {
    // Initialize the ONNX model
    onnx::ModelProto model;
    model.set_ir_version(onnx::IR_VERSION);
    model.set_producer_name("example_producer");

    // Add opset version
    onnx::OperatorSetIdProto* opset = model.add_opset_import();
    opset->set_domain("");  // Use the default domain
    opset->set_version(13); // Set the opset version (e.g., 13)

    // Create a graph
    onnx::GraphProto* graph = model.mutable_graph();
    graph->set_name("example_graph");

    // Add input tensor
    onnx::ValueInfoProto* input = graph->add_input();
    input->set_name("input");
    onnx::TypeProto* input_type = input->mutable_type();
    onnx::TypeProto::Tensor* input_tensor_type = input_type->mutable_tensor_type();
    input_tensor_type->set_elem_type(onnx::TensorProto::FLOAT);
    onnx::TensorShapeProto* input_shape = input_tensor_type->mutable_shape();
    input_shape->add_dim()->set_dim_value(1);
    input_shape->add_dim()->set_dim_value(3);
    input_shape->add_dim()->set_dim_value(224);
    input_shape->add_dim()->set_dim_value(224);

    // Add output tensor
    onnx::ValueInfoProto* output = graph->add_output();
    output->set_name("output");
    onnx::TypeProto* output_type = output->mutable_type();
    onnx::TypeProto::Tensor* output_tensor_type = output_type->mutable_tensor_type();
    output_tensor_type->set_elem_type(onnx::TensorProto::FLOAT);
    onnx::TensorShapeProto* output_shape = output_tensor_type->mutable_shape();
    output_shape->add_dim()->set_dim_value(1);
    output_shape->add_dim()->set_dim_value(1000);

    // Add a node (e.g., a ReLU node)
    onnx::NodeProto* node = graph->add_node();
    node->set_op_type("Relu");
    node->add_input("input");
    node->add_output("output");

    // Serialize the model to a file
    std::ofstream output_file("model.onnx", std::ios::binary);
    if (!model.SerializeToOstream(&output_file)) {
        std::cerr << "Failed to write model to file" << std::endl;
        return -1;
    }

    std::cout << "ONNX model created successfully" << std::endl;
    return 0;
}
