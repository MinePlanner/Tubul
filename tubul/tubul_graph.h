//
// Created by Carlos Acosta on 21-04-23.
//

#pragma once
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>
#include <vector>

namespace TU::Graph
{

static constexpr char   GraphHeader[] = "%ALICANGRAPH%";
static constexpr size_t HeaderSize    = sizeof(GraphHeader);
using NodeId                          = int32_t;
using CostType                        = int32_t;

struct SparseWeightDirected
{
	struct Edge
	{
		NodeId   dest_;
		CostType cost_;
	};

	using EdgeList      = std::vector<Edge>;
	using NodeIdList    = std::vector<NodeId>;
	using NodeNameList  = std::deque<std::string>;
	using NodeNameIndex = std::unordered_map<std::string_view, size_t>;

	[[nodiscard]] inline size_t nodeCount() const
	{
		return adj_.size();
	};

	[[nodiscard]] inline const EdgeList &neighbors(NodeId n) const
	{
		return adj_.at(n);
	}

	[[nodiscard]] inline EdgeList &neighbors(NodeId n)
	{
		return adj_.at(n);
	}

	NodeNameList          nameTable_;
	NodeNameIndex         nameIndex_;
	std::vector<EdgeList> adj_;
};

template <typename GraphType>
struct GraphDescriptionInfo;

template <>
struct GraphDescriptionInfo<SparseWeightDirected>
{
	static const int8_t typeId = '1';
};

bool equal(const SparseWeightDirected &l, const SparseWeightDirected &r);
bool equal(const SparseWeightDirected::Edge &l, const SparseWeightDirected::Edge &r);

namespace IO::Text
{
	void write(const SparseWeightDirected &g, const std::string &filename);

	SparseWeightDirected read(const std::string &filename);
} // namespace IO::Text

namespace IO::Binary
{
	void write(const SparseWeightDirected &g, const std::string &filename);

	SparseWeightDirected read(const std::string &filename);
} // namespace IO::Binary

namespace IO::Encoded
{
	void write(const SparseWeightDirected &g, const std::string &filename);

	SparseWeightDirected read(const std::string &filename);
} // namespace IO::Encoded

namespace IO::Prec
{
	void write(const SparseWeightDirected &g, const std::string &filename);

	SparseWeightDirected read(const std::string &filename);
} // namespace IO::Prec

} // namespace TU::Graph
