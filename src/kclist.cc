#include <algorithm>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <set>
#include <omp.h>
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

int main(int argc, char* argv[]){
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs()){
		return -1;
	}

	Builder b(cli);
	Graph g = b.MakeGraph();
	Graph dag = b.MakeDag(g);
	int k = atoi(argv[3]);

	Graph_Info graph_struct;
	Graph_Info *g_i = &graph_struct; 
	auto start = std::chrono::system_clock::now();
	Init(dag, g_i, k);
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

	for(int i = 0; i < dag.num_nodes(); i++){
		cout << g_i->sub[k][i] << endl;
	}
	cout << "Time to create graph struct: " << elapsed.count() << endl; 
	
	//cout << "Number of cliques: " << count << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
