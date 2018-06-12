/*
Sample Run (assuming shadow is the compiled binary)

./shadow -s13276 
    networks/HSapiens/HSapiens.gw networks/SCerevisiae/SCerevisiae.gw networks/RNorvegicus/RNorvegicus.gw 
    HS.oneline.align SC.oneline.align RN.oneline.align > shadow3.gw
*/
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>
#include <unordered_set>
#include <stdexcept>
#include "argparse.hpp"
#include "../../src/utils/cereal/types/unordered_map.hpp"
#include "../../src/utils/cereal/types/memory.hpp"
#include "../../src/utils/cereal/types/vector.hpp"
#include "../../src/utils/cereal/types/string.hpp"
#include "../../src/utils/cereal/types/utility.hpp"
#include "../../src/utils/cereal/archives/binary.hpp"
#include "../../src/utils/cereal/access.hpp"


typedef std::pair<std::string, std::string> StringPair;

namespace fnv {
    constexpr static unsigned int FNV_PRIME  = 0x01000193;
    constexpr static unsigned int FNV_OFFSET = 0x811c9dc5;
    constexpr unsigned int hash(char const * const str, unsigned int val=FNV_OFFSET) {
        return (str[0] == '\0') ? val : hash(&str[1], (val ^ (unsigned int)str[0]) * FNV_PRIME);
    }
}

enum class GraphType {
        gw,
        el
};

GraphType getGraphType(const char* c_str) {
    std::stringstream err;
    switch (fnv::hash(c_str)) {
        case (fnv::hash(".gw")): return GraphType::gw;
        case (fnv::hash(".el")): return GraphType::el;
        default: { err << "Invalid graphType: " << c_str;
                     std::cerr << err.str().c_str() << std::endl;
                     throw err.str().c_str();
                 }
    }
    return GraphType::gw;
}

namespace filesystem {
    std::string name(const std::string & s) {
        char sep = '/'; // NOT PORTABLE, only on Linux
        std::size_t i = s.rfind(sep, s.length());
        if (i != std::string::npos) {
            return s.substr(i+1);
        }
        return s;
    }
    std::string suffix(const std::string & s) {
        std::size_t dot_pos = s.rfind('.');
        return s.substr(dot_pos);
    }
    std::string stem(const std::string & s) {
        if (s.size() == 0) {
            return std::string("");
        }
        std::size_t dot_pos = s.rfind('.');
        std::size_t slash_pos = s.rfind('/'); // WARNING: NOT PORTABLE
        slash_pos = (slash_pos != std::string::npos) ? slash_pos : 0;
        dot_pos = (dot_pos != std::string::npos) ? dot_pos : std::string::npos;
        return (dot_pos > slash_pos) ? s.substr(slash_pos+1, dot_pos-1-slash_pos) : s.substr(slash_pos+1);
    }
}

