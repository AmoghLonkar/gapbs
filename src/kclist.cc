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
	NodeID id;
	int outDegree;
	vector<NodeID> neighbors;
};

struct NewGraph{
	vector<NodeInfo> nodes;
};

void PrintVec(vector<NodeID> vec)
{
	for(NodeID i : vec)
	{
		cout << i << " ";
	}
	cout << endl;
}

void PrintInfo(NewGraph graph, NodeID i){
	cout << "NodeID: " << graph.nodes[i].id << endl;
	cout << "Out Degree: " << (graph.nodes[i]).outDegree << endl;
	PrintVec((graph.nodes[i]).neighbors);
}

NewGraph GetInfo(Graph &g, int size){
	NewGraph graphInfo;
	(graphInfo.nodes).reserve(g.num_nodes());

	vector<NodeID> neighborhood;
	neighborhood.reserve(100);

	NodeInfo temp;

	for(NodeID u = 0; u < g.num_nodes(); u++){
		for(NodeID node: g.out_neigh(u)){
			neighborhood.push_back(node);
		}

		temp.id = u;
		temp.outDegree = g.out_degree(u);
		temp.neighbors = neighborhood;
		neighborhood.clear();

		(graphInfo.nodes).push_back(temp);
	}	

	return graphInfo;
}

vector<int> InitLabels(Graph &g, int k){
	vector<int> labels;
	labels.reserve(g.num_nodes());
	for(int i = 0; i < g.num_nodes(); i++){
		labels[i] = k;
	}
	
	return labels;
}

NewGraph InducedGraph(NewGraph graph, NodeID vertex){
	NewGraph subgraph;
	
	for(NodeID node: (graph.nodes[vertex]).neighbors){
		(subgraph.nodes).push_back(graph.nodes[node]);
	}

	return subgraph;	
}

int main(int argc, char* argv[]){
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs()){
		return -1;
	}

	Builder b(cli);
	Graph g = b.MakeGraph();
	
	auto start = std::chrono::system_clock::now();
	NewGraph graph = GetInfo(g, atoi(argv[3]));
	auto end = std::chrono::system_clock::now();
	
	cout << "Checking induced subgraph: " << endl;
	
	NewGraph induced = InducedGraph(graph, 0);	
	for(int i = 0; i < induced.nodes.size(); i++){
		PrintInfo(induced, i);
	}
	
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
