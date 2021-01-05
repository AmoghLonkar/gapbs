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
	vector<int> cd;
	vector<NodeID> adj_list;
	vector<vector<NodeID>> sub;
	vector<int> lab;
};

struct Min_Heap{
	int n;
	int *ptrs;
	pair<NodeID, int> *kv_pair;
};

void InitHeap(Min_Heap *heap, int num_nodes){
	heap->n = 0;
	heap->ptrs = new int[num_nodes];

	for(int i = 0; i < num_nodes; i++){
		heap->ptrs[i] = -1;
	}

	heap->kv_pair = new pair<NodeID, int>[num_nodes];
}

void Swap(Min_Heap *heap, int i, int j){
	pair<NodeID, int> temp_kv = heap->kv_pair[i];
	int temp_ptr = heap->ptrs[i];

	heap->ptrs[heap->kv_pair[i].first] = heap->ptrs[heap->kv_pair[j].first];
	heap->kv_pair[i] = heap->kv_pair[j];

	heap->ptrs[heap->kv_pair[j].first] = temp_ptr;
	heap->kv_pair[j] = temp_kv;
}

void BubbleUp(Min_Heap *heap, int i){
	int j = (i-1)/2;
	while(i > 0){
		if(heap->kv_pair[j].second > heap->kv_pair[i].second){
			Swap(heap, i, j);
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
			Swap(heap, i, j);
			i = j;
			j1 = 2*i + 1;
			j2 = j1 + 1;
			continue;
		}
		break;
	}
}

void Insert(Min_Heap *heap, pair<NodeID, int> nodeDegPair){
	heap->ptrs[nodeDegPair.first] = (heap->n)++;
	heap->kv_pair[heap->n - 1] = nodeDegPair;
	BubbleUp(heap, heap->n - 1);
}

void UpdateHeap(Min_Heap *heap, NodeID node){
	int ptr = heap->ptrs[node];
	if(ptr != -1){
		heap->kv_pair[ptr].second--;
		BubbleUp(heap, node);
	}	
}

pair<NodeID, int> PopMin(Min_Heap *heap){
	pair<NodeID, int> root = heap->kv_pair[0];
	heap->ptrs[root.first] = -1;
	heap->kv_pair[0] = heap->kv_pair[--(heap->n)];
	heap->ptrs[heap->kv_pair[0].first] = 0;
	BubbleDown(heap);

	return root;
}

void MkHeap(Graph &g, Min_Heap *heap){
	InitHeap(heap, g.num_nodes());

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

	delete[] heap->ptrs;
	delete[] heap->kv_pair;

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

	vector<int> cd(g.num_nodes() + 1, 0);
	for(NodeID i = 1; i < g.num_nodes() + 1; i++){
		cd[i] = cd[i-1] + g.out_degree(i-1);
	}
	g_i->cd = cd;

	vector<NodeID> adj;
	
	for(NodeID i = 0; i < g.num_nodes(); i++){
		for(NodeID neighbor: g.out_neigh(i)){
			adj.push_back(neighbor);
		}
	}

	g_i->adj_list = adj;

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
			NodeID u = g_i->sub[2][i];
			(*n) += g_i->d[2][u];
		}
		return;	
	}
	
	// For each node in g_l
	// Initializing vertex-induced subgraph
	for(int i = 0; i < g_i->ns[l]; i++){
		g_i->ns[l-1] = 0;

		NodeID u = g_i->sub[l][i];
		
		
		int bound = g_i->cd[u] + g_i->d[l][u];
		for(int j = g_i->cd[u]; j < bound; j++){
			NodeID neighbor = g_i->adj_list[j];
			if(g_i->lab[neighbor] == l){
				g_i->lab[neighbor] = l-1;
				
				// Adding nodes to subgraph
				g_i->sub[l-1][g_i->ns[l-1]] = neighbor;
				g_i->ns[l-1]++;
				//g_i->d[l-1][neighbor] = 0;
			}
		}
		
		// Only proceed if there is potential for a clique
		//if(g_i->ns[l-1] >= l - 1){
		{
			// Building subgraph
			for(int j = 0; j < g_i->ns[l-1]; j++){
				g_i->d[l-1][j] = 0;
				NodeID node = g_i->sub[l-1][j];
				bound = g_i->cd[node] + g_i->d[l][node];
				
				// Looking at edges between nodes
				for(int k = g_i->cd[node]; k < bound; k++){
					NodeID neighbor = g_i->adj_list[k];
					
					// Node is present in the subgraph
					if(g_i->lab[neighbor] == l-1){
						(g_i->d[l-1][node])++;
					}
					else{
						bound--;
						g_i->adj_list[k] = g_i->adj_list[bound];
						k--;
						g_i->adj_list[bound] = neighbor;
					}
				}
			}
		}

		Listing(g, g_i, l-1, n);
		
		// Resetting labels	
		for(int u = 0; u < g_i->ns[l-1]; u++){
			NodeID node = g_i->sub[l-1][u];
			g_i->lab[node] = l;
			//g_i->d[l-1][u] = 0; 
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
	
	//Min_Heap bin_heap;
	//vector<int> ranking = OrdCore(g, &bin_heap);
	//Graph dag = b.MakeDagFromRank(g, ranking);
	
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