namespace shadow_graph {
    class Graph {
        void skipGWheader(std::ifstream & reader) {
            std::string line;
            for (int i = 0; i < 4; i++) {
                std::getline(reader,line);
            }
        }
        void skipGWNodes(std::ifstream & reader, unsigned short numNodes) {
            std::string line;
            for (int i = 0; i < numNodes; i++) {
                std::getline(reader, line);
            }
        }
        void loadGW(std::string filename) {
            std::ifstream reader(filename);
            this->skipGWheader(reader);

            std::string line;
            std::getline(reader,line);
            unsigned short numNodes;
            std::istringstream iss(line);
            iss >> numNodes;
            this->nodes = numNodes;
            this->skipGWNodes(reader, numNodes);

            std::getline(reader,line);
            // Clear iss
            iss.str(line);
            iss.clear();
            int numEdges;
            iss >> numEdges;

            adjList = std::vector<std::vector<Pair> >(numNodes, std::vector<Pair>(0));
            //std::cerr << "\tNodes: " << numNodes << ", Edges: " << numEdges << std::endl;
            for (int i = 0; i < numEdges; i++) {
                getline(reader, line);
                iss.str(line);
                iss.clear();
                unsigned short node1;
                unsigned short node2;
                unsigned short weight;
                char dump;
                if (!(iss >> node1 >> node2 >> dump >> dump >> dump)) {
                    std::cerr << "Failed to read edge ( " << i << "th edge) : " << line << std::endl;
                    throw std::runtime_error("EdgeParsingError");
                }
                
                if (!(iss >> weight)) {
                    weight = 1;
                }
                //weight = weight > 0 ? weight : 1;
                node1--;
                node2--;
                this->adjList[node1].push_back(Pair(node2,weight));
                this->adjList[node2].push_back(Pair(node1,weight));
            }
            reader.close();
            writeBin(*this, filename);
        }
        void loadEL(std::string filename) {
            std::ifstream reader(filename);
            unsigned short nodes = 0;
            std::string node1;
            std::string node2;
            unsigned short weight;
            std::string line;
            std::istringstream iss;
            while(getline(reader, line)) {
                iss.str(line);
                iss.clear();
                if (!(iss >> node1 >> node2)) {
                    std::cerr << "Failed to read edge: " << line << std::endl;
                    throw std::runtime_error("EdgeParsingError");
                }
                if (!(iss >> weight)) {
                    weight = 1;
                }
                weight = weight > 0 ? weight : 1;
                if (node_map.find(node1) == node_map.end()) {
                    rev_node_map[nodes] = node1;
                    node_map[node1] = nodes++;
                    adjList.push_back(std::vector<Pair>(0));
                }
                if (node_map.find(node2) == node_map.end()) {
                    rev_node_map[nodes] = node2;
                    node_map[node2] = nodes++;
                    adjList.push_back(std::vector<Pair>(0));
                }
                adjList[node_map[node1]].push_back(Pair(node_map[node2],weight));
                adjList[node_map[node2]].push_back(Pair(node_map[node1],weight));
            }
            this->nodes=nodes - 1;
            reader.close();
            writeBin(*this, filename);
        }
        //serialization
        
        friend class cereal::access;
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(CEREAL_NVP(node_map), CEREAL_NVP(rev_node_map), CEREAL_NVP(adjList),
                    CEREAL_NVP(nodes));
        }
        
        void writeBin(Graph& G, std::string& fileName)
        {
            std::string::size_type pos = fileName.find('.');
            if (pos != std::string::npos)
            {
                fileName = fileName.substr(0, pos);
            }
            std::ofstream ofs("networks/" + fileName + "/autogenerated/" + fileName + "_Shadow.bin", std::ofstream::binary | std::ofstream::out);
            if (ofs.is_open())
            {
                cereal::BinaryOutputArchive oArchive(ofs);
                oArchive(G);
                ofs.close();
            }
        }
        
        void readBin(Graph& G, std::string& fileName)
        {
            std::ifstream ifs("networks/" + fileName + "/autogenerated/" + fileName + "_Shadow.bin", std::ifstream::binary | std::ifstream::in);
            if (ifs.is_open()) {
                cereal::BinaryInputArchive iArchive(ifs);
                iArchive(G);
                ifs.close();
            }
        }
        
        public:
            typedef std::pair<unsigned short, unsigned short> Pair;
            std::unordered_map<std::string, unsigned short> node_map;
            std::unordered_map<unsigned short, std::string> rev_node_map;
            std::vector<std::vector<Pair> > adjList;
            unsigned short nodes = 0;
            
