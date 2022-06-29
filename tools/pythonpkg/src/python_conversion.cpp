#include "duckdb_python/python_conversion.hpp"
#include "duckdb_python/pybind_wrapper.hpp"

#include "duckdb_python/pyrelation.hpp"
#include "duckdb_python/pyconnection.hpp"
#include "duckdb_python/pyresult.hpp"

#include "datetime.h" //From Python

namespace duckdb {

bool DictionaryHasMapFormat(py::handle dict_values, idx_t len) {
	if (len != 2) {
		return false;
	}
	auto keys = dict_values.attr("__getitem__")(0);
	auto values = dict_values.attr("__getitem__")(1);
	// Dont check for 'py::list' to allow ducktyping
	if (!py::hasattr(keys, "__getitem__") || !py::hasattr(keys, "__len__")) {
		// throw std::runtime_error("Dictionary malformed, keys(index 0) found within 'dict.values' is not a list");
		return false;
	}
	if (!py::hasattr(values, "__getitem__") || !py::hasattr(values, "__len__")) {
		// throw std::runtime_error("Dictionary malformed, values(index 1) found within 'dict.values' is not a list");
		return false;
	}
	auto size = py::len(keys);
	if (size != py::len(values)) {
		// throw std::runtime_error("Dictionary malformed, keys and values lists are not of the same size");
		return false;
	}
	return true;
}

Value TransformDictionaryToMap(py::handle dict_values) {
	auto keys = dict_values.attr("__getitem__")(0);
	auto values = dict_values.attr("__getitem__")(1);

	auto key_size = py::len(keys);
	D_ASSERT(key_size == py::len(values));
	if (key_size == 0) {
		// dict == { 'key': [], 'value': [] }
		return Value::MAP(Value::EMPTYLIST(LogicalType::SQLNULL), Value::EMPTYLIST(LogicalType::SQLNULL));
	}
	auto key_list = TransformPythonValue(keys);
	auto value_list = TransformPythonValue(values);
	return Value::MAP(key_list, value_list);
}

Value TransformListValue(py::handle ele) {
	auto size = py::len(ele);

	if (size == 0) {
		return Value::EMPTYLIST(LogicalType::SQLNULL);
	}

	vector<Value> values;
	values.reserve(size);

	LogicalType element_type = LogicalType::SQLNULL;
	for (idx_t i = 0; i < size; i++) {
		Value new_value = TransformPythonValue(ele.attr("__getitem__")(i));
		element_type = LogicalType::MaxLogicalType(element_type, new_value.type());
		values.push_back(move(new_value));
	}

	return Value::LIST(element_type, values);
}

Value TransformPythonValue(py::handle ele) {
	auto &import_cache = *DuckDBPyConnection::ImportCache();

	if (ele.is_none()) {
		return Value();
	} else if (py::isinstance<py::bool_>(ele)) {
		return Value::BOOLEAN(ele.cast<bool>());
	} else if (py::isinstance<py::int_>(ele)) {
		return Value::BIGINT(ele.cast<int64_t>());
	} else if (py::isinstance<py::float_>(ele)) {
		if (std::isnan(PyFloat_AsDouble(ele.ptr()))) {
			return Value();
		}
		return Value::DOUBLE(ele.cast<double>());
	} else if (py::isinstance(ele, import_cache.decimal.Decimal())) {
		return py::str(ele).cast<string>();
	} else if (py::isinstance(ele, import_cache.uuid.UUID())) {
		auto string_val = py::str(ele).cast<string>();
		return Value::UUID(string_val);
	} else if (py::isinstance(ele, import_cache.datetime.datetime())) {
		auto ptr = ele.ptr();
		auto year = PyDateTime_GET_YEAR(ptr);
		auto month = PyDateTime_GET_MONTH(ptr);
		auto day = PyDateTime_GET_DAY(ptr);
		auto hour = PyDateTime_DATE_GET_HOUR(ptr);
		auto minute = PyDateTime_DATE_GET_MINUTE(ptr);
		auto second = PyDateTime_DATE_GET_SECOND(ptr);
		auto micros = PyDateTime_DATE_GET_MICROSECOND(ptr);
		return Value::TIMESTAMP(year, month, day, hour, minute, second, micros);
	} else if (py::isinstance(ele, import_cache.datetime.time())) {
		auto ptr = ele.ptr();
		auto hour = PyDateTime_TIME_GET_HOUR(ptr);
		auto minute = PyDateTime_TIME_GET_MINUTE(ptr);
		auto second = PyDateTime_TIME_GET_SECOND(ptr);
		auto micros = PyDateTime_TIME_GET_MICROSECOND(ptr);
		return Value::TIME(hour, minute, second, micros);
	} else if (py::isinstance(ele, import_cache.datetime.date())) {
		auto ptr = ele.ptr();
		auto year = PyDateTime_GET_YEAR(ptr);
		auto month = PyDateTime_GET_MONTH(ptr);
		auto day = PyDateTime_GET_DAY(ptr);
		return Value::DATE(year, month, day);
	} else if (py::isinstance<py::str>(ele)) {
		return ele.cast<string>();
	} else if (py::isinstance<py::bytearray>(ele)) {
		auto byte_array = ele.ptr();
		auto bytes = PyByteArray_AsString(byte_array);
		return Value::BLOB_RAW(bytes);
	} else if (py::isinstance<py::memoryview>(ele)) {
		py::memoryview py_view = ele.cast<py::memoryview>();
		PyObject *py_view_ptr = py_view.ptr();
		Py_buffer *py_buf = PyMemoryView_GET_BUFFER(py_view_ptr);
		return Value::BLOB(const_data_ptr_t(py_buf->buf), idx_t(py_buf->len));
	} else if (py::isinstance<py::bytes>(ele)) {
		const string &ele_string = ele.cast<string>();
		return Value::BLOB(const_data_ptr_t(ele_string.data()), ele_string.size());
	} else if (py::isinstance<py::list>(ele)) {
		return TransformListValue(ele);
	} else if (py::isinstance<py::dict>(ele)) {
		//! DICT -> MAP FORMAT
		// keys() = [key, value]
		// values() = [ [n keys] ], [ [n values] ]

		//! DICT -> STRUCT FORMAT
		// keys() = ['a', .., 'n']
		// values() = [ val1, .., valn]
		auto dict_values = py::list(ele.attr("values")());
		auto value_size = py::len(dict_values);
		if (value_size == 0) {
			// dict == {}
			return Value::MAP(Value::EMPTYLIST(LogicalType::SQLNULL), Value::EMPTYLIST(LogicalType::SQLNULL));
		}

		if (DictionaryHasMapFormat(dict_values, value_size)) {
			return TransformDictionaryToMap(dict_values);
		}
		auto dict_keys = py::list(ele.attr("keys")());
		child_list_t<Value> struct_values;
		for (idx_t i = 0; i < value_size; i++) {
			auto key = string(py::str(dict_keys.attr("__getitem__")(i)));
			auto val = TransformPythonValue(dict_values.attr("__getitem__")(i));
			struct_values.emplace_back(make_pair(key, move(val)));
		}
		return Value::STRUCT(move(struct_values));
	} else if (py::isinstance(ele, import_cache.numpy.ndarray())) {
		return TransformPythonValue(ele.attr("tolist")());
	} else {
		throw std::runtime_error("TransformPythonValue unknown param type " + py::str(ele.get_type()).cast<string>());
	}
}

} // namespace duckdb
