//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/execution/operator/order/physical_order.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/types/chunk_collection.hpp"
#include "duckdb/common/types/row_data_collection.hpp"
#include "duckdb/common/types/row_layout.hpp"
#include "duckdb/execution/expression_executor.hpp"
#include "duckdb/execution/physical_sink.hpp"
#include "duckdb/parallel/pipeline.hpp"
#include "duckdb/planner/bound_query_node.hpp"

namespace duckdb {

struct SortingState;
struct SortedBlock;
class OrderLocalState;
class OrderGlobalState;

//! Physically re-orders the input data
class PhysicalOrder : public PhysicalSink {
public:
	PhysicalOrder(vector<LogicalType> types, vector<BoundOrderByNode> orders, idx_t estimated_cardinality);

	//! Input data
	vector<BoundOrderByNode> orders;

public:
	void Sink(ExecutionContext &context, GlobalOperatorState &gstate_p, LocalSinkState &lstate_p,
	          DataChunk &input) const override;
	void Combine(ExecutionContext &context, GlobalOperatorState &gstate_p, LocalSinkState &lstate_p) override;
	bool Finalize(Pipeline &pipeline, ClientContext &context, unique_ptr<GlobalOperatorState> gstate_p) override;

	unique_ptr<LocalSinkState> GetLocalSinkState(ExecutionContext &context) override;
	unique_ptr<GlobalOperatorState> GetGlobalState(ClientContext &context) override;

	void GetChunkInternal(ExecutionContext &context, DataChunk &chunk, PhysicalOperatorState *state) const override;
	unique_ptr<PhysicalOperatorState> GetOperatorState() override;

	string ParamsToString() const override;

	//! Tuples are merged in strides of size MERGE_STRIDE
	constexpr static idx_t MERGE_STRIDE = 1024;

	//! Schedule merge tasks until all blocks are merged
	static void ScheduleMergeTasks(Pipeline &pipeline, ClientContext &context, OrderGlobalState &state);

private:
	//! Sort and re-order local state data when the local state has aggregated SORTING_BLOCK_SIZE data
	void SortLocalState(ClientContext &context, OrderLocalState &lstate, OrderGlobalState &state) const;
};

} // namespace duckdb
