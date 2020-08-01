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

void PrintInfo(NewGraph &graph, NodeID i){
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

void UpdateLabels(Graph &g, NodeID u, vector<int> *labels){
        for(NodeID node: g.out_neigh(u)){
                labels->at(node)--;
        }
}

vector<NodeID> Intersection(vector<NodeID> &v1, vector<NodeID> &v2){
	vector<NodeID> intersect(v1.size() + v2.size());
	intersect.reserve(max(v1.size(), v2.size()));	
	
	auto it = set_intersection(v1.begin(),
                          v1.end(),
                          v2.begin(),
                          v2.end(),
                          intersect.begin());
	
	
	intersect.resize(it - intersect.begin());
	return intersect;
}

NewGraph InducedGraph(NewGraph &graph, NodeID vertex, vector<int> *labels){
	NewGraph subgraph;
	
	for(NodeID node: (graph.nodes[vertex]).neighbors){
		(subgraph.nodes).push_back(graph.nodes[node]);
		
		(subgraph.nodes.back()).neighbors = Intersection((subgraph.nodes.back()).neighbors, (graph.nodes[vertex]).neighbors);
		(subgraph.nodes.back()).outDegree = (subgraph.nodes.back().neighbors).size();	
		//Updating label
		labels->at(graph.nodes[node].id)--;
	}	

	return subgraph;	
}

void ReorderNeigh(NewGraph &graph, vector<int> *labels){
	
}


int main(int argc, char* argv[]){
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs()){
		return -1;
	}

	Builder b(cli);
	Graph g = b.MakeGraph();
	
	int k = atoi(argv[3]);

	auto start = std::chrono::system_clock::now();
	NewGraph graph = GetInfo(g, k);
	auto end = std::chrono::system_clock::now();

	//Initialize Labels	
        vector<int> *labels = new vector<int>;

        labels->reserve(g.num_nodes());
        for(int i = 0; i < g.num_nodes(); i++){
                labels->push_back(k);
        }
	
	cout << "Checking induced subgraph: " << endl;
	
	NewGraph induced = InducedGraph(graph, 0, labels);	
	for(int i = 0; i < induced.nodes.size(); i++){
		PrintInfo(induced, i);
	}
	
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
