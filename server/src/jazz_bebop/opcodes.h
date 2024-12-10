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


/** \brief A type definition for tensors across all the technologies: Jazz native, ONNX protocol buffer and ONNX runtime.
*/
class TensorType {
	int jazz_type;									///< The Jazz native type.
	onnx::TensorProto::DataType onnx_proto_type;	///< The ONNX protocol buffer type.
	ONNXTensorElementDataType onnx_rt_type;			///< The ONNX runtime type.
};


class OnnxParameter {


};



/** \brief In Bop-25, opcodes are onnx-runtime operations.

*/

namespace jazz_bebop
{

/** \brief OpCodes: The opcodes.
*/
class OpCodes : public Service {

	public:

		OpCodes(pLogger a_logger, pConfigFile a_config);

		virtual StatusCode start	();
		virtual StatusCode shut_down();

		int latest_opset_version();

		bool set_opset_version(int version);

	private:

		ConfigFile onnx_conf = ConfigFile(nullptr);		///< The ONNX opcodes reference stored as a ConfigFile.
		int ir_version;									///< Argument of `model.set_ir_version()` (Stored as ONNX_IR_VERSION in config.)

};
typedef OpCodes *pOpCodes;		///< A pointer to a OpCodes

} // namespace jazz_bebop

#endif // ifndef INCLUDED_JAZZ_BEBOP_OPCODES
