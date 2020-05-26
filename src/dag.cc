#include <fstream>
#include <vector>
#include <algorithm>
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "benchmark.h"


int main(int argc, char* argv[]){

	CLBase cli(argc, argv, "subgraph isomorphism");
    if (!cli.ParseArgs()){
		return -1;
    }
    Builder b(cli);
    Graph g = b.MakeGraph();
    g.PrintTopology();
}
