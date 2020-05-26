#include <fstream>
#include <vector>
#include <algorithm>
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "benchmark.h"
using namespace std;

int main(int argc, char* argv[]){

    CLBase cli(argc, argv, "subgraph isomorphism");
    if (!cli.ParseArgs()){
	return -1;
    }
    Builder b(cli);
    Graph g = b.MakeGraph();
    
    ofstream outFile(argv[3]); 
    	if (outFile.is_open()){
    	for(NodeID u = 0; u < g.num_nodes(); u++){
    	    for(NodeID v: g.out_neigh(u)){	
		if(g.out_degree(u) > g.out_degree(v)){
			outFile << v << " " << u << endl;
		}
		else if(g.out_degree(u) == g.out_degree(v) && u > v){
			outFile << v << " " << u << endl;
		}
		else if(g.out_degree(u) == g.out_degree(v) && v > u){
			outFile << u << " " << v << endl;
		}
		else{
			outFile << u << " " << v << endl;
		}		
	    }	
    	}
    outFile.close();		
    }
}
