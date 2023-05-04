//
// Created by Carlos Acosta on 21-04-23.
//

#include "tubul_graph.h"
#include "tubul_irange.h"
#include "tubul_exception.h"
#include "tubul_varint.h"
#include "tubul_string.h"
#include "tubul_file_utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <deque>


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

    template<typename EnumType>
    auto toNumber(EnumType val) -> std::underlying_type_t<EnumType> {
        return static_cast< std::underlying_type_t<EnumType> > (val);
    }

    template<typename EnumType>
    auto toEnum( std::underlying_type_t<EnumType> val) -> EnumType {
        return static_cast< EnumType >(val);
    }

    //Namespace for functions that can be used by other more specific
    //IO methods.
    namespace IO
    {
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
            while( byte & continuationBit ){
                i.get(byte);
                buff.push_back(byte);
            }

            return fromVarint(buff);
        }
    }//namespace IO.

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
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!" + error);
            }

            int n;
            line >> n;
            g.adj_.resize(n);
        }

        DAG::EdgeList readEdgeList(std::istream& in) {
            size_t nEdges;
            in >> nEdges;
            DAG::EdgeList edgeList;
            for ( int i = 0; i < nEdges; ++i) {
                NodeId nid;
                CostType cost;
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
                g.adj_.at(i) = readEdgeList(in);
            }

            return g;
        }
    }// namespace IO::Text

    namespace IO::Binary
    {

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
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!" + error);
            }

            size_t n;
            readPod(i, n);
            g.adj_.resize(n);
        }

        DAG::EdgeList readEdgeList(std::istream& in) {
            size_t nEdges=0;
            readPod(in, nEdges);
            DAG::EdgeList edgeList;
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
                g.adj_.at(i) = readEdgeList(in);
            }

            return g;
        }
    }//namespace IO::Binary

    namespace IO::Encoded{

        enum class EdgeListDescriptionMask : uint8_t {
            UniqueCosts,
            NoCost,
            SameCost,
            CostByGroup
        };


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
        std::unordered_map<CostType, DAG::NodeIdList > bucketCosts(const ContainerType& c)  {
            std::unordered_map<CostType, DAG::NodeIdList > costs;
            for (const auto& item: c) {
                costs[item.cost_].push_back( item.dest_ );
            }
            return costs;
        }

        void writeExpandedEdgeList(std::ostream& o, const DAG::EdgeList& c){
            auto mask = toNumber( EdgeListDescriptionMask::UniqueCosts );
            writePod(o, mask);
            for ( const auto& edge: c) {
                writeEdge(o, edge );
            }
        }

        bool bucketMinSize(const std::unordered_map<CostType, DAG::NodeIdList >& buckets, size_t min){
            return std::all_of( buckets.begin(), buckets.end(),
                                [=](const auto& item) -> bool
                                         { return (item.second.size() >= min); }
            );
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

            if ( c.size() == 1 ) {
                writeExpandedEdgeList(o,c);
                return;
            }
            auto costBuckets = bucketCosts(c);
            //There's only one cost, so we use the abbreviated form (even
            //better if it's 0 because we skip it altogether).
            if (costBuckets.size() == 1) {
                auto firstCost = c.front().cost_;
                if (firstCost == 0) {
                    auto mask = toNumber( EdgeListDescriptionMask::NoCost );
                    writePod(o, mask);
                } else {
                    auto mask = toNumber( EdgeListDescriptionMask::SameCost );
                    writePod(o, mask);
                    writePod(o, firstCost);
                }

                for (const auto &edge: c) {
                    writeVarInt(o, toVarint(edge.dest_));
                }
                return;
            }
            //What I would like here is a proper way to estimate which way
            //of encoding is going to save me more bytes. For now, I expect that each
            //bucket has more than 2 items each.
            if (costBuckets.size() == 2 and bucketMinSize(costBuckets,2))
            {
                auto mask = toNumber( EdgeListDescriptionMask::CostByGroup );
                writePod(o, mask);
                for (const auto& item: costBuckets ) {
                    auto firstCost = item.first;
                    writeVarInt(o, toVarint( item.second.size() ) );
                    if (firstCost == 0) {
                        auto submask = toNumber( EdgeListDescriptionMask::NoCost );
                        writePod(o, submask);
                    } else {
                        auto submask = toNumber( EdgeListDescriptionMask::SameCost );
                        writePod(o, submask);
                        writePod(o, firstCost);
                    }
                    for (const auto &dest: item.second) {
                        writeVarInt(o, toVarint(dest));
                    }
                }

                return;
            }



            //Final case, simply write all edges
            writeExpandedEdgeList(o,c);

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
                throw TU::Exception("[Graph] There's a difference in the header, file may be corrupt!" + error);
            }

            size_t n = readVarInt(i);
            //readPod(i, n);
            g.adj_.resize(n);
        }

        DAG::EdgeList readEdgeList(std::istream& in) {
            size_t nEdges= readVarInt(in);
            DAG::EdgeList edgeList;
            if ( nEdges == 0 )
                return edgeList;

            auto c = toNumber( EdgeListDescriptionMask::NoCost );
            readPod(in,c);
            auto descr =  toEnum<EdgeListDescriptionMask>(c );

            //If all arcs have unique costs, we simply read all edges directly.
            CostType commonCost = 0;
            if ( descr == EdgeListDescriptionMask::CostByGroup) {
                size_t edgesToRead = nEdges;
                while ( edgesToRead > 0){
                    commonCost = 0;
                    size_t subListEdges = readVarInt(in);
                    auto sublistByte = toNumber( EdgeListDescriptionMask::NoCost );
                    readPod(in,sublistByte);
                    auto sublistDescr = toEnum<EdgeListDescriptionMask>(sublistByte );
                    //Should only be SameCost or NoCost.
                    if ( sublistDescr == EdgeListDescriptionMask::SameCost )
                        readPod(in, commonCost);
                    else if ( sublistDescr != EdgeListDescriptionMask::NoCost)
                        throw TU::Exception("Reading list with subgroups decoded something invalid");

                    for (int i = 0; i < subListEdges; ++i) {
                        DAG::Edge e{0, 0};
                        e.dest_ = static_cast<NodeId >( readVarInt(in));
                        e.cost_ = commonCost;
                        edgeList.push_back(e);
                    }
                    edgesToRead -= subListEdges;
                }
                return edgeList;

            }
            else if ( descr == EdgeListDescriptionMask::UniqueCosts ) {
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
                g.adj_.at(i) = readEdgeList(in);
            }

            return g;
        }
    }//namespace IO::Encoded

    namespace IO::Prec {
        std::string buildPrecString(const DAG::Edge& e){
            if (e.cost_ == 0)
                return std::to_string(e.dest_);
            else
                return std::to_string(e.dest_) + ":" + std::to_string(e.cost_);
        }

        std::string buildPrecLine(NodeId nId, const DAG::EdgeList& edges){
            std::ostringstream precLine;
            precLine << nId << ' ' << edges.size();
            if (edges.empty())
                return precLine.str();

            auto edgeIt = edges.begin();
            precLine << ' ' << buildPrecString(*edgeIt );
            ++edgeIt;
            for (;edgeIt != edges.end(); ++edgeIt ){
                precLine << ' ' << buildPrecString(*edgeIt );

            }
            return precLine.str();
        }

        std::string buildPrecString(const DAG::Edge& e, const DAG::NodeNameList& names){
            if (e.cost_ == 0)
                return names[e.dest_];
            else
                return names[e.dest_] + ":" + std::to_string(e.cost_);
        }

        std::string buildPrecLine(NodeId nId, const DAG::EdgeList& edges, const DAG::NodeNameList& names){
            std::ostringstream precLine;
            precLine << names[nId] << ' ' << edges.size();
            if (edges.empty())
                return precLine.str();

            auto edgeIt = edges.begin();
            precLine << ' ' << buildPrecString(*edgeIt, names );
            ++edgeIt;
            for (;edgeIt != edges.end(); ++edgeIt ){
                precLine << ' ' << buildPrecString(*edgeIt, names );

            }
            return precLine.str();
        }

        void write(const DAG& g, const std::string& filename){
            std::ofstream o(filename);
            if (g.nameTable_.empty()){
                for (std::integral auto nId: TU::irange(g.nodeCount())) {
                    const auto &neighbors = g.neighbors(nId);
                    if ( not neighbors.empty())
                        o << buildPrecLine(nId, neighbors) << '\n';
                }
            }
            else{
                for (auto nodeName: g.nameTable_) {
                    NodeId nId = g.nameIndex_.at(nodeName);
                    const auto &neighbors = g.neighbors(nId);
                    if ( not neighbors.empty())
                        o << buildPrecLine(nId, neighbors, g.nameTable_) << '\n';
                }

            }

        }

        std::tuple<std::string_view, int> readPrecEdge(std::string_view item){
            auto pieces = TU::split(item,":");
            auto checkPrecType = []( std::string_view t) -> bool{
                if ( t.size() != 2)
                    return false;
                if ( t == "mb" || t == "MB")
                    return true;
                return false;
            };

            switch ( pieces.size() ) {
                case 1:
                    return {item, 0};
                    break;
                case 2:{
                    auto& firstItem = pieces.front();
                    if (  checkPrecType(firstItem) )
                        return {pieces[1], 0};
                    return { pieces[0], TU::strToInt(pieces[1])};
                }
                    break;
                case 3:{
                    auto& firstItem = pieces.front();
                   if ( not checkPrecType(firstItem) )
                       throw TU::Exception("Error when trying to parse precedence. First item should be identifier MB, but isn't");
                   return {pieces[1], TU::strToInt(pieces[2])};

                }
                    break;
                default:
                    throw TU::Exception("Error when trying to parse precedence");
            }
        }


         DAG read(const std::string& filename){
            DAG graph;
            DAG::NodeNameList & nameTable = graph.nameTable_;
            DAG::NodeNameIndex & nameIndex = graph.nameIndex_;

            auto getNameId = [&](std::string_view nodeName) -> size_t {
                auto headFound = nameIndex.find( nodeName );
                if ( headFound != nameIndex.end()){
                    return headFound->second;
                }

                auto newId = nameTable.size();
                auto [strbegin, strend] = TU::strview_range(nodeName);
                nameTable.emplace_back(strbegin, strend);
                std::string_view nameView( nameTable.back() );
                nameIndex.emplace(nameView, newId);
                return newId;
            };

            auto safeNeighbors = [&](size_t n) -> DAG::EdgeList& {
                if ( graph.nodeCount() <= n)
                    graph.adj_.resize(n+1);
                return graph.neighbors(n);
            };

            auto precCount = TU::countCharInFile(filename, '\n');
            graph.adj_.resize(precCount);

            std::ifstream in(filename);
            std::string line;
            size_t lineNumber = 0;
            while ( std::getline(in,line)){

                //just in case, drop the \r that may be left at the end.
                if ( line.ends_with('\r') )
                    line.erase(line.size()-1);
                //split by spaces and the line should have at least 2 items:
                //the head and count of precedences
                auto tokens = TU::split(line);
                if ( tokens.size() < 2)
                    throw TU::Exception("Error parsing precedence line. Line contains less than 2 items.");

                auto it = tokens.begin();
                auto [headName, dummy] = readPrecEdge(*it);
                ++it;
                auto count = TU::strToInt(*it);
                ++it;

                    size_t headId = getNameId(headName);
                    DAG::EdgeList &nodeEdges = safeNeighbors(headId);
                    nodeEdges.reserve(count);


                    if (tokens.size() - 2 != count)
                        throw TU::Exception(
                                "Error parsing precedence line. Line mismatch between declared count and found items.");

                    for (; it != tokens.end(); ++it) {
                        auto [name, lag] = readPrecEdge(*it);
                        auto precNameId = getNameId(name);
                        nodeEdges.emplace_back(DAG::Edge{static_cast<NodeId>(precNameId), static_cast<CostType>(lag)});
                    }

                ++lineNumber;

            }

            return graph;
        }

    } //namespace IO::Prec
} // TU