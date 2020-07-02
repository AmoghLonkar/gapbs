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

//Binary Search for connectedness
bool IsConnected(vector<NodeID> list, NodeID target){
	int l = 0;
	int r = list.size() - 1;
	int mid = 0;
	while(l <= r){
		mid = l + (r - l / 2);
		cout << l << " " << mid << " " << r << endl;
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

//Linear search for connectedness
bool IsNeigh(const Graph &g, NodeID u, NodeID v) {
	for(NodeID node: g.out_neigh(u))
	{
		if ( node == v)
		{
			return true;
		}
	}
	return false;
}

bool Connected(const Graph &g, vector<NodeID> vec, NodeID extend, NodeID candidate){
	int count = 0;
	for(NodeID i: vec){
		if(i == extend){
			count++;
			continue;
		}

		if(IsNeigh(g, i, candidate)){
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

vector<vector<NodeID>> CF(const Graph &g, int size){
	vector<vector<NodeID>> cliques;
	vector<NodeID> temp;

	for(NodeID i = 0; i < g.num_nodes(); i++){
		if(g.out_degree(i) < size - 1){
			continue;
		}

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
				
				if(Connected(g, temp, j, k)){
					temp.push_back(k);
					
					if(temp.size() == size){
						cliques.push_back(temp);
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
	vector<vector<NodeID>> embedding = CF(g, atoi(argv[3]));
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	//auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	cout << "Number of cliques: " << embedding.size() << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
