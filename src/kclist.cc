#include <algorithm>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <set>
#include <omp.h>
#include <unordered_map>
#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"
#include "platform_atomics.h"

using namespace std;

struct NodeInfo{
	int outDegree;
	vector<NodeID> neighbors;
	int label;
};

vector<NodeInfo> GetInfo(Graph &g, int size){
	vector<NodeInfo> graphInfo;
	graphInfo.reserve(g.num_nodes());

	vector<NodeID> neighborhood;
	neighborhood.reserve(100);

	NodeInfo temp;

	for(NodeID u = 0; u < g.num_nodes(); u++){
		for(NodeID node: g.out_neigh(u)){
			neighborhood.push_back(node);
		}
		
		temp.outDegree = g.out_degree(u);
		temp.neighbors = neighborhood;
		temp.label = size;
		neighborhood.clear();

		graphInfo.push_back(temp);
	}	

	return graphInfo;
}
	
void PrintVec(vector<NodeID> vec)
{
	for(NodeID i : vec)
	{
		cout << i << " ";
	}
	cout << endl;
}

void PrintInfo(vector<NodeInfo> graph, NodeID i){
	cout << "NodeID: " << i << endl;
	cout << "Label: " << graph[i].label << endl;
	cout << "Out Degree: " << graph[i].outDegree << endl;
	PrintVec(graph[i].neighbors);
}

int main(int argc, char* argv[]){
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs()){
		return -1;
	}

	Builder b(cli);
	Graph g = b.MakeGraph();
	
	auto start = std::chrono::system_clock::now();
	vector<NodeInfo> graph = GetInfo(g, atoi(argv[3]));
	PrintInfo(graph, 2);
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
