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

struct Edge{
	int source;
	int dest;
};

struct Graph_Info{
	int *ns;
	int e;
	Edge *edges;
	int **d;
	int *cd;
	int *adj_list;
	int **sub;
	int *lab;
};

struct Min_Heap{
	int n;
	int *ptrs;
	pair<NodeID, int> *kv_pair;
};

void GetEdges(Graph &g, Graph_Info *g_i){
	g_i->edges = new Edge[g.num_edges()];
	int e = 0;
	int index = 0;
	for(int i = 0; i < g.num_nodes(); i++){
		for(int neighbor: g.out_neigh(i)){
			Edge temp;
			temp.source = i;
			temp.dest = neighbor;
			g_i->edges[index++] = temp;
			e++;
		}	
	}
	
	g_i->e = e;	
}

void Relabel(Graph_Info *g_i, vector<int> ranking){
	for(int i = 0; i < g_i->e; i++){
		int source_rank = ranking[g_i->edges[i].source];
		int dest_rank = ranking[g_i->edges[i].dest];
		if(source_rank < dest_rank){
			swap(source_rank, dest_rank);
		}
		g_i->edges[i].source = source_rank;
		g_i->edges[i].dest = dest_rank;
	}
}

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
	g_i->ns = new int[k+1];
	g_i->ns[k] = g.num_nodes();

	int *d = new int[g.num_nodes()];
	for(int i = 0; i < g.num_nodes(); i++){
		d[i] = g.out_degree(i);
	}

	g_i->d = new int*[k+1];
	for(int i = 2; i < k; i++){
		g_i->d[i] = new int[g.num_nodes()];
	}
	g_i->d[k] = d;

	g_i->cd = new int[g.num_nodes()+1];
	g_i->cd[0] = 0;
	for(int i = 1; i < g.num_nodes() + 1; i++){
		g_i->cd[i] = g_i->cd[i-1] + g.out_degree(i - 1);
	}

	g_i->adj_list = new int[g.num_edges()];
	int index = 0;
	for(int i = 0; i < g.num_nodes(); i++){
		for(int neighbor: g.out_neigh(i)){
			g_i->adj_list[index++] = neighbor;
		}
	}

	g_i->lab = new int[g.num_nodes()];
	for(int i = 0; i < g.num_nodes(); i++){
		g_i->lab[i] = k;
	}
	
	int *sub = new int[g.num_nodes()];
	for(int i = 0; i < g.num_nodes(); i++){
		sub[i] = i;
	}

	g_i->sub = new int*[k+1];
	for(int i = 2; i < k; i++){
		g_i->sub[i] = new int[g.num_nodes()];
	}
	g_i->sub[k] = sub;
}

void FreeMem(Graph_Info *g_i, int k){
	delete[] g_i->ns;
	delete[] g_i->cd;
	delete[] g_i->adj_list;
	delete[] g_i->lab;

	for(int i = 2; i < k+1; i++){
		delete []g_i->d[i];
		delete []g_i->sub[i];
	}
}

void Listing(Graph_Info *g_i, int l, int *n, double *loop_time){
	int i, j, k, bound;
	int node, neighbor, u;

	if(l == 2){
		for(int i = 0; i < g_i->ns[2]; i++){
			u = g_i->sub[2][i];
			(*n) += g_i->d[2][u];
		}
		return;	
	}
	
	// For each node in g_l
	// Initializing vertex-induced subgraph
	for(i = 0; i < g_i->ns[l]; i++){
		u = g_i->sub[l][i];
		g_i->ns[l-1] = 0;
		
		bound = g_i->cd[u] + g_i->d[l][u];

		for(j = g_i->cd[u]; j < bound; j++){
			neighbor = g_i->adj_list[j];
			if(g_i->lab[neighbor] == l){
				g_i->lab[neighbor] = l-1;
				
				// Adding nodes to subgraph
				g_i->sub[l-1][g_i->ns[l-1]++] = neighbor;
				g_i->d[l-1][neighbor] = 0;
				//g_i->d[l-1][g_i->ns[l-1]++] = 0;
			}
		}
		

		// Computing degrees
		for(j = 0; j < g_i->ns[l-1]; j++){
			node = g_i->sub[l-1][j];
			bound = g_i->cd[node] + g_i->d[l][node];
			
			// Looking at edges between nodes
			for(k = g_i->cd[node]; k < bound; k++){
				neighbor = g_i->adj_list[k];
				
				// Node is present in the subgraph
				if(g_i->lab[neighbor] == l-1){
					(g_i->d[l-1][node])++;
					//(g_i->d[l-1][j])++;
				}
				else{
					//auto start = std::chrono::system_clock::now();
					g_i->adj_list[k--] = g_i->adj_list[--bound];
					g_i->adj_list[bound] = neighbor;
					//auto end = std::chrono::system_clock::now();
					//auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
					//(*loop_time) += elapsed.count();
				}
			}
		}

		Listing(g_i, l-1, n, loop_time);
		
		// Resetting labels	
		for(j = 0; j < g_i->ns[l-1]; j++){
			node = g_i->sub[l-1][j];
			g_i->lab[node] = l;
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
	Graph_Info graph_struct;
	GetEdges(g, &graph_struct);
	
	auto start = std::chrono::system_clock::now();
	
	Min_Heap bin_heap;
	vector<int> ranking = OrdCore(g, &bin_heap);
	Relabel(&graph_struct, ranking);
	Graph dag = b.MakeDagFromRank(g, ranking);
	
	//Graph dag = b.MakeDag(g);
	int k = atoi(argv[3]);

	Init(dag, &graph_struct, k);

	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to create graph struct: " << elapsed.count() << "s" << endl; 
	
	cout << "Adjacency List: ";
	for(int i = 0; i < dag.num_edges(); i++){
		cout << graph_struct.adj_list[i] << " ";  
	}
	cout << endl;

	start = std::chrono::system_clock::now();
	int n = 0;
	double loop_time = 0;
	Listing(&graph_struct, k, &n, &loop_time);
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	
	FreeMem(&graph_struct, k);
	cout << "Number of cliques: " << n << endl;
	//cout << "Total time to calculate degrees: " << loop_time << "s" << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
