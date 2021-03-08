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
#include <numeric>
#include <unordered_map>
#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"
#include "platform_atomics.h"

using namespace std;

struct Graph_Info{
	vector<int64_t> ns;
	vector<int64_t> cd;
	vector<NodeID> adj_list;
	vector<vector<int64_t>> d;
	vector<vector<NodeID>> sub;
	vector<int64_t> lab;
};

struct Min_Heap{
	vector<pair<NodeID, int64_t>> nodeDegPairs;
	vector<int64_t> index;
};


void BubbleDown(Min_Heap *heap, int64_t n, int64_t i){
	int64_t root = i;
	int64_t leftChild = 2*i + 1;
	int64_t rightChild = 2*i + 2;

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

void BubbleUp(Min_Heap *heap, int64_t i){
	int64_t parent = (i-1) / 2;
	if(i > 0 && heap->nodeDegPairs[parent].second > heap->nodeDegPairs[i].second){
		swap(heap->nodeDegPairs[i], heap->nodeDegPairs[parent]);
		swap(heap->index[heap->nodeDegPairs[i].first], heap->index[heap->nodeDegPairs[parent].first]);
		BubbleUp(heap, parent);
	}
}

void InsertNode(Min_Heap *heap, pair<NodeID, int64_t> nodeDegPair){
	heap->index[nodeDegPair.first] = heap->nodeDegPairs.size();
	
	heap->nodeDegPairs.push_back(nodeDegPair);
	BubbleUp(heap, heap->nodeDegPairs.size() - 1);
}

void InitHeap(Graph &g, Min_Heap *heap){
	vector<int64_t> index(g.num_nodes(), -1);
	heap->index = index;
	index.clear();

	for(NodeID u = 0; u < g.num_nodes(); u++){
		InsertNode(heap, make_pair(u, g.out_degree(u)));
	}			
}

pair<NodeID, int64_t> PopMin(Min_Heap *heap){
	pair<NodeID, int64_t> root = heap->nodeDegPairs[0];
	heap->index[root.first] = -1;

	heap->nodeDegPairs[0] = heap->nodeDegPairs.back();
	heap->index[heap->nodeDegPairs[0].first] = 0;

	heap->nodeDegPairs.pop_back();
	heap->index.pop_back();

	BubbleDown(heap, heap->nodeDegPairs.size(), 0);
	return root;
}

vector<int64_t> OrdCore(Graph &g){
	vector<int64_t> ranking(g.num_nodes());
	int64_t n = g.num_nodes();
	int64_t r = 0;

	Min_Heap heap;
	//Initialize Heap
	InitHeap(g, &heap);

	for(int64_t i = n - 1; i >= 0; i--){
		pair<NodeID, int64_t> root = PopMin(&heap);
		//Update ranking
		ranking[root.first] = n - (++r);

		//Update degrees for all neighbors of root
		for(NodeID neighbor: g.out_neigh(root.first)){
			int64_t index = heap.index[neighbor];
			if(index != -1){
				heap.nodeDegPairs[index].second--;
				BubbleUp(&heap, index);
			}
		}
	}
	
	/*
	for(auto elem: ranking){
		cout << "Ranking:" << elem << endl;
	}*/
	
	return ranking;
}

vector<int64_t> GetRankFromFile(string fileName){
	ifstream rankFile(fileName);
	string temp;
	vector<int64_t> ranking;
	while(getline(rankFile, temp)){
		ranking.push_back(stoi(temp));
	}
	rankFile.close();
		
	return ranking;
}

void GenGraph(Graph& g, Graph_Info *g_i, vector<int64_t> ranking, int64_t k){
	
	//Get list of edges
	vector<pair<NodeID, NodeID>> edges;
	for(NodeID u = 0; u < g.num_nodes(); u++){
		for(NodeID v: g.out_neigh(u)){
			if(ranking[u] < ranking[v]){
				edges.push_back(make_pair(ranking[v], ranking[u]));
			}
			else{
				edges.push_back(make_pair(ranking[u], ranking[v]));
			}
		}
	}
	vector<int64_t> ns(k+1, 0);
    ns[k] = g.num_nodes();	
	g_i->ns = ns;
	ns.clear();

	vector<vector<int64_t>> d(k+1, vector<int64_t>(g.num_nodes(), 0));
	for(int64_t i = 0; i < g.num_edges(); i++){
		d[k][edges[i].first]++;
	}

	vector<int64_t> cd(g.num_nodes());
	partial_sum(d[k].begin(), d[k].end(), cd.begin());
	cd.insert(cd.begin(), 0);
	fill(d[k].begin(), d[k].end(), 0);

	vector<NodeID> adj_list(g.num_edges(), 0);
	for(int64_t i = 0; i < g.num_edges(); i++){
		adj_list[cd[edges[i].first] + d[k][edges[i].first]++] = edges[i].second;
	}

	g_i->d = d;
	d.clear();
	g_i->cd = cd;
	cd.clear();
	g_i->adj_list = adj_list;
	adj_list.clear();
	
	vector<vector<NodeID>> sub(k+1, vector<NodeID>(g.num_nodes(), 0));
	for(NodeID u = 0; u < g.num_nodes(); u++){
		sub[k][u] = u; 
	}
	g_i->sub = sub;	
	sub.clear();

	vector<int64_t> lab(g.num_nodes(), k);
	g_i->lab = lab;
	lab.clear();
} 

bool CyclicityCheck(Graph_Info *g_i, NodeID node, vector<bool> visited, vector<bool> recStack){
	if(visited[node] == false){
		visited[node] = true;
		recStack[node] = true;
		
		set<NodeID> neighbors(g_i->adj_list.begin() + g_i->cd[node], g_i->adj_list.begin() + g_i->cd[node + 1]);
		for(NodeID neighbor: neighbors){
			if(visited[neighbor] && CyclicityCheck(g_i, neighbor, visited, recStack)){
				return true;
			}
			else if(recStack[neighbor]){
				return true;
			}
		}
	}

	recStack[node] = false;
	return false;
}

bool DAGCheck(Graph_Info *g_i, int k){
	int n = g_i->ns[k];
	vector<bool> visited(n, false);
	vector<bool> recStack(n, false);

	for(int i = 0; i < g_i->ns[k]; i++){
		if(CyclicityCheck(g_i, i, visited, recStack)){
			return true;
		}
	}

	return false;
}

void Listing(Graph_Info *g_i, int64_t l, int64_t *n){
	
	if(l == 2){
		for(int64_t i = 0; i < g_i->ns[2]; i++){
			(*n) += g_i->d[2][g_i->sub[2][i]];
		}
		return;	
	}
	
	// For each node in g_l
	// Initializing vertex-induced subgraph
	for(int64_t i = 0; i < g_i->ns[l]; i++){
		g_i->ns[l-1] = 0;

		NodeID u = g_i->sub[l][i];
		for(int64_t j = g_i->cd[u]; j < g_i->cd[u] + g_i->d[l][u]; j++){
			NodeID v = g_i->adj_list[j];
			if(g_i->lab[v] == l){
				g_i->lab[v] = l-1;
				
				// Adding nodes to subgraph
				g_i->sub[l-1][g_i->ns[l-1]++] = v;
				g_i->d[l-1][v] = 0;
			}
		}
		
		// Only proceed if there is potential for a clique
		//if(g_i->ns[l-1] >= l - 1){
		// Building subgraph
		for(int64_t j = 0; j < g_i->ns[l-1]; j++){
			NodeID u = g_i->sub[l-1][j];
			
			// Looking at edges between nodes 
			int64_t neighborhood_size = g_i->cd[u] + g_i->d[l][u];
			for(int64_t k = g_i->cd[u]; k < neighborhood_size; k++){
				NodeID v = g_i->adj_list[k];
				// Node is present in the subgraph
				if(g_i->lab[v] == l-1){
					(g_i->d[l-1][u])++;
				}
				else{
					swap(g_i->adj_list[k--], g_i->adj_list[--neighborhood_size]);
				}
			}
		}
		//}
		
		Listing(g_i, l-1, n);
		
		// Resetting labels	
		for(int64_t i = 0; i < g_i->ns[l-1]; i++){
			NodeID node = g_i->sub[l-1][i];
			g_i->lab[node] = l;
		}
	}
}

void PrintCliqueCount(int64_t k, int64_t *n){
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
	vector<int64_t> ranking;
	if(cli.file_name() == ""){
		ranking = OrdCore(g);
	}
	else{
		ranking = GetRankFromFile(cli.file_name());
	}

	//Graph dag = b.RelabelByRank(g, ranking);
	//vector<vector<NodeID>> dag = GenDag(g, ranking);
	GenGraph(g, &graph_struct, ranking, cli.clique_size());

	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to create graph struct: " << elapsed.count() << "s" << endl; 
	
	/*
	if(DAGCheck(&graph_struct, cli.clique_size())){
		cout << "Ordering constraints not satisfied. Graph is not a DAG." << endl;
	}
	else{
		cout << "Ordering constraints satisfied! Graph is a DAG." << endl;
	}
	*/

	start = std::chrono::system_clock::now();
	int64_t n = 0;
	Listing(&graph_struct, cli.clique_size(), &n);
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	
	PrintCliqueCount(cli.clique_size(), &n);
	//BenchmarkKernel(cli, g, Listing, PrintCliqueCount, ListingVerifier);
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
