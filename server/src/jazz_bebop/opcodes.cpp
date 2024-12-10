/* Jazz (c) 2018-2024 kaalam.ai (The Authors of Jazz), using (under the same license):

	1. Biomodelling - The AATBlockQueue class (c) Jacques Basaldúa, 2009-2012 licensed
	  exclusively for the use in the Jazz server software.

	  Copyright 2009-2012 Jacques Basaldúa

	2. BBVA - Jazz: A lightweight analytical web server for data-driven applications.

      Copyright 2016-2017 Banco Bilbao Vizcaya Argentaria, S.A.

      This product includes software developed at

      BBVA (https://www.bbva.com/)

	3. LMDB, Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.

	  Licensed under http://www.OpenLDAP.org/license.html


	  Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	  http://www.apache.org/licenses/LICENSE-2.0

	  Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/


#include "src/jazz_bebop/opcodes.h"


namespace jazz_bebop
{

/** \brief A table to locate all the possible ONNX tensor types by their name in the config file "onnx.ini".

	The table is a map of strings to a TensorType: Jazz native type, ONNX protocol buffer type and ONNX runtime type.
*/
const TensorTypeDict TENSOR_TYPES = {

	{(pChar) "bool",	{CELL_TYPE_BYTE_BOOLEAN, onnx::TensorProto::BOOL,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL}},
	{(pChar) "double",	{CELL_TYPE_DOUBLE,		 onnx::TensorProto::DOUBLE,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE}},
	{(pChar) "float",	{CELL_TYPE_SINGLE,		 onnx::TensorProto::FLOAT,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT}},
	{(pChar) "string",	{CELL_TYPE_STRING,		 onnx::TensorProto::STRING,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING}},

	{(pChar) "bfloat16",{CELL_TYPE_BFLOAT16,	 onnx::TensorProto::BFLOAT16, ONNX_TENSOR_ELEMENT_DATA_TYPE_BFLOAT16}},
	{(pChar) "float16",	{CELL_TYPE_FLOAT16,		 onnx::TensorProto::FLOAT16,  ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16}},

	{(pChar) "int16",	{CELL_TYPE_INT16,		 onnx::TensorProto::INT16,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16}},
	{(pChar) "int32",	{CELL_TYPE_INTEGER,		 onnx::TensorProto::INT32,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32}},
	{(pChar) "int64",	{CELL_TYPE_LONG_INTEGER, onnx::TensorProto::INT64,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64}},
	{(pChar) "int8",	{CELL_TYPE_INT8,		 onnx::TensorProto::INT8,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8}},

	{(pChar) "uint16",	{CELL_TYPE_UINT16,		 onnx::TensorProto::UINT16,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16}},
	{(pChar) "uint32",	{CELL_TYPE_UINT32,		 onnx::TensorProto::UINT32,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32}},
	{(pChar) "uint64",	{CELL_TYPE_UINT64,		 onnx::TensorProto::UINT64,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64}},
	{(pChar) "uint8",	{CELL_TYPE_BYTE,		 onnx::TensorProto::UINT8,	  ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8}}
};


/** \brief A table to locate all the possible ONNX attribute types by their name in the config file "onnx.ini".

	The table is a map of strings to an AttributeType: Jazz native type, ONNX protobuf type and a boolean if the attribute is a list.
*/
const AttributeTypeDict ATTRIBUTE_TYPES = {

	{(pChar) "float",	{CELL_TYPE_SINGLE,		 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_FLOAT, false}},
	{(pChar) "floats",	{CELL_TYPE_SINGLE,		 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_FLOATS, true}},
	{(pChar) "int",		{CELL_TYPE_LONG_INTEGER, onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_INT, false}},
	{(pChar) "ints",	{CELL_TYPE_LONG_INTEGER, onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_INTS, true}},
	{(pChar) "string",	{CELL_TYPE_STRING,		 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_STRING, false}},
	{(pChar) "strings",	{CELL_TYPE_STRING,		 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_STRING, true}},

	{(pChar) "graph",	{CELL_TYPE_ONNX_GRAPH,	 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_GRAPH, false}},
	{(pChar) "tensor",	{CELL_TYPE_ONNX_TENSOR,	 onnx::AttributeProto::AttributeType::AttributeProto_AttributeType_TENSOR, false}}
};

/*	-----------------------------------------------
	 OpCodes : I m p l e m e n t a t i o n
--------------------------------------------------- */

/** \brief Bop: Start the OpCodes.

	\param a_logger		A pointer to the logger.
	\param a_config		A pointer to the configuration.
*/
OpCodes::OpCodes(pLogger a_logger, pConfigFile a_config) : Service(a_logger, a_config) {}


StatusCode OpCodes::start() {
	if (!get_conf_key("ONNX_IR_VERSION", ir_version)) {
		log(LOG_ERROR, "Config key ONNX_IR_VERSION not valid in OpCodes::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	std::string s;

	if (!get_conf_key("ONNX_OPCODE_DEFS_FN", s)) {
		log(LOG_ERROR, "Config key ONNX_OPCODE_DEFS_FN not found in OpCodes::start");

		return SERVICE_ERROR_BAD_CONFIG;
	}

	if (!onnx_conf.load_config(s.c_str())) {
		log_printf(LOG_ERROR, "Failed reading configuration file '%s' in OpCodes::start", s.c_str());

		return SERVICE_ERROR_IO_ERROR;
	}

	return SERVICE_NO_ERROR;
}


StatusCode OpCodes::shut_down() {
	return SERVICE_NO_ERROR;
}


/** \brief Get the latest opset version.

	\return The latest opset version.
*/
int OpCodes::latest_opset_version() {
	return 0;
}


/** \brief Set the opset version.

	\param version	The version to set.

	\return True if the version was set, false otherwise.
*/
bool OpCodes::set_opset_version(int version) {
	return false;
}

} // namespace jazz_bebop

#if defined CATCH_TEST
#include "src/jazz_bebop/tests/test_opcodes.ctest"
#endif
