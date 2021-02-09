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

struct Edge{
	NodeID source;
	NodeID dest;
};

struct Graph_Info{
	vector<int> ns;
	int e;
	vector<Edge> edges;
	vector<vector<int>> d;
	vector<int> cd;
	vector<NodeID> adj_list;
	vector<vector<NodeID>> sub;
	vector<int> lab;
};

struct Min_Heap{
	int n;
	vector<int> ptrs;
	vector<pair<NodeID, int>> kv_pair;
};

void GetEdges(Graph &g, Graph_Info *g_i){
	vector<Edge> edges;
	g_i->e = 0;
	for(int i = 0; i < g.num_nodes(); i++){
		for(NodeID neighbor: g.out_neigh(i)){
			Edge temp;
			temp.source = i;
			temp.dest = neighbor;
			edges.push_back(temp);
			g_i->e++;
		}	
	}
	g_i->edges = edges;
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

vector<int> OrdCore(Graph &g){
	vector<int> ranking(g.num_nodes());
	int n = g.num_nodes();
	int r = 0;

	//Initialize Heap
	vector<pair<NodeID, int>> nodeDegPairs;
	for(NodeID u = 0; u < n; u++){
		nodeDegPairs.push_back(make_pair(u, g.out_degree(u)));
	}
	make_heap(nodeDegPairs.begin(), nodeDegPairs.end());
	sort(nodeDegPairs.begin(), nodeDegPairs.end(), [](const pair<NodeID, int> &left, const pair<NodeID, int> &right) { return left.second < right.second; });

	for(int i = 0; i < n; i++){
		pair<NodeID, int> root = nodeDegPairs.front();
		ranking[root.first] = n - (++r);
		
		for(NodeID neighbor: g.out_neigh(root.first)){
			auto it = find_if( nodeDegPairs.begin(), nodeDegPairs.end(), [&neighbor](const pair<NodeID, int>& element){ return element.first == neighbor;} );
			it->second--;
		}
		
		pop_heap(nodeDegPairs.begin(), nodeDegPairs.end());
		nodeDegPairs.pop_back();
		
		sort_heap(nodeDegPairs.begin(), nodeDegPairs.end(), [](const pair<NodeID, int> &left, const pair<NodeID, int> &right) { return left.second < right.second; });
	}
	
	/*
	for(auto elem: ranking){
		cout << "Ranking: " << elem << endl;
	}
	*/
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
	for(NodeID i = 0; i < g_i->e; i++){
		d[k][g_i->edges[i].source]++;
	}
	
	vector<vector<NodeID>> sub(k+1, vector<NodeID>(g.num_nodes(), 0));
	for(NodeID i = 0; i < g.num_nodes(); i++){
		sub[k][i] = i; 
	}
	g_i->sub = sub;	

	vector<int> cd(g.num_nodes() + 1, 0);
	for(NodeID i = 1; i < g.num_nodes() + 1; i++){
		cd[i] = cd[i-1] + d[k][i-1];
		d[k][i-1] = 0;
	}
	g_i->cd = cd;
	
	vector<NodeID> adj(g.num_edges(), 0);
	for(int i = 0; i < g_i->e; i++){
		adj[g_i->cd[g_i->edges[i].source] + d[k][g_i->edges[i].source]++] = g_i->edges[i].dest;
	}

	g_i->adj_list = adj;
	g_i->d = d;

	vector<int> lab(g.num_nodes(), k);
	g_i->lab = lab;
}

void Listing(Graph_Info *g_i, int l, unsigned int *n){
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
		NodeID u = g_i->sub[l][i];
		g_i->ns[l-1] = 0;
		
		int bound = g_i->cd[u] + g_i->d[l][u];

		for(int j = g_i->cd[u]; j < bound; j++){
			NodeID neighbor = g_i->adj_list[j];
			if(g_i->lab[neighbor] == l){
				g_i->lab[neighbor] = l-1;
				
				// Adding nodes to subgraph
				g_i->sub[l-1][g_i->ns[l-1]++] = neighbor;
				g_i->d[l-1][neighbor] = 0;
				//g_i->d[l-1][g_i->ns[l-1]++] = 0;
			}
		}
		
		// Computing degrees
		for(int j = 0; j < g_i->ns[l-1]; j++){
			NodeID node = g_i->sub[l-1][j];
			bound = g_i->cd[node] + g_i->d[l][node];
			
			// Looking at edges between nodes
			for(int k = g_i->cd[node]; k < bound; k++){
				NodeID neighbor = g_i->adj_list[k];
				
				// Node is present in the subgraph
				if(g_i->lab[neighbor] == l-1){
					(g_i->d[l-1][node])++;
					//(g_i->d[l-1][j])++;
				}
				else{
					g_i->adj_list[k--] = g_i->adj_list[--bound];
					g_i->adj_list[bound] = neighbor;
				}
			}
		}

		Listing(g_i, l-1, n);
		
		// Resetting labels	
		for(int j = 0; j < g_i->ns[l-1]; j++){
			NodeID node = g_i->sub[l-1][j];
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
	//vector<int> ranking = OrdCore(g, &bin_heap);
	vector<int> ranking = OrdCore(g);
	//vector<int> ranking = GetRankFromFile(argv[3]);
	Relabel(&graph_struct, ranking);
	//Graph dag = b.MakeDagFromRank(g, ranking);
	
	//Graph dag = b.MakeDag(g);
	int k = atoi(argv[3]);

	Init(g, &graph_struct, k);
	
	/*	
	for(int i = 0; i < graph_struct.e; i++){
		cout << "Adjacency list: " << graph_struct.adj_list[i] << endl; 
	}
	*/
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	cout << "Time to create graph struct: " << elapsed.count() << "s" << endl; 
	
	start = std::chrono::system_clock::now();
	unsigned int n = 0;
	Listing(&graph_struct, k, &n);
	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	
	cout << "Number of cliques: " << n << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
