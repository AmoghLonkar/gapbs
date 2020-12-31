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
	vector<vector<NodeID>> sub;
};

struct Min_Heap{
	int n;
	vector<pair<NodeID, int>> kv_pair;
};

void BubbleUp(Min_Heap *heap, int i){
	int j = (i-1)/2;
	while(i > 0){
		if(heap->kv_pair[j].second > heap->kv_pair[i].second){
			swap(heap->kv_pair[i], heap->kv_pair[j]);
			i = j;
			j = (i - 1)/2;
		}
		else{
			break;
		}
		
	}
}

void BubbleDown(Min_Heap *heap){
	int i = 0;
	int j;
	int j1 = 1;
	int j2 = 2;

	while(j1 < heap->n){
		if((j2 < heap->n) && (heap->kv_pair[j2].second < heap->kv_pair[j1].second)){
			j = j2;
		}
		else{
			j = j1;
		}
		if(heap->kv_pair[j].second < heap->kv_pair[i].second){
			swap(heap->kv_pair[i], heap->kv_pair[j]);
			i = j;
			j1 = 2*i + 1;
			j2 = j1 + 1;
			continue;
		}
		break;
	}
}

void Insert(Min_Heap *heap, pair<NodeID, int> nodeDegPair){
	heap->n++;
	heap->kv_pair[heap->n - 1] = nodeDegPair;
	BubbleUp(heap, heap->n - 1);
}

void UpdateHeap(Min_Heap *heap, NodeID node){
	for(int i = 0; i < heap->n; i++){
		if(heap->kv_pair[i].first == node){
			heap->kv_pair[i].second--;
		}
	}
	BubbleUp(heap, node);
}

pair<NodeID, int> PopMin(Min_Heap *heap){
	pair<NodeID, int> root = heap->kv_pair[0];
	heap->kv_pair[0] = heap->kv_pair[--(heap->n)];
	BubbleDown(heap);

	return root;
}

void MkHeap(Graph &g, Min_Heap *heap){
	heap->n = 0;
	
	vector<pair<NodeID, int>> nodeDegPairs(g.num_nodes());
	heap->kv_pair = nodeDegPairs;

	for(NodeID u = 0; u < g.num_nodes(); u++){
		pair<NodeID, int> nodeDegPair = make_pair(u, g.out_degree(u));
		Insert(heap, nodeDegPair);
	}
}

vector<int> OrdCore(Graph &g, Min_Heap *heap){
	vector<int> ranking(g.num_nodes());
	int n = g.num_nodes();
	int r = 0;

	MkHeap(g, heap);

	for(int i = 0; i < n; i++){
		pair<NodeID, int> root = PopMin(heap);
		ranking[root.first] = n - (++r);
		for(NodeID neighbor: g.out_neigh(root.first)){
			UpdateHeap(heap, neighbor);
		}

		
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
	
	vector<vector<NodeID>> sub(k+1, vector<int>(g.num_nodes(), 0));
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

		NodeID u = g_i->sub[l][i];
		for(NodeID neighbor: g.out_neigh(u)){
			if(g_i->lab[neighbor] == l){
				g_i->lab[neighbor] = l-1;
				
				// Adding nodes to subgraph
				g_i->sub[l-1][g_i->ns[l-1]++] = neighbor;
			}
			
		}
		
		// Only proceed if there is potential for a clique
		if(g_i->ns[l-1] >= l - 1){
			// Building subgraph
			for(int j = 0; j < g_i->ns[l-1]; j++){
				g_i->d[l-1][j] = 0;
				NodeID node = g_i->sub[l-1][j];

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
			NodeID node = g_i->sub[l-1][k];
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

	auto start = std::chrono::system_clock::now();
	
	Min_Heap bin_heap;
	vector<int> ranking = OrdCore(g, &bin_heap);
	
	//Graph dag = b.MakeDag(g);
	Graph dag = b.MakeDagFromRank(g, ranking);
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
