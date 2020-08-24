#include <algorithm>
#include <bits/stdc++.h>
#include <chrono>
#include <iostream>
#include <omp.h>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"
#include "platform_atomics.h"

using namespace std;
#define SIZE 100000

vector<NodeID> Intersection(vector<NodeID> &v1, Graph &g, NodeID v){
	vector<NodeID> intersect(v1.size() + g.out_degree(v));
	
	auto it = set_intersection(v1.begin(),
	                           v1.end(),
	                           g.out_neigh(v).begin(),
	                           g.out_neigh(v).end(),
	                           intersect.begin());
				
				
	intersect.resize(it - intersect.begin());
	return intersect;
}


unordered_map<NodeID, vector<NodeID>> InitGraph(Graph &g){
	unordered_map<NodeID, vector<NodeID>> graph;
	
	for(NodeID u = 0; u < g.num_nodes(); u++){
		vector<NodeID> neighbors;
		neighbors.reserve(g.out_degree(u));
		
		for(NodeID v: g.out_neigh(u)){
			neighbors.push_back(v);
		}
		
		graph[u] = neighbors;	
	}

	return graph;
}
/*
unordered_map<NodeID, vector<NodeID>> InducedSubgraph(unordered_map<NodeID, vector<NodeID> &graph>, NodeID vertex){
	unordered_map<NodeID, vector<NodeID>> subgraph;
	
	return subgraph;
}
*/

int RecCount(Builder b, Graph &g, vector<NodeID> I, int l){
	if(l == 1){
		return I.size();
	}

	vector<int> T(g.num_nodes());

	for(NodeID u = 0; u < g.num_nodes(); u++){
		vector<NodeID> I_prime;
		//I_prime = BitMapIntersection(I, g, u);
		I_prime = Intersection(I, g, u);
		
		Graph subgraph = b.InducedSubgraph(g, u);
		int t_prime = RecCount(b, subgraph, I_prime, l - 1);
		T[u] = t_prime;
	}

	int t = accumulate(T.begin(), T.end(), 0); 
	return t;
}

int main(int argc, char* argv[]){
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs()){
		return -1;
	}

	Builder b(cli);
	Graph g = b.MakeGraph();
	
	auto start = std::chrono::system_clock::now();
	Graph dag = b.MakeDag(g);
	auto end = std::chrono::system_clock::now();

	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to create DAG: " << elapsed.count() << endl; 
	
	start = std::chrono::system_clock::now();
	unordered_map<NodeID, vector<NodeID>> graph = InitGraph(dag);	
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);	
	
	cout << "Time to create graph data structure: " << elapsed.count() << endl;
	/*
	start = std::chrono::system_clock::now();
	int k = atoi(argv[3]);
	vector<NodeID> V(dag.num_nodes());
	iota(std::begin(V), std::end(V), 0);
	
	int count = RecCount(b, dag, V, k);
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);	
	
	cout << "Number of cliques: " << count << endl;
	cout << "Time to count cliques: " << elapsed.count() << endl; 
	*/
	return 0;
}	
