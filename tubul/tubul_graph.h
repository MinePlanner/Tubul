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

    struct DAG {
        struct Edge {
            NodeId dest_;
            double cost_;
        };
        struct NodeInfo {
            std::string name;
        };

        [[nodiscard]] inline
        size_t nodeCount() const {
            return adj_.size();
        };

        [[nodiscard]] inline
        const std::vector<Edge>& neighbors(NodeId n) const{
            return adj_.at(n);
        }

        inline
        std::vector<Edge>& neighbors(NodeId n) {
            return adj_.at(n);
        }


        std::vector< std::vector<Edge> > adj_;
        //std::vector<NodeInfo> info_; //Needs to be here?
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
} // TU::Graph

