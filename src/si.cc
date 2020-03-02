#include <algorithm>
#include <iostream>
#include <vector>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"

using namespace std;


vector<Graph> InitEmbed(const Graph &g)
{
	vector<Graph> twoVertEmbed;
	for (NodeID u=0; u < g.num_nodes(); u++)
	{	
		for (NodeID v : g.out_neigh(u))
		{
			if(v > u)
			{
				break;
			}
			else
			{
				pvector<NodeID> edgeList;
				edgeList.push_back(u);
				edgeList.push_back(v);
				
				BuilderBase<NodeID> b();
				Graph temp = b.MakeGraphFromEL(edgeList);
				twoVertEmbed.push_back(temp);	
			}
		}
	}

	return twoVertEmbed;
}	

int main(int argc, char* argv[])
{
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs())
	{
		return -1;
	}
	Builder b(cli);
	Graph g = b.MakeGraph();
	vector<Graph> twoVertEmbed = InitEmbed(g);

	//g.PrintTopology(); 

	return 0;
}
