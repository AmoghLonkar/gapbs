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


void PrintVec(vector<NodeID> vec)
{
	for(NodeID i : vec)
	{
		cout << i << " ";
	}
	cout << endl;
}

unordered_map<NodeID, vector<NodeID>> GetNeigh(Graph &g){
	unordered_map<NodeID, vector<NodeID>> neighborhood;
	vector<NodeID> temp;
	temp.reserve(100);

	for(NodeID u = 0; u < g.num_nodes(); u++){
		for(NodeID node: g.out_neigh(u))
		{
			temp.push_back(node);
		}
		neighborhood[u] = temp;
		temp.clear();
	}

	return neighborhood;
}

bool IsNeighSet(set<NodeID> neighborhood, NodeID v) {
	auto pos = neighborhood.find(v);

	if(pos != neighborhood.end()){
		return true;
	}
	else{
		return false;
	}
}

bool IsNeighVec(vector<NodeID> neighborhood, NodeID v) {
	for(NodeID i: neighborhood){
		if(i == v){
			return true;
		}
	}

	return false;
}

bool Connected(vector<NodeID> neighborhood, vector<NodeID> vec, NodeID extend){
	int count = 0;
	for(NodeID i: vec){
		if(i == extend){
			count++;
			continue;
		}

		if(IsNeighVec(neighborhood, i)){
			count++;
		}
	}
	if(count == vec.size()){
		return true;
	}
	else{
		return false;
	}
}

vector<vector<NodeID>> CF(const Graph &g, unordered_map<NodeID, vector<NodeID>> neighborhood ,int size){
	vector<vector<NodeID>> cliques;

	#pragma omp parallel for
	for(NodeID i = 0; i < g.num_nodes(); i++){
		if(g.out_degree(i) < size - 1){
			continue;
		}

		vector<NodeID> temp;
		for(NodeID j: g.out_neigh(i)){
			if((g.out_degree(j) < size -1) || (j <= i)){
				continue;
			}
			
			if(size > 3){	
				temp = {i, j};
			}

			for(NodeID k: g.out_neigh(j)){

				if(size == 3){
					temp = {i, j};
				}	

				if((g.out_degree(k) < size - 1) || (k <= j)){
					continue;
				}
				
				if(Connected(neighborhood[k], temp, j)){
					temp.push_back(k);
					
					if(temp.size() == size){
						#pragma omp critical
						{
							cliques.push_back(temp);
						}
					}
				}		
			}
		}
	}

	return cliques;
}

int main(int argc, char* argv[]){
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs()){
		return -1;
	}

	Builder b(cli);
	Graph g = b.MakeGraph();
	//g.PrintTopology();
	auto start = std::chrono::system_clock::now();
	unordered_map<NodeID, vector<NodeID>> neighborhood = GetNeigh(g);
	auto neighEnd = std::chrono::system_clock::now();
	auto neighElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(neighEnd - start);
	cout << "Time to calculate neighborhood: " <<neighElapsed.count() << "s" << endl;
	vector<vector<NodeID>> embedding = CF(g, neighborhood, atoi(argv[3]));
	auto embedEnd = std::chrono::system_clock::now();
	auto embedElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(embedEnd - neighEnd);
	//auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	cout << "Number of cliques: " << embedding.size() << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<embedElapsed.count() << "s" << endl;
	return 0;
}
