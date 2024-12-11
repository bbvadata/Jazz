#!/usr/bin/env python3

import datetime, os, re

from onnx.defs import get_all_schemas_with_history


__VERSION__ = '0.1.0'


class OperatorRef:

	def __init__(self, verbose = True):
		self.basic_types		= set(['bool', 'double', 'float', 'string'])
		self.specific_types		= set(['bfloat16', 'float16', 'int16', 'int32', 'int64', 'int8', 'uint16', 'uint32', 'uint64', 'uint8'])
		self.unused_types		= set(['complex128', 'complex64', 'float8e4m3fn', 'float8e4m3fnuz', 'float8e5m2', 'float8e5m2fnuz'])
		self.attr_types			= set(['FLOAT', 'FLOATS', 'GRAPH', 'INT', 'INTS', 'STRING', 'STRINGS', 'TENSOR'])
		self.unused_attr_types	= set(['SPARSE_TENSOR', 'TYPE_PROTO'])

		self.rex_optional	= re.compile('^optional\\((.*)\\)$')
		self.rex_seq		= re.compile('^seq\\((.*)\\)$')
		self.rex_tensor		= re.compile('^tensor\\((.*)\\)$')
		self.rex_attr_type	= re.compile('^AttrType\\.(.*)$')
		self.rex_name		= re.compile('^[A-Za-z][A-Za-z0-9_]*$')

		self.verbose = verbose


	def validate_types(self, types):
		tensor_types = set()
		sequence_types = set()

		for t in types:
			optional = self.rex_optional.match(t)
			if optional:
				t = self.rex_optional.sub('\\1', t)

			sequence = self.rex_seq.match(t)
			if sequence:
				t = self.rex_seq.sub('\\1', t)

			assert self.rex_tensor.match(t)
			t = self.rex_tensor.sub('\\1', t)

			assert t in self.basic_types or t in self.specific_types or t in self.unused_types

			if t not in self.unused_types:
				if sequence:
					sequence_types.add(t)
				else:
					tensor_types.add(t)

		for t in sequence_types:
			if t not in tensor_types:
				return False

		return len(tensor_types) > 0


	def tensor_types(self, types):
		tensor_types = set()

		for t in types:
			if self.rex_optional.match(t):
				t = self.rex_optional.sub('\\1', t)

			if self.rex_seq.match(t):
				t = self.rex_seq.sub('\\1', t)

			t = self.rex_tensor.sub('\\1', t)

			if t not in self.unused_types:
				tensor_types.add(t)

		assert len(tensor_types) > 0
		tensor_types = list(tensor_types)
		tensor_types.sort()
		return ','.join(tensor_types)


	def validate_attr_type(self, t):
		t = str(t)

		assert self.rex_attr_type.match(t)
		t = self.rex_attr_type.sub('\\1', t)

		assert t in self.attr_types or t in self.unused_attr_types

		return t in self.attr_types


	def lower_attr_type(self, t):
		t = str(t)

		t = self.rex_attr_type.sub('\\1', t)

		return t.lower()


	def validate_all_types(self, op, silent = False):
		silent = silent or not self.verbose
		for i in op.inputs:
			if not self.validate_types(i.types):
				if not silent:
					print('Invalid input types in', op.name)

				return False

		for o in op.outputs:
			if not self.validate_types(o.types):
				if not silent:
					print('Invalid output types in', op.name)

				return False

		for _, a in op.attributes.items():
			if not self.validate_attr_type(a.type):
				if not silent:
					print('Invalid attribute type in "%s" with type "%s"' % (op.name, str(a.type)))

				return False

		return True


	def write_onnx_ini(self, output_fn = '../../config/onnx.ini', valid_domains = set(['']), valid_deprecated = set([False])):
		names		= set()
		name_vers	= set()
		conf		= {}
		max_version = 0
		max_key_len = 0

		for op in get_all_schemas_with_history():
			if op.domain not in valid_domains:
				continue
			if op.deprecated not in valid_deprecated:
				continue
			if not self.validate_all_types(op, silent = True):
				continue
			if not self.rex_name.match(op.name):
				continue

			name = op.name
			vers = op.since_version

			if vers > max_version:
				max_version = vers

			key	= '%s.%d' % (name, vers)

			names.add(name)
			name_vers.add(key)

			nkey = '%s.%s' % (key, 'num_inputs')
			conf[nkey] = str(len(op.inputs))
			max_key_len = max(max_key_len, len(nkey))

			for i, inp in enumerate(op.inputs):
				nkey = '%s.input.%d.name' % (key, i)
				conf[nkey] = inp.name
				max_key_len = max(max_key_len, len(nkey))

				nkey = '%s.input.%d.tensor_types' % (key, i)
				conf[nkey] = self.tensor_types(inp.types)
				max_key_len = max(max_key_len, len(nkey))

			nkey = '%s.%s' % (key, 'num_outputs')
			conf[nkey] = str(len(op.outputs))
			max_key_len = max(max_key_len, len(nkey))

			for i, out in enumerate(op.outputs):
				nkey = '%s.output.%d.name' % (key, i)
				conf[nkey] = out.name
				max_key_len = max(max_key_len, len(nkey))

				nkey = '%s.output.%d.tensor_types' % (key, i)
				conf[nkey] = self.tensor_types(out.types)
				max_key_len = max(max_key_len, len(nkey))

			nkey = '%s.%s' % (key, 'num_attributes')
			conf[nkey] = str(len(op.attributes))
			max_key_len = max(max_key_len, len(nkey))

			for i, (nam, a) in enumerate(op.attributes.items()):
				nkey = '%s.attribute.%d.name' % (key, i)
				conf[nkey] = nam
				max_key_len = max(max_key_len, len(nkey))

				nkey = '%s.attribute.%d.type' % (key, i)
				conf[nkey] = self.lower_attr_type(a.type)
				max_key_len = max(max_key_len, len(nkey))

		names = list(names)
		names.sort()
		conf['__names'] = ','.join(names)

		name_vers = list(name_vers)
		name_vers.sort()
		conf['__name_vers'] = ','.join(name_vers)

		output_fn = os.path.abspath(output_fn)

		if self.verbose:
			print('Writing to:', output_fn)

		os.makedirs(os.path.dirname(output_fn), exist_ok = True)

		template = '%' + str(max_key_len) + 's = %s\n'

		with open(output_fn, 'w') as f:
			f.write('// This file is automatically generated. DO NOT EDIT!\n')
			f.write('\n')
			f.write('//   Is part of the Jazz server project: https://github.com/kaalam/Jazz\n')
			f.write('//   Was written using:                  <Jazz>/server/src/onnx_proto/opcode_ref_builder.py\n')
			f.write('//   Version of opcode_ref_builder:      %s\n' % __VERSION__)
			f.write('//   Supports ONNX until opset version:  %s\n' % str(max_version))
			f.write('//   Created:                            %s\n' % datetime.datetime.now().strftime('%Y-%m-%d %H:%M'))

			f.write('\n')

			keys = list(conf.keys())
			keys.sort()

			for key in keys:
				f.write(template % (key, conf[key]))

		if self.verbose:
			print('Done.')


if __name__ == '__main__':
	op_ref = OperatorRef(verbose = True)
	op_ref.write_onnx_ini()
