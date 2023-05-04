//
// Created by Carlos Acosta on 21-04-23.
//

#pragma once
#include <vector>
#include <string>
#include <string_view>

namespace TU::Graph {


    static constexpr char GraphHeader[]  = "%ALICANGRAPH%";
    static constexpr size_t HeaderSize = sizeof(GraphHeader);
    using NodeId = std::int32_t;
    using CostType = double;

    struct DAG {
        struct Edge {
            NodeId dest_;
            CostType cost_;
        };
        struct NodeInfo {
            std::string name;
        };

        using EdgeList = std::vector<Edge>;
        using NodeIdList = std::vector<NodeId>;

        [[nodiscard]] inline
        size_t nodeCount() const {
            return adj_.size();
        };

        [[nodiscard]] inline
        const EdgeList& neighbors(NodeId n) const{
            return adj_.at(n);
        }

        inline
        EdgeList& neighbors(NodeId n) {
            return adj_.at(n);
        }


        std::vector< EdgeList > adj_;
    };


    template <typename GraphType>
    struct GraphDescriptionInfo;


    template<>
    struct GraphDescriptionInfo<DAG>
    {
        static const int8_t typeId = '1';
    };

    bool equal( const DAG& l, const DAG& r);
    bool equal( const DAG::Edge& l, const DAG::Edge& r);


    namespace IO::Text {
        void write(const DAG& g, const std::string& filename);

        DAG read(const std::string& filename);
    }// namespace IO::Text

    namespace IO::Binary {
        void write(const DAG& g, const std::string& filename);

        DAG read(const std::string& filename);
    }

    namespace IO::Encoded{
        void write(const DAG& g, const std::string& filename);

        DAG read(const std::string& filename);
    }

    namespace IO::Prec {
        void write(const DAG& g, const std::string& filename);

        std::tuple<std::vector<std::string>, DAG> read(const std::string& filename);
    }

} // TU::Graph

