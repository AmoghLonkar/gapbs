#include <algorithm>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <set>
#include <omp.h>
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

vector<vector<NodeID>> GetNeighVec(Graph &g){
	vector<vector<NodeID>> neighborhood;
	vector<NodeID> temp;
	
	neighborhood.reserve(g.num_nodes());
	temp.reserve(100);
	for(NodeID u = 0; u < g.num_nodes(); u++){
		for(NodeID node: g.out_neigh(u))
		{
			temp.push_back(node);
		}
		neighborhood.push_back(temp);
		temp.clear();
	}
	return neighborhood;
}

vector<set<NodeID>> GetNeighSet(Graph &g){
	vector<set<NodeID>> neighborhood;
	set<NodeID> temp;
	
	neighborhood.reserve(g.num_nodes());
	for(NodeID u = 0; u < g.num_nodes(); u++){
		for(NodeID node: g.out_neigh(u))
		{
			temp.insert(node);
		}
		neighborhood.push_back(temp);
		temp.clear();
	}
	return neighborhood;
}

//Binary Search for connectedness
bool BinNeigh(vector<NodeID> list, NodeID target){
	int l = 0;
	int r = list.size() - 1;
	int mid = 0;
	while(l <= r){
		mid = l + (r - l / 2);
		if(list[mid] > target){
			r = mid - 1;
		}
		else if(list[mid] < target){
			l = mid + 1;
		}
		else{
			return true;
		}
	}
	return false;
}

bool BinConnected(vector<vector<NodeID>> neighborhood, vector<NodeID> vec, NodeID extend, NodeID candidate){
	int count = 0;
	for(NodeID i: vec){
		if(i == extend){
			count++;
			continue;
		}

		if(BinNeigh(neighborhood[i], candidate)){
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

//Linear search for connectedness
bool LinNeigh(const Graph &g, NodeID u, NodeID v) {
	for(NodeID node: g.out_neigh(u))
	{
		if ( node == v)
		{
			return true;
		}
	}
	return false;
}

bool LinConnected(const Graph &g, vector<NodeID> vec, NodeID extend, NodeID candidate){
	int count = 0;
	for(NodeID i: vec){
		if(i == extend){
			count++;
			continue;
		}

		if(LinNeigh(g, i, candidate)){
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

bool InSet(set<NodeID> outNeigh, NodeID target){
	auto pos = outNeigh.find(target);

	if(pos != outNeigh.end()){
		return true;
	}
	else{
		return false;
	}
}

bool Connected(vector<set<NodeID>> neighborhood, vector<NodeID> vec, NodeID extend, NodeID candidate){
	int count = 0;
	for(NodeID i: vec){
		if(i == extend){
			count++;
			continue;
		}

		if(InSet(neighborhood[i], candidate)){
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

vector<vector<NodeID>> CF(const Graph &g, vector<set<NodeID>> neighborhood ,int size){
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
				
				//if(LinConnected(g, temp, j, k)){
				//if(BinConnected(neighborhood, temp, j, k)){
				if(Connected(neighborhood, temp, j, k)){
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
	vector<set<NodeID>> neighborhood = GetNeighSet(g);
	auto neighEnd = std::chrono::system_clock::now();
	auto neighElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(neighEnd - start);
	cout << "Time to calculate neighborhood: " <<neighElapsed.count() << "s" << endl;
	vector<vector<NodeID>> embedding = CF(g, neighborhood,atoi(argv[3]));
	auto embedEnd = std::chrono::system_clock::now();
	auto embedElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(embedEnd - neighEnd);
	//auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	cout << "Number of cliques: " << embedding.size() << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<embedElapsed.count() << "s" << endl;
	return 0;
}
