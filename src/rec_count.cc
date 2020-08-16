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

//Will need to relabel
vector<NodeID> BitMapIntersection(vector<NodeID> I, Graph &g, NodeID v){
	
	bitset<SIZE> neighborhood;
	int y;
	for(NodeID u: g.out_neigh(v)){
		y = (int) u;
		neighborhood.set(y);
	}
	
	bitset<SIZE> bits_I;
	for(NodeID u: I){
		y = (int) u;
		bits_I.set(y);
	}

	bitset<SIZE> intersection = neighborhood & bits_I;

	vector<NodeID> I_prime;
	for(int i = 0; i < intersection.size(); i++){
		if(intersection[i] == 1){
			I_prime.push_back(i);
		}
	}

	return I_prime;	
}

int RecCount(Builder b, Graph &g, vector<NodeID> I, int l){
	if(l == 1){
		return I.size();
	}

	vector<int> T(g.num_nodes());

	for(NodeID u = 0; u < g.num_nodes(); u++){
		vector<NodeID> I_prime;
		I_prime = BitMapIntersection(I, g, u);
		
		Graph subgraph = b.InducedSubgraph(g, u);
		int t_prime = RecCount(b, subgraph, I_prime, l -1);
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
	int k = atoi(argv[3]);
	vector<NodeID> V(dag.num_nodes());
	iota(std::begin(V), std::end(V), 0);
	
	int count = RecCount(b, dag, V, k);
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);	
	
	cout << "Number of cliques: " << count << endl;
	cout << "Time to count cliques: " << elapsed.count() << endl; 

	return 0;
}	
