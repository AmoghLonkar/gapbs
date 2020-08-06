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

//Print info for all nodes in the graph
void PrintInfo(NewGraph &graph){
	for(int i = 0; i < graph.nodes.size(); i++){
		cout << "NodeID: " << graph.nodes[i].id << endl;
		cout << "Out Degree: " << (graph.nodes[i]).outDegree << endl;
		PrintVec((graph.nodes[i]).neighbors);
	}
}

//Print info for specified vertex
void PrintInfo(NewGraph &graph, NodeID i){
	cout << "NodeID: " << graph.nodes[i].id << endl;
	cout << "Out Degree: " << (graph.nodes[i]).outDegree << endl;
	PrintVec((graph.nodes[i]).neighbors);
}

//Convert GAPBS graph into NewGraph object 
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

//Reset labels to k after finishing calculation
void ResetLabels(NewGraph &g, vector<int> *labels, int k){
        for(NodeInfo node: g.nodes){
                labels->at(node.id) = k;
        }
}

//Getting edges in the vertex-induced subgraph
vector<NodeID> Intersection(vector<NodeID> &v1, vector<NodeID> &v2){
	vector<NodeID> intersect(v1.size() + v2.size());
	
	auto it = set_intersection(v1.begin(),
                          v1.end(),
                          v2.begin(),
                          v2.end(),
                          intersect.begin());
	
	
	intersect.resize(it - intersect.begin());
	return intersect;
}

//Generating vertex induced subgraph
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

//Recursive function to count cliques
void Listing(vector<int> *labels, NewGraph &graph, vector<NodeID> *clique, int *count, int k){
	
	//Set label
	int l = labels->at(clique->back());
	
	//Last level of recursion
	if (l == 2){
		for(int i = 0; i < graph.nodes.size(); i++){
			
			//Increment count based on the number of edges in the final level subgraph
			*count += graph.nodes[i].neighbors.size();
			/*
			for(NodeID node: graph.nodes[i].neighbors){
				*count++;
				cout << "Here" << endl;
				clique->clear();
			}*/
		}
		ResetLabels(graph, labels, k);
		clique->clear();
		return;
	}
	

	//Generate vertex induced subgraph	
	NewGraph subgraph = InducedGraph(graph, clique->back(), labels);
	
	//No out neighbors
	if(subgraph.nodes.empty()){
		return;
	}
	//Add current vertex to candidate clique
	clique->push_back(subgraph.nodes.front().id);	
	Listing(labels, subgraph, clique, count, k);
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
	
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to create graph struct: " << elapsed.count() << endl; 
	
	//Initialize Labels	
        vector<int> *labels = new vector<int>;

        labels->reserve(g.num_nodes());
        for(int i = 0; i < g.num_nodes(); i++){
                labels->push_back(k);
        }
	
	int *count = new int;
	*count = 0;

	vector<NodeID> *clique = new vector<NodeID>;
	clique->reserve(k);
	
	start = std::chrono::system_clock::now();
	for(NodeID u = 0; u < g.num_nodes(); u++){
		clique->push_back(u);
		Listing(labels, graph, clique, count, k);
	}
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	
	cout << "Number of cliques: " << *count << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
