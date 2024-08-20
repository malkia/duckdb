//-------------------------------------------------------------------------
//                         DuckDB
//
//
// duckdb/common/enums/metrics_type.hpp
// 
// This file is automatically generated by scripts/generate_metric_enums.py
// Do not edit this file manually, your changes will be overwritten
//-------------------------------------------------------------------------

#pragma once

#include "duckdb/common/constants.hpp"
#include "duckdb/common/unordered_set.hpp"

namespace duckdb {

enum class MetricsType : uint8_t {
    QUERY_NAME,
    BLOCKED_THREAD_TIME,
    CPU_TIME,
    EXTRA_INFO,
    CUMULATIVE_CARDINALITY,
    OPERATOR_TYPE,
    OPERATOR_CARDINALITY,
    CUMULATIVE_ROWS_SCANNED,
    OPERATOR_ROWS_SCANNED,
    OPERATOR_TIMING,
    ALL_OPTIMIZERS,
    CUMULATIVE_OPTIMIZER_TIMING,
    PLANNER,
    PLANNER_BINDING,
    PHYSICAL_PLANNER,
    PHYSICAL_PLANNER_COLUMN_BINDING,
    PHYSICAL_PLANNER_RESOLVE_TYPES,
    PHYSICAL_PLANNER_CREATE_PLAN,
    OPTIMIZER_EXPRESSION_REWRITER,
    OPTIMIZER_FILTER_PULLUP,
    OPTIMIZER_FILTER_PUSHDOWN,
    OPTIMIZER_CTE_FILTER_PUSHER,
    OPTIMIZER_REGEX_RANGE,
    OPTIMIZER_IN_CLAUSE,
    OPTIMIZER_JOIN_ORDER,
    OPTIMIZER_DELIMINATOR,
    OPTIMIZER_UNNEST_REWRITER,
    OPTIMIZER_UNUSED_COLUMNS,
    OPTIMIZER_STATISTICS_PROPAGATION,
    OPTIMIZER_COMMON_SUBEXPRESSIONS,
    OPTIMIZER_COMMON_AGGREGATE,
    OPTIMIZER_COLUMN_LIFETIME,
    OPTIMIZER_BUILD_SIDE_PROBE_SIDE,
    OPTIMIZER_LIMIT_PUSHDOWN,
    OPTIMIZER_TOP_N,
    OPTIMIZER_COMPRESSED_MATERIALIZATION,
    OPTIMIZER_DUPLICATE_GROUPS,
    OPTIMIZER_REORDER_FILTER,
    OPTIMIZER_JOIN_FILTER_PUSHDOWN,
    OPTIMIZER_EXTENSION,
    OPTIMIZER_MATERIALIZED_CTE,
};

struct MetricsTypeHashFunction {
	uint64_t operator()(const MetricsType &index) const {
		return std::hash<uint8_t>()(static_cast<uint8_t>(index));
	}
};

typedef unordered_set<MetricsType, MetricsTypeHashFunction> profiler_settings_t;
typedef unordered_map<MetricsType, Value, MetricsTypeHashFunction> profiler_metrics_t;

profiler_settings_t GetOptimizerMetrics();
MetricsType GetOptimizerMetricByType(OptimizerType type);

} // namespace duckdb
