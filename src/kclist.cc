#include <algorithm>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <set>
#include <omp.h>
#include <queue>
#include <unordered_map>
#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"
#include "platform_atomics.h"

using namespace std;

struct Graph_Info{
	vector<int> ns;
	vector<vector<int>> d;
	vector<int> lab;
	vector<vector<int>> sub;
};

struct Min_Heap{
	int n;
	vector<pair<NodeID, int>> kv_pair;
};

void MkHeap(Graph &g, Min_Heap *heap){
	heap->n = g.num_nodes();
	
	vector<pair<NodeID, int>> nodeDegPairs(g.num_nodes(), make_pair(0, 0));
	for(NodeID u = 0; u < g.num_nodes(); u++){
		nodeDegPairs[u] = make_pair(u, g.out_degree(u));
	}

	heap->kv_pair = nodeDegPairs;
}

vector<int> OrdCore(Graph &g, Min_Heap *heap){
	vector<int> ranking(g.num_nodes());
	int n = g.num_nodes();
	int r = 0;

	for(int i = 0; i < n; i++){
		ranking[heap->kv_pair[i].first] = n - (++r);
	}

	return ranking;
}

void Init(Graph &g, Graph_Info *g_i, int k){
	vector<int> ns(k+1, 0);
	ns[k] = g.num_nodes();
	g_i->ns = ns;
	
	vector<vector<int>> d(k+1, vector<int>(g.num_nodes(), 0));
	for(NodeID i = 0; i < g.num_nodes(); i++){
		d[k][i] = g.out_degree(i);
	}
	g_i->d = d;

	vector<int> lab(g.num_nodes(), k);
	g_i->lab = lab;
	
	vector<vector<int>> sub(k+1, vector<int>(g.num_nodes(), 0));
	for(NodeID i = 0; i < g.num_nodes(); i++){
		sub[k][i] = i; 
	}
	g_i->sub = sub;	
}

void Listing(Graph &g, Graph_Info *g_i, int l, int *n){

	if(l == 2){
		for(int i = 0; i < g_i->ns[2]; i++){
			(*n) += g_i->d[2][i];
		}
		return;	
	}
	
	// For each node in g_l
	// Initializing vertex-induced subgraph
	for(int i = 0; i < g_i->ns[l]; i++){
		g_i->ns[l-1] = 0;

		for(NodeID neighbor: g.out_neigh(g_i->sub[l][i])){
			if(g_i->lab[neighbor] == l){
				g_i->lab[neighbor] = l-1;
				
				// Adding nodes to subgraph
				g_i->sub[l-1][g_i->ns[l-1]++] = neighbor;
			}
			
		}
		
		// Only proceed if there is potential for a clique
		//if(g_i->ns[l-1] >= l - 1){
		{
			// Building subgraph
			for(int j = 0; j < g_i->ns[l-1]; j++){
				g_i->d[l-1][j] = 0;
				int node = g_i->sub[l-1][j];

				// Looking at edges between nodes 
				for(NodeID neighbor: g.out_neigh(node)){
					// Node is present in the subgraph
					if(g_i->lab[neighbor] == l-1){
						(g_i->d[l-1][j])++;
					}
				}
			}
		}
		
		Listing(g, g_i, l-1, n);
		
		// Resetting labels	
		for(int k = 0; k < g_i->ns[l-1]; k++){
			int node = g_i->sub[l-1][k];
			g_i->lab[node] = l;
			g_i->d[l-1][k] = 0; 
		}
	}

}

int main(int argc, char* argv[]){
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs()){
		return -1;
	}

	Builder b(cli);
	Graph g = b.MakeGraph();
	
	Min_Heap bin_heap;
	MkHeap(g, &bin_heap);	

	auto start = std::chrono::system_clock::now();
	
	Graph dag = b.MakeDag(g);
	int k = atoi(argv[3]);
	Graph_Info graph_struct;
	Init(dag, &graph_struct, k);
	
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to create graph struct: " << elapsed.count() << "s" << endl; 
	
	start = std::chrono::system_clock::now();
	int n = 0;
	Listing(dag, &graph_struct, k, &n);
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	
	cout << "Number of cliques: " << n << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
