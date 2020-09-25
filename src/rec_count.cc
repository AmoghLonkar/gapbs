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

vector<NodeID> Intersection(vector<NodeID> v1, vector<NodeID> v2){
	vector<NodeID> intersect(v1.size() + v2.size());
		
	auto it = set_intersection(v1.begin(),
			           v1.end(),
				   v2.begin(),
				   v2.end(),
				   intersect.begin());
				
	intersect.resize(it - intersect.begin());
	return intersect;
}

vector<NodeID> MyIntersection(vector<NodeID> v1, vector<NodeID> v2){

	auto first1 = v1.begin();
	auto last1 = v1.end();
	auto first2 = v2.begin();
	auto last2 = v2.end();

	vector<NodeID> intersect;
	intersect.reserve(v1.size() + v2.size());

	while (first1!=last1 && first2!=last2){
      		if (*first1<*first2) ++first1;
    		else if (*first2<*first1) ++first2;
    		else {
	      		intersect.push_back(*first1);
	        	++first1; ++first2;
	        }
  	}

  	return intersect;
}

unordered_map<NodeID, vector<NodeID>> InducedSubgraph(unordered_map<NodeID, vector<NodeID>> &graph, NodeID vertex){
	unordered_map<NodeID, vector<NodeID>> subgraph;

	vector<NodeID> nodes = graph.at(vertex);
	for(NodeID node: nodes){
		subgraph[node] = Intersection(graph.at(node), graph.at(vertex));
		//subgraph[node] = MyIntersection(graph.at(node), graph.at(vertex));
	}	
	
	return subgraph;
}

int RecCount(unordered_map<NodeID, vector<NodeID>> &g, vector<NodeID> I, int l){
	vector<int> T(g.size());

	for(NodeID u = 0; u < g.size(); u++){
		vector<NodeID> I_prime = g.at(I[u]);
		
		int t_prime = 0;	
		if(l != 2 && I_prime.size() >= l - 1){
		//if(l != 2){
			unordered_map<NodeID, vector<NodeID>> subgraph = InducedSubgraph(g, I[u]);
			t_prime = RecCount(subgraph, I_prime, l - 1);
		}
		
		else if(l !=2 && I_prime.size() < l - 1){
			t_prime = 0;
		}
		
		else{
			t_prime = I_prime.size();
		}
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
	
	start = std::chrono::system_clock::now();
	int k = atoi(argv[3]);
	vector<NodeID> V(dag.num_nodes());
	iota(std::begin(V), std::end(V), 0);
	
	int count = RecCount(graph, V, k);
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);	
	
	cout << "Number of cliques: " << count << endl;
	cout << "Time to count cliques: " << elapsed.count() << endl; 
	
	return 0;
}	
