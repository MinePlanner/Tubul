//
// Created by Carlos Acosta on 21-04-23.
//

#include "tubul_graph.h"
#include "tubul_irange.h"
#include "tubul_exception.h"
#include "tubul_varint.h"
#include <iostream>
#include <fstream>
#include <sstream>


namespace TU::Graph {

    bool equal( const DAG::Edge& l, const DAG::Edge& r)
    {
        return l.dest_ == r.dest_ and l.cost_ == r.cost_;
    }

    bool equal( const DAG& l, const DAG& r)
    {
        if ( l.nodeCount() != r.nodeCount())
            return false;
        for (std::integral auto nid: irange(l.nodeCount()))
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
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!");
        }

        void readDescription(std::istream& i, TU::Graph::DAG& g) {
            std::string descr;
            std::getline(i, descr);
            std::istringstream line(descr);
            char id;
            line >> id;

            auto expected = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG>::typeId;
            if ( id != expected ) {
                std::string error;
                error += "Expected graph with id '" + std::to_string(expected) +
                          "' but found '" + std::to_string( id ) + "'" ;
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!");
            }

            int n;
            line >> n;
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

        DAG read(const std::string& filename)
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
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!");
        }

        void readDescription(std::istream& i, TU::Graph::DAG& g) {
            std::string descr;
            auto expected = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG>::typeId;
            auto id = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG>::typeId;
            readPod(i, id);

            if ( id != expected ) {
                std::string error;
                error += "Expected graph with id '" + std::to_string(expected) +
                         "' but found '" + std::to_string(id) + "'";
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!");
            }

            size_t n;
            readPod(i, n);
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

        DAG read(const std::string& filename)
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

    namespace IO::Encoded{

        enum class EdgeListDescriptionMask : uint8_t {
            UniqueCosts,
            NoCost,
            SameCost
        };

        template<typename TypeToWrite>
        void writePod(std::ostream& o, const TypeToWrite& t) {
            o.write(reinterpret_cast<const char*>(&t), sizeof(t));
        }

        template<typename TypeToRead>
        void readPod(std::istream& i, TypeToRead& t) {
            i.read(reinterpret_cast<char*>(&t), sizeof(t));
        }

        void writeVarInt(std::ostream& o, const VarIntBuffer& v){
            o.write(reinterpret_cast<const char*>(v.data()), v.size());
        }

        uint64_t readVarInt(std::istream& i){
            VarIntBuffer buff;
            char byte = 0;
            i.get(byte);
            buff.push_back(byte);
            constexpr uint8_t continuationBit = 128;
            while( byte & 128 ){
                i.get(byte);
                buff.push_back(byte);
            }

            return fromVarint(buff);
        }

        void writeHeader(std::ostream& o){
            o.write(GraphHeader, sizeof(GraphHeader));
        }

        void writeDescription(std::ostream& o, const TU::Graph::DAG& g){
            auto id = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG >::typeId;
            writePod( o, id);
            auto n = g.nodeCount();
            auto encodedN = toVarint(n);
            writeVarInt( o, encodedN);
        }

        void writeEdge(std::ostream& o, const DAG::Edge& e) {
            writeVarInt(o, toVarint(e.dest_));
            writePod(o, e.cost_);
        }

        template <typename ContainerType>
        bool allCostsEqual(const ContainerType& c) {
            auto firstCost = c.front().cost_;
            auto found = std::find_if(c.begin(), c.end(),
                                      [=](const typename ContainerType::value_type& v)->bool
                                             { return v.cost_ != firstCost;}
                                      );
            if (found == c.end())
                return true;
            return false;
        }
        template <typename ContainerType>
        void writeEdgeList(std::ostream& o, const ContainerType& c){
            //The edge list will be written depending on what is the content.
            //1) If the list is empty, we simply write the 0
            //2) The list contains 1 item or all the costs are different, write
            // the number of items, the uniquecost identifier and each item.
            //3) The list contains more than 1 item and all are the same cost. Write
            //the appropriate identifier if the cost is 0 or something else.
            //3.1) If the cost is 0, we can skip the cost altogether and just write
            //the destinations.
            //3.2) If the cost is not 0, we can write the common cost, and then
            //write only the destinations.
            auto encodedSize = toVarint( c.size() );
            writeVarInt(o, encodedSize);
            if (c.empty())
                return;

            if ( c.size() > 1 && allCostsEqual(c)) {
                auto firstCost = c.front().cost_;
                if ( firstCost == 0) {
                    auto mask = (std::underlying_type_t<EdgeListDescriptionMask>) EdgeListDescriptionMask::NoCost;
                    o.write((char *) (&mask), sizeof(mask));
                    for (const auto &edge: c) {
                        writeVarInt(o, toVarint(edge.dest_));
                    }
                } else {
                    auto mask = (std::underlying_type_t<EdgeListDescriptionMask>) EdgeListDescriptionMask::SameCost;
                    writePod(o, mask);
                    writePod( o, firstCost );
                    for (const auto &edge: c) {
                        writeVarInt(o, toVarint(edge.dest_));
                    }
                }
                return;
            }

            auto mask = (std::underlying_type_t<EdgeListDescriptionMask>) EdgeListDescriptionMask::UniqueCosts;
            writePod(o, mask);
            for ( const auto& edge: c) {
                writeEdge(o, edge );
            }
        }

        void write(const DAG& g, const std::string& filename) {
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
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!");
        }

        void readDescription(std::istream& i, TU::Graph::DAG& g) {
            std::string descr;
            auto expected = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG>::typeId;
            auto id = ::TU::Graph::GraphDescriptionInfo<::TU::Graph::DAG>::typeId;
            readPod(i, id);

            if ( id != expected ) {
                std::string error;
                error += "Expected graph with id '" + std::to_string(expected) +
                         "' but found '" + std::to_string(id) + "'";
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!");
            }

            size_t n = readVarInt(i);
            //readPod(i, n);
            g.adj_.resize(n);
        }

        std::vector<DAG::Edge> readEdgeList(std::istream& in, TU::Graph::DAG& g) {
            size_t nEdges= readVarInt(in);
            std::vector<DAG::Edge> edgeList;
            if ( nEdges == 0 )
                return edgeList;

            auto c = (std::underlying_type_t<EdgeListDescriptionMask>) EdgeListDescriptionMask::NoCost;
            readPod(in,c);
            auto descr =  static_cast<EdgeListDescriptionMask>((std::underlying_type_t<EdgeListDescriptionMask>)c);

            //If all arcs have unique costs, we simply read all edges directly.
            double commonCost = 0;
            if ( descr == EdgeListDescriptionMask::UniqueCosts ) {
                for (int i = 0; i < nEdges; ++i) {
                    DAG::Edge e{0, 0};
                    e.dest_ = static_cast<NodeId >( readVarInt(in));
                    readPod(in, e.cost_);
                    edgeList.push_back(e);
                }
                return edgeList;
            }
            //If all arcs have the same cost, we "buffer" the common cost
            //and use it for all edges. If the cost is 0, it works the same.
            else if ( descr == EdgeListDescriptionMask::SameCost ){
                readPod(in, commonCost);
            }
            //else EdgeListDescriptionMask::NoCost -> nothing to do
            for (int i = 0; i < nEdges; ++i) {
                DAG::Edge e{0, 0};
                e.dest_ = static_cast<NodeId >( readVarInt(in));
                e.cost_ = commonCost;
                edgeList.push_back(e);
            }
            return edgeList;

        }

        DAG read(const std::string& filename)
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
    }//namespace IO::Encoded
} // TU