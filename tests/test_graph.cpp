//
// Created by Carlos Acosta on 13-04-23.
//
#include "tubul.h"
#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <string_view>
#include <concepts>
#include <algorithm>

namespace TU::Graph {

    static constexpr char GraphHeader[]  = "%ALICANGRAPH%";
    static constexpr size_t HeaderSize = sizeof(GraphHeader);
    using NodeId = std::int32_t;
    struct DAG;

    template <typename GraphType>
    struct GraphDescriptionInfo;

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

        [[nodiscard]]
        const std::vector<Edge>& neighbors(NodeId n) const{
            return adj_.at(n);
        }

        std::vector<Edge>& neighbors(NodeId n) {
            return adj_.at(n);
        }


        std::vector< std::vector<Edge> > adj_;
        //std::vector<NodeInfo> info_; //Needs to be here?
    };

    bool equal( const DAG::Edge& l, const DAG::Edge& r)
    {
        return l.dest_ == r.dest_ and l.cost_ == r.cost_;
    }

    bool equal( const DAG& l, const DAG& r)
    {
        if ( l.nodeCount() != r.nodeCount())
            return false;
        for (std::integral auto nid: TU::irange(l.nodeCount()))
        {
            //If both sides are sorted, this would be A LOT faster/
            auto const& lside = l.neighbors(nid);
            auto const& rside = r.neighbors(nid);
            if (lside.size() != rside.size())
                return false;
            for ( auto item: lside) {
                auto comp = [=](const DAG::Edge& d)->bool { return equal(d,item);};
                auto found = std::find_if(rside.begin(), rside.end(),comp);
                if( found == rside.end() )
                    return false;
            }
        }
        return true;
    }
    template<>
    struct GraphDescriptionInfo<DAG>
    {
        static const int8_t typeId = '1';
    };




    namespace IO::Text
    {
        void writeHeader(std::ostream& o){
            o << GraphHeader  << '\n';
        }

        void writeDescription(std::ostream& o, const TU::Graph::DAG& g){
            auto id = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG >::typeId;
            o << id << ' ' << g.nodeCount() << '\n';
        }

        void writeEdge(std::ostream& o, const DAG::Edge& e) {
            o << e.dest_ << ' ' << e.cost_;
        }

        template <typename ContainerType>
        void writeEdgeList(std::ostream& o, const ContainerType& c){
            o << c.size();
            if (c.empty())
                return;
            o << ' ';
            writeEdge(o, c.front() );

            auto it = c.begin()+1;
            auto end = c.end();
            for (; it != end; ++it) {
                const auto& item = *it;
                o << ' ';
                writeEdge(o, item);
            }
        }

        void write(const DAG& g, const std::string& filename)
        {

            //#nodes? graph type? offset adj table? //Quizas al final? es mas facil..
            //Tabla de nodos - (id implicito) Nombre - otros params? Num edges?
            //..
            //[Id-nodo](Implicito) lista adj{id node, w}

            std::ofstream out(filename);
            //Header-signature
            writeHeader(out);
            //Description for type of graph other info? #nodes?
            writeDescription(out, g);
            //Dump all edge lists for each node. (only non/empty?) I lose the auto-tracking if I skip them
            for ( std::integral auto i: TU::irange(g.nodeCount()))
            {
                auto& nodeNeighbors = g.neighbors( i );
                writeEdgeList( out, nodeNeighbors );
                out << '\n';
            }
        }

        void readHeader(std::istream& i)
        {
            std::string header;
            std::getline(i, header);
            if (header != GraphHeader )
                std::cout << "There's a difference in the header, file may be corrupt!" << std::endl;
        }

        void readDescription(std::istream& i, TU::Graph::DAG& g) {
            std::string descr;
            std::getline(i, descr);
            std::istringstream line(descr);
            char id;
            line >> id;

            auto expected = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG>::typeId;
            if ( id != expected )
                std::cout << "Expected graph with id '" << expected
                          << "' but found '" <<  id << "'\n";

            int n;
            line >> n;
            std::cout << " Type of graph '" << id << "' with " << n << " nodes\n";
            g.adj_.resize(n);
        }

        std::vector<DAG::Edge> readEdgeList(std::istream& in, TU::Graph::DAG& g) {
            size_t nEdges;
            in >> nEdges;
            std::vector<DAG::Edge> edgeList;
            for ( int i = 0; i < nEdges; ++i) {
                NodeId nid;
                double cost;
                in >> nid;
                in >> cost;
                edgeList.push_back( DAG::Edge{nid,cost} );
            }
            return edgeList;
        }

        DAG read(const std::string_view filename)
        {
            DAG g;
            std::ifstream in(filename);
            //Header-signature
            readHeader(in);
            readDescription(in,g);
            std::string line;
            for(int i = 0; i < g.nodeCount(); ++i) {
                g.adj_.at(i) = readEdgeList(in, g);
            }

            return g;
        }
    }// namespace IO::Text

    namespace IO::Binary
    {
        template<typename TypeToWrite>
        void writePod(std::ostream& o, const TypeToWrite& t) {
            o.write(reinterpret_cast<const char*>(&t), sizeof(t));
        }

        template<typename TypeToRead>
        void readPod(std::istream& i, TypeToRead& t) {
            i.read(reinterpret_cast<char*>(&t), sizeof(t));
        }

        void writeHeader(std::ostream& o){
            o.write(GraphHeader, sizeof(GraphHeader));
        }

        void writeDescription(std::ostream& o, const TU::Graph::DAG& g){
            auto id = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG >::typeId;
            writePod( o, id);
            auto n = g.nodeCount();
            writePod( o, n);
        }

        void writeEdge(std::ostream& o, const DAG::Edge& e) {
            writePod(o,e);
        }

        template <typename ContainerType>
        void writeEdgeList(std::ostream& o, const ContainerType& c){
            size_t csize = c.size();
            writePod(o, csize) ;
            if (c.empty())
                return;

            for ( const auto& edge: c) {
                writeEdge(o, edge );
            }
        }

        void write(const DAG& g, const std::string& filename)
        {

            //#nodes? graph type? offset adj table? //Quizas al final? es mas facil..
            //Tabla de nodos - (id implicito) Nombre - otros params? Num edges?
            //..
            //[Id-nodo](Implicito) lista adj{id node, w}

            std::ofstream out(filename);
            //Header-signature
            writeHeader(out);
            //Description for type of graph other info? #nodes?
            writeDescription(out, g);
            //Dump all edge lists for each node. (only non/empty?) I lose the auto-tracking if I skip them
            for ( std::integral auto i: TU::irange(g.nodeCount()))
            {
                auto& nodeNeighbors = g.neighbors( i );
                writeEdgeList( out, nodeNeighbors );
            }
        }

        void readHeader(std::istream& i) {
            char buffer[sizeof(GraphHeader)];
            i.read(buffer, sizeof(GraphHeader));
            auto eq = std::equal( buffer, buffer+HeaderSize, GraphHeader);

            if ( !eq )
                std::cout << "There's a difference in the header, file may be corrupt!" << std::endl;
        }

        void readDescription(std::istream& i, TU::Graph::DAG& g) {
            std::string descr;
            auto expected = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG>::typeId;
            auto id = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG>::typeId;
            readPod(i, id);

            if ( id != expected )
                std::cout << "Expected graph with id '" << expected
                          << "' but found '" <<  id << "'\n";

            size_t n;
            readPod(i, n);
            std::cout << " Type of graph '" << id << "' with " << n << " nodes\n";
            g.adj_.resize(n);
        }

        std::vector<DAG::Edge> readEdgeList(std::istream& in, TU::Graph::DAG& g) {
            size_t nEdges=0;
            readPod(in, nEdges);
            std::vector<DAG::Edge> edgeList;
            for ( int i = 0; i < nEdges; ++i) {
                DAG::Edge e{0,0};
                readPod(in,e);
                edgeList.push_back( e );
            }
            return edgeList;
        }

        DAG read(const std::string_view filename)
        {
            DAG g;
            std::ifstream in(filename);
            //Header-signature
            readHeader(in);
            readDescription(in,g);

            for(int i = 0; i < g.nodeCount(); ++i) {
                g.adj_.at(i) = readEdgeList(in, g);
            }

            return g;
        }
    }//namespace IO::Binary



}//Namespace TU::Graph




TEST(TUBULGraph, testBasic) {
    std::cout << "Testing graph stuff" << std::endl;

    TU::Graph::DAG dag{{
                               {{1, 1}, {2, 1}},
                               {{2, 1}},
                               {{1, 1}}
                       }
    };
    std::cout << dag.nodeCount() << std::endl;

    std::string filename("test.graph");
    std::cout << "Writing text graph to filename " << filename << std::endl;
    TU::Graph::IO::Text::write(dag,filename);

    auto textg = TU::Graph::IO::Text::read("test.graph");
    std::cout << "Finished reading from file" << std::endl;
    std::cout << "Comparing original and read back graph: ";
    if ( equal(dag, textg))
        std::cout << " Success!!!\n";
    else
        std::cout << " Failure!!!\n";

    std::string filename2("testbin.graph");
    std::cout << "Writing binary graph to filename " << filename2 << std::endl;
    TU::Graph::IO::Binary::write(dag,filename);

    auto bing = TU::Graph::IO::Binary::read("test.graph");
    std::cout << "Finished reading from file" << std::endl;
    std::cout << "Comparing original and read back graph: ";

    if ( equal(dag, bing))
        std::cout << " Success!!!\n";
    else
        std::cout << " Failure!!!\n";
}
