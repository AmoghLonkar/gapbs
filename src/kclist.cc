#include <algorithm>
#include <iostream>
#include <fstream>
#include <functional>
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
	vector<vector<NodeID>> sub;
	vector<int> lab;
};

struct Min_Heap{
	vector<pair<NodeID, int>> nodeDegPairs;
	vector<int> index;
};

void BubbleDown(Min_Heap *heap, int n, int i){
	int root = i;
	int leftChild = 2*i + 1;
	int rightChild = 2*i + 2;

	if(leftChild < n && heap->nodeDegPairs[leftChild].second < heap->nodeDegPairs[root].second){
		root = leftChild;
	}
	
	if(rightChild < n && heap->nodeDegPairs[rightChild].second < heap->nodeDegPairs[root].second){
		root = rightChild;
	}

	if(root != i){
		swap(heap->nodeDegPairs[i], heap->nodeDegPairs[root]);
		swap(heap->index[heap->nodeDegPairs[i].first], heap->index[heap->nodeDegPairs[root].first]);
		BubbleDown(heap, n, root);
	}
}

void BubbleUp(Min_Heap *heap, int i){
	int parent = (i-1) / 2;
	if(i > 0 && heap->nodeDegPairs[parent].second > heap->nodeDegPairs[i].second){
		swap(heap->nodeDegPairs[i], heap->nodeDegPairs[parent]);
		swap(heap->index[heap->nodeDegPairs[i].first], heap->index[heap->nodeDegPairs[parent].first]);
		BubbleUp(heap, parent);
	}
}

void InsertNode(Min_Heap *heap, pair<NodeID, int> nodeDegPair){
	heap->index[nodeDegPair.first] = heap->nodeDegPairs.size();
	
	heap->nodeDegPairs.push_back(nodeDegPair);
	BubbleUp(heap, heap->nodeDegPairs.size() - 1);
}

void InitHeap(Graph &g, Min_Heap *heap){
	vector<NodeID> index(g.num_nodes(), -1);
	heap->index = index;

	for(NodeID u = 0; u < g.num_nodes(); u++){
		InsertNode(heap, make_pair(u, g.out_degree(u)));
	}			
}

pair<NodeID, int> PopMin(Min_Heap * heap){
	pair<NodeID, int> root = heap->nodeDegPairs[0];
	heap->index[root.first] = -1;

	heap->nodeDegPairs[0] = heap->nodeDegPairs.back();
	heap->index[heap->nodeDegPairs[0].first] = 0;

	heap->nodeDegPairs.pop_back();
	heap->index.pop_back();

	BubbleDown(heap, heap->nodeDegPairs.size(), 0);
	return root;
}

vector<int> OrdCore(Graph &g){
	vector<int> ranking(g.num_nodes());
	int n = g.num_nodes();
	int r = 0;

	Min_Heap heap;
	//Initialize Heap
	InitHeap(g, &heap);

	for(int i = n - 1; i >= 0; i--){
		pair<NodeID, int> root = PopMin(&heap);
		//Update ranking
		ranking[root.first] = n - (++r);

		//Update degrees for all neighbors of root
		for(NodeID neighbor: g.out_neigh(root.first)){
			int index = heap.index[neighbor];
			if(index != -1){
				heap.nodeDegPairs[index].second--;
				BubbleUp(&heap, index);
			}
		}
	}
	
	return ranking;
}

vector<int> GetRankFromFile(string fileName){
	ifstream rankFile(fileName);
	string temp;
	vector<int> ranking;
	while(getline(rankFile, temp)){
		ranking.push_back(stoi(temp));
	}
	rankFile.close();
		
	return ranking;
}

void Init(Graph &g, Graph_Info *g_i, int k){
	vector<int> ns(k+1, 0);
    ns[k] = g.num_nodes();	
	g_i->ns = ns;

	vector<vector<int>> d(k+1, vector<int>(g.num_nodes(), 0));
	for(NodeID u = 0; u < g.num_nodes(); u++){
		d[k][u] = g.out_degree(u);
	}
	g_i->d = d;
	
	vector<vector<NodeID>> sub(k+1, vector<NodeID>(g.num_nodes(), 0));
	for(NodeID u = 0; u < g.num_nodes(); u++){
		sub[k][u] = u; 
	}
	g_i->sub = sub;	

	vector<int> lab(g.num_nodes(), k);
	g_i->lab = lab;
}

void Listing(Graph &g, Graph_Info *g_i, int l, unsigned int *n){

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
		for(NodeID v: g.out_neigh(u)){
			if(g_i->lab[v] == l){
				g_i->lab[v] = l-1;
				
				// Adding nodes to subgraph
				g_i->sub[l-1][g_i->ns[l-1]++] = v;
			}
			
		}
		
		// Only proceed if there is potential for a clique
		//if(g_i->ns[l-1] >= l - 1){
		// Building subgraph
		for(int j = 0; j < g_i->ns[l-1]; j++){
			g_i->d[l-1][j] = 0;
			NodeID u = g_i->sub[l-1][j];

			// Looking at edges between nodes 
			for(NodeID v: g.out_neigh(u)){
				// Node is present in the subgraph
				if(g_i->lab[v] == l-1){
					(g_i->d[l-1][j])++;
				}
			}
		}
		//}
		
		Listing(g, g_i, l-1, n);
		
		// Resetting labels	
		for(int i = 0; i < g_i->ns[l-1]; i++){
			NodeID node = g_i->sub[l-1][i];
			g_i->lab[node] = l;
		}
	}

}

void PrintCliqueCount(int k, unsigned int *n){
	cout << "Number of " << k <<"-cliques: " << *n << endl;
}

void ListingVerifier(){
	cout << "Verifier has not been implemented yet" << endl;
}

int main(int argc, char* argv[]){
	CLKClique cli(argc, argv, "k-clique counting", 3, "");
	if (!cli.ParseArgs()){
		return -1;
	}
	
	Builder b(cli);
	Graph g = b.MakeGraph();
	Graph_Info graph_struct;
	
	auto start = std::chrono::system_clock::now();
	vector<int> ranking;
	if(cli.file_name() == ""){
		ranking = OrdCore(g);
	}
	else{
		ranking = GetRankFromFile(cli.file_name());
	}

	for(auto elem: ranking){
		cout << "Ranking: " << elem << endl;
	}

	Graph dag = b.RelabelByRank(g, ranking);
	Init(dag, &graph_struct, cli.clique_size());

	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to create graph struct: " << elapsed.count() << "s" << endl; 
	
	start = std::chrono::system_clock::now();
	unsigned int n = 0;
	Listing(dag, &graph_struct, cli.clique_size(), &n);
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	
	PrintCliqueCount(cli.clique_size(), &n);
	//BenchmarkKernel(cli, g, Listing, PrintCliqueCount, ListingVerifier);
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
