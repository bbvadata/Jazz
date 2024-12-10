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


#include "src/jazz_bebop/data_space.h"
#include "src/onnx_proto/onnx.pb.h"
#include "onnxruntime_c_api.h"

#if defined CATCH_TEST
#ifndef INCLUDED_JAZZ_CATCH2
#define INCLUDED_JAZZ_CATCH2

#include "src/catch2/catch.hpp"

#endif
#endif


#ifndef INCLUDED_JAZZ_BEBOP_OPCODES
#define INCLUDED_JAZZ_BEBOP_OPCODES


/** \brief In Bop-25, opcodes are onnx-runtime operations.

*/
namespace jazz_bebop
{

/** \brief A type definition for tensors across all the technologies: Jazz native, ONNX protocol buffer and ONNX runtime.
*/
class TensorType {

	public:

		/** \brief The constructor for a TensorType.
			\param jazz		The Jazz native type.
			\param proto	The ONNX protocol buffer type.
			\param rt		The ONNX runtime type.
		*/
    	TensorType(int jazz, onnx::TensorProto::DataType proto, ONNXTensorElementDataType rt) {
			jazz_type		= jazz;
			onnx_proto_type = proto;
			onnx_rt_type	= rt;
		}

		int jazz_type;											///< The Jazz native type.
		onnx::TensorProto::DataType onnx_proto_type;			///< The ONNX protocol buffer type.
		ONNXTensorElementDataType onnx_rt_type;					///< The ONNX runtime type.
};
typedef std::map<stdName, TensorType>	TensorTypeDict;			///< A map of TensorType objects.
typedef std::vector<TensorType>			TensorTypes;			///< A list of TensorType objects.


/** \brief A type definition for attributes across Jazz native (with special codes), and ONNX protocol buffer.
*/
class AttributeType {

	public:

		/** \brief The constructor for an AttributeType.
			\param jazz		The Jazz native type.
			\param proto	The ONNX protocol buffer type.
			\param multi	True if the attribute is a list of the type.
		*/
		AttributeType(int jazz, onnx::AttributeProto::AttributeType proto, bool multi) {
			jazz_type		= jazz;
			onnx_proto_type = proto;
			is_multi		= multi;
		}

		int jazz_type;											///< The Jazz native type.
		onnx::AttributeProto::AttributeType onnx_proto_type;	///< The ONNX protocol buffer type.
		bool is_multi = false;									///< True if the attribute is a list of the type.
};
typedef std::map<stdName, AttributeType>	AttributeTypeDict;	///< A map of AttributeType objects.
typedef std::vector<AttributeType>			AttributeTypes;		///< A list of AttributeType objects.


/** \brief A parameter (input or output) for an ONNX OpCode.
*/
class OnnxParameter {

	public:

		/** \brief The constructor for an OnnxParameter.
			\param nam	The name of the parameter.
			\param typ	The types of the parameter.
		*/
		OnnxParameter(stdName nam, TensorTypes typ) {
			name	= nam;
			types	= typ;
		}

		stdName name;											///< The name of the parameter.
		TensorTypes types;										///< The types of the parameter.
};
typedef std::vector<OnnxParameter>			OnnxParameters;		///< A list of OnnxParameter objects.


/** \brief An attribute for an ONNX OpCode.
*/
class OnnxAttribute {

	public:

		/** \brief The constructor for an OnnxAttribute.
			\param nam	The name of the attribute.
			\param typ	The types of the attribute.
		*/
		OnnxAttribute(stdName nam, AttributeTypes typ) {
			name	= nam;
			types	= typ;
		}

		stdName name;											///< The name of the attribute.
		AttributeTypes types;									///< The types of the attribute.
};
typedef std::vector<OnnxAttribute>			OnnxAttributes;		///< A list of OnnxAttribute objects.


/** \brief An ONNX OpCode.
*/
class OnnxOpCode {

	public:

		/** \brief The constructor for an OnnxOpCode.
			\param nam		The name of the ONNX OpCode.
			\param version	The version of the opset.
			\param in		The input parameters.
			\param out		The output parameters.
			\param attr		The attributes.
		*/
		OnnxOpCode(stdName nam, int version, OnnxParameters in, OnnxParameters out, OnnxAttributes attr) {
			name			= nam;
			opset_version	= version;
			inputs			= in;
			outputs			= out;
			attributes		= attr;
		}

		stdName name;				///< The name of the ONNX OpCode.
		int opset_version;			///< The version of the opset.
		OnnxParameters inputs;		///< The input parameters.
		OnnxParameters outputs;		///< The output parameters.
		OnnxAttributes attributes;	///< The attributes.
};


/** \brief A pair of a name and a version to be used as a key in a dictionary.
*/
class stdNameVersion {

	public:

		/** \brief The constructor for a stdNameVersion.
			\param nam		The name of the ONNX OpCode.
			\param version	The version of the opset.
		*/
		stdNameVersion(stdName nam, int version) {
			name		  = nam;
			opset_version = version;
		}

		/** \brief Operator this == o.

			\param o	The object to compare with.

			\return True if the objects are equal.
		*/
		bool operator==(const stdNameVersion &o) const {
			return (opset_version == o.opset_version) && (strcmp(name.name, o.name.name) == 0);
		}

		/** \brief Operator this < o.

			\param o	The object to compare with.

			\return True if this is less than o.
		*/
		bool operator<(const stdNameVersion &o) const {
			return (opset_version < o.opset_version) || ((opset_version == o.opset_version) && (strcmp(name.name, o.name.name) < 0));
		}

		stdName name;				///< The name of the ONNX OpCode.
		int opset_version;			///< The version of the opset.
};
typedef std::map<stdNameVersion, OnnxOpCode> OnnxOpCodeDict;		///< A map of OnnxOpCode objects.



/** \brief OpCodes: The opcodes.
*/
class OpCodes : public Service {

	public:

		OpCodes(pLogger a_logger, pConfigFile a_config);

		virtual StatusCode start	();
		virtual StatusCode shut_down();

		int latest_opset_version();

		bool set_opset_version(int version);

#ifndef CATCH_TEST
	private:
#endif

		ConfigFile onnx_conf = ConfigFile(nullptr);		///< The ONNX opcodes reference stored as a ConfigFile.
		int ir_version;									///< Argument of `model.set_ir_version()` (Stored as ONNX_IR_VERSION in config.)
		OnnxOpCodeDict opcodes;							///< The opcodes parsed as binary objects.
};
typedef OpCodes *pOpCodes;		///< A pointer to a OpCodes

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_OPCODES