            void clear() {
                node_map.clear();
                for (unsigned int i = 0; i < adjList.size(); ++i) {
                    adjList[i].clear();
                }
                adjList.clear();
            }
            void load(const std::string& filename) {
                this->clear();

                std::string::size_type pos = filename.find('.');
                std::string fileName;
                if (pos != std::string::npos)
                {
                    fileName = filename.substr(0, pos);
                }
                std::string binName = "networks/" + fileName + "/autogenerated/" + fileName + "_Shadow.bin";
                
                struct stat st;
    
                if (stat(binName.c_str(), &st) != 0)
                {
                    readBin(*this, fileName);
                    return;
                }
                
                std::string extension = filesystem::suffix(filename);
                GraphType g = getGraphType(extension.c_str());
                if (g == GraphType::gw) {
                    this->loadGW(filename);
                } else {
                    this->loadEL(filename);
                }
            }
    };
    /**
        Expects a one liner format of alignment
        Basically one line text file, and the i-th element index of aligned node of i-th node in shadow network
    */
    void loadAlignment(std::vector<unsigned short> & alignment, std::string & s) {
        std::ifstream reader;
        std::string line;
        reader.open(s);
        std::getline(reader, line);
        std::istringstream iss(line);
        unsigned short nodeNo;
        while (iss >> nodeNo) {
            alignment.push_back(nodeNo);
        }
        reader.close();
    }
    //void loadEdgeListAlignment(std::vector<StringPair> &alignment, std::string & s){
    void loadEdgeListAlignment(std::unordered_map<std::string, std::string>  &alignment, std::string & s){
        std::ifstream reader;
        std::string line;
        reader.open(s);

        std::string a,b;
        while(reader >> a >> b){
            //alignment.push_back(StringPair(a,b));
            alignment[a] = b;
        }
        reader.close();
    }

}

void output_GW_Format( std::vector<std::unordered_map<int, int>> &adjList){
    std::cout << "LEDA.GRAPH" << std::endl;
    std::cout << "string" << std::endl;
    std::cout << "short" << std::endl;
    std::cout << "-2" << std::endl;

    // Print nodes
    std::cout << adjList.size() << std::endl;
    for (unsigned int i = 0; i < adjList.size(); ++i) {
        std::cout << "|{shadow" << i << "}|" << std::endl;
    }

    // compute numEdges
    int numEdges = 0;
    for (unsigned int i = 0; i < adjList.size(); ++i) 
        for(auto it = adjList[i].begin(); it != adjList[i].end(); ++it) 
            ++numEdges;
    
    // Print edges
    std::cout << numEdges << std::endl;
    for (unsigned int i = 0; i < adjList.size(); ++i) {
        for (auto it = adjList[i].begin(); it != adjList[i].end(); ++it) {
            if ((unsigned) it->first <= i) {
                // this should not happen
                // We only assigned edge value from lower index to higher index nodes
                std::cerr << it->first << " " << i << std::endl;
                throw std::runtime_error("Lost an edge");
                continue;
            }
	    // Stupid LEDA numbers nodes from 1, so +1 to the iterators.
            std::cout << i+1 << ' ' << it->first+1 << " 0 |{" << it->second << "}|" << std::endl;
        }
    }
}


void output_el_Format( std::vector<std::unordered_map<int, int>> &adjList, 
                            std::vector<std::string> &shadowNames, 
                            std::vector<bool> &nodeTypes,
                            bool nodesHaveTypes){
    std::cerr << "printing out .el" << std::endl;
    for (unsigned int i = 0; i < adjList.size(); ++i) {
        for (auto it = adjList[i].begin(); it != adjList[i].end(); ++it) {
            if ((unsigned) it->first <= i) {
                // this should not happen
                // We only assigned edge value from lower index to higher index nodes
                std::cerr << it->first << " " << i << std::endl;
                throw std::runtime_error("Lost an edge");
                continue;
            }
            
            if(nodesHaveTypes){
                bool firstType  = nodeTypes[i];
                bool secondType = nodeTypes[it->first];

                if(firstType == secondType){
                    // two of same kind cannot have edge
                    std::cerr << "type:  " << firstType << " " << secondType << std::endl;
                    std::cerr << "ids: " << i << " " << it -> first << std::endl;
                    std::cerr << "names: " << shadowNames[i] << " " << shadowNames[it -> first] << std::endl;
                    throw std::runtime_error("two of same kind cannot have edge");
                }
                
                if(firstType)
                    std::cout << shadowNames[it->first] << " " << shadowNames[i] << " " << it->second << std::endl;
                else
                    std::cout << shadowNames[i] << " " << shadowNames[it->first] << " " << it->second << std::endl;
            }
            else {
                std::cout << shadowNames[i] << " " << shadowNames[it->first] << " " << it->second << std::endl;
            }
        }      
    }
}


