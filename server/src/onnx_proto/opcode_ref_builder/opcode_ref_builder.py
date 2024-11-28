from onnx.defs import get_all_schemas_with_history


def list_op_versions():
	ver = set()
	for s in get_all_schemas_with_history():
		if not s.deprecated:
			ver.add(s.since_version)
	ret = list(ver)
	ret.sort()
	return ret


def list_operators(version: int):
	op = set()
	for s in get_all_schemas_with_history():
		if s.since_version <= version and not s.deprecated:
			op.add(s.name)
	ret = list(op)
	ret.sort()
	return ret