int main(int argc, const char** argv) {
    static_assert(fnv::hash("FNV Hash Test") == 0xF38B3DB9, "fnv1a_32::hash failure");
    assert(getGraphType(".gw") == GraphType::gw);

    args::ArgumentParser parser("Executable to create the shadow network");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::Group opt(parser, "Optional Flag", args::Group::Validators::DontCare);
    args::Flag nodesHaveTypesFlag(opt, "haveTypes", "Enables -nodes-have-types", {'n', "nodes-have-types"});
    args::ValueFlag<std::string> shadowNamesFlag(opt, "shadowNames", "Block size", {"shadowNames"});

    // args::Flag compact(parser, "compact", "Alignment file format", {'c',"compact"});
    // args::ValueFlag<std::string> format(parser, "format", "Output format", {'f',"format"});
    args::Group required(parser, "", args::Group::Validators::All);
    args::ValueFlag<int> shadowNodeSize(required, "shadowNodeSize", "number of shadowNodes", 
                                            {'s',"shadowNodeSize"});
    args::PositionalList<std::string> networks(required, "networks", "Network filenames");

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    if (args::get(networks).size() % 2 != 0) {
        std::cerr << "Number of alignments must equal number of networks" << std::endl;
        std::cerr << parser;
        return 2;
    }

    // TODO: allow for different alignment file formats
    // bool useCompact = false;
    // if (compact) {
    //     useCompact = true;
    // }
    //GraphType g = GraphType::gw;
    // if (format) {
    //     try {
    //         g = getGraphType(args::get(format).c_str());
    //     } catch (char const * e) {
    //         std::cerr << "--format must be el or gw" << std::endl;
    //         return 3;
    //     }
    // }
    std::vector<std::string> pos_args = args::get(networks);
    // Check Existance of files
    for(unsigned int i = 0; i < pos_args.size(); ++i){
        std::ifstream f(pos_args[i]);
        if(!f.good()){
            std::cerr << "Failed to load file: " << pos_args[i] << std::endl;
            throw std::runtime_error("Network file not found");
        }
        f.close();
    }

    // Read Shadow Names from file
    std::string shadowNamesFile = args::get(shadowNamesFlag);
    std::unordered_map<std::string, unsigned short> shadowName2Index; // TODO
    std::vector<std::string> shadowNames;
    std::vector<bool> nodeTypes;   // 0 gene, 1 miRNA
    if(args::get(nodesHaveTypesFlag)){
        std::ifstream f(shadowNamesFile);
        if(!f.good()){
            std::cerr << "Failed to load file: " << shadowNamesFile << std::endl;
            throw std::runtime_error("ShadowNames file not found");
        }
        bool type = 0;
        std::string line;
        std::string node;
        while(getline(f, line)) {
            std::istringstream iss(line);
            iss.str(line);
            iss.clear();
            iss >> node;
            if(!(iss >> type)){    
                type = 0;
                // assmuing its miRNA if starring with MNEST
                if(node.find("shadow") != std::string::npos)
                    type = 1;
            }
            shadowName2Index[node] = shadowNames.size(); //basically current index
            shadowNames.push_back(node);
            nodeTypes.push_back(type);
        }
        if(shadowNames.size() != (unsigned) args::get(shadowNodeSize)){
            std::cerr << "Number of shadow node names (" << shadowNames.size() 
                  << ") doest not match number of shadow nodes (" 
                  << args::get(shadowNodeSize) << ")" << std::endl;

            throw std::runtime_error("Number of ShadowNames != Shadow nodes count");
        }
    }
   

    // Separte network and alignment filenames
    std::vector<std::string> network_files(pos_args.size() / 2);
    std::vector<std::string> alignment_files(pos_args.size() / 2);
    int offset = pos_args.size() / 2;
    for (unsigned int i = 0; i < (pos_args.size() / 2); ++i) {
        network_files[i] = pos_args.at(i);
        alignment_files[i] = pos_args.at(i + offset);
    }
    assert(network_files.size() == alignment_files.size());

    std::cerr << "Starting to make Shadow graph " << std::endl;

    std::vector<std::unordered_map<int, int>> adjList(args::get(shadowNodeSize));
    shadow_graph::Graph tempGraph;
    for (unsigned int gi = 0; gi < network_files.size(); ++gi) {
        std::cerr << "graph " << gi << ": " << network_files.at(gi);        
        
        tempGraph.load(network_files.at(gi));

        std::cerr << ", nodes = " << tempGraph.adjList.size() << std::endl;

        // Load alignment
        std::vector<unsigned short> tempAligOneline(0); 
        //std::vector<StringPair> tempAlig(0);
        std::unordered_map<std::string, std::string> tempAlig;
        
        if(args::get(nodesHaveTypesFlag)){
            shadow_graph::loadEdgeListAlignment(tempAlig, alignment_files.at(gi));
        }
        else {
            shadow_graph::loadAlignment(tempAligOneline, alignment_files.at(gi));
        }

        unsigned int n = args::get(nodesHaveTypesFlag) ? tempAlig.size(): tempAligOneline.size();
     //   std::cerr << "~~ " << n << " " << tempGraph.adjList.size() << std::endl;
        assert(n == tempGraph.adjList.size()); // alignment size should be same as graph node count

        for (unsigned int peg = 0; peg < n; ++peg) {
            
            // neighbors of peg
            for (unsigned int j = 0; j < tempGraph.adjList[peg].size(); ++j) {
                unsigned short end_peg = tempGraph.adjList[peg][j].first;
                // only traverse each edge once
                if(peg  == end_peg){
                    std::cerr << "selfloop: " << peg << " " << end_peg << std::endl;
                    throw std::runtime_error("Selfloop");
                }
                if(peg > end_peg) {
                    continue;
                }

                unsigned short hole     = -1;
                unsigned short end_hole = -1;

                if(args::get(nodesHaveTypesFlag)){
                    // expects edge list alignment
                    std::string name1 = tempAlig[tempGraph.rev_node_map[peg]];  // tempAlig.at(peg).second;
                    std::string name2 = tempAlig[tempGraph.rev_node_map[end_peg]];// tempAlig.at(end_peg).second;
                    
                    bool test1 = shadowName2Index.find(name1) == shadowName2Index.end();
                    bool test2 = shadowName2Index.find(name2) == shadowName2Index.end();

                    if(test1){
                        std::cerr << "Gene name not found: " << name1 << std::endl;
                        throw std::runtime_error("Gene " + name1 + " not found in shadow network");
                    }
                    if(test2){
                        std::cerr << "MiRNA name not found: " << name2 << std::endl;
                        throw std::runtime_error("MiRNA " + name2 + " not found in shadow network");
                    }

                    hole     = shadowName2Index[name1];
                    end_hole = shadowName2Index[name2];

                    // std::cerr << ".. " << name1 << " " << name2 << " " << hole << " " << end_hole << std::endl;
                }
                else {
                    // expects one line interger alignment
                    hole     = tempAligOneline.at(peg);
                    end_hole = tempAligOneline.at(end_peg);
                }

                // we store edge wegiht only one side, from smaller node index to bigger
                if(hole > end_hole)
                    std::swap(hole, end_hole);
                        
                adjList[hole][end_hole] += tempGraph.adjList[peg][j].second;
                
                // debugging
                // if(adjList[hole][end_hole] >= 2){
                    // std::cerr << peg << " " << end_peg << std::endl;
                //     std::cerr << hole << " " << end_hole << " " << adjList[hole][end_hole] << " <- " << tempGraph.adjList[peg][j].second << std::endl;
                //     throw 1;
                // }
            }
        }
    }

    if(args::get(nodesHaveTypesFlag))
        output_el_Format(adjList, shadowNames, nodeTypes, true);
    else
        output_GW_Format(adjList);

    return 0;
}
