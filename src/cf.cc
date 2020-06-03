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

void PrintSet(set<NodeID> s){
	set<NodeID>::iterator si;
	for(si = s.begin(); si != s.end(); ++si){
		cout << *si << " "; 
	}
	cout << endl;
}

void PrintEmbedding(set<set<NodeID>> embed){
	set<set<NodeID>>::const_iterator si;
	set<NodeID>::iterator in;
	for(si = embed.begin(); si != embed.end(); ++si){
		for(in = si->begin(); in != si->end(); ++in){
			cout << *in << " ";
		}
		cout << endl;
	}
}

vector<vector<NodeID>> GetNeigh(Graph &g){
	vector<vector<NodeID>> neighborhood(g.num_nodes());
	for(NodeID u = 0; u < g.num_nodes(); u++){
		for(NodeID node: g.out_neigh(u))
		{
			neighborhood[u].push_back(node);
		}
		cout << u << ": " << PrintVec(neighborhood[u]);
	}
	return neighborhood;
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

set<NodeID> VecToSet(vector<NodeID> vec){
	set<NodeID> s;
	copy(s.begin(), s.end(), back_inserter(vec));
	return s;
}

//vector<vector<NodeID>> CF(const Graph &g, int size, vector<vector<NodeID>> neighborhood){
set<set<NodeID>> CF(const Graph &g, int size, vector<vector<NodeID>> neighborhood){
	set<set<NodeID>> cliques;
	#pragma omp parallel for shared(g, numCliques, size) private(u, v, temp, vertex, count, localBuffer)
	vector<vector<NodeID>> localBuffer;
	for(NodeID u = 0; u < g.num_nodes(); u++){
		if (g.out_degree(u) < size - 1){
			continue;
		}
		else{
			vector<NodeID> temp;
			temp.push_back(u);
			if(temp.size() < size){
				for(NodeID v: g.out_neigh(u)){	
					//if(g.out_degree(v) >= size - 1){
					if(temp.size() > 2){
						//check if each vertex in temp is connected with every other vertex
						int count = 0;
						for(NodeID vertex: temp){
							if((IsNeigh(g, v, vertex) && (u < v)){
							//if((IsNeigh(g, v, vertex) || IsNeigh(g, vertex, v)) && u < v){
								//if(IsConnected(neighborhood[v], vertex) && u < v){
								count++;
							}
							else{
								break;
							}

							}

							if(count == temp.size()){
								temp.push_back(v);
							}

						}
						else if(u < v){
							temp.push_back(v);
						}
						//}
				}
				}
				/*
				   if(temp.size() == size){
				   localBuffer.push_back(temp);
				   }
				   */
				//PrintVec(temp);	
				set<NodeID> tempSet = VecToSet(temp);
				#pragma omp critical (ListCliques)
				if(temp.size() == size){
					//if(localBuffer.size() > 100){
					//PrintVec(temp);
					//cliques.push_back(temp);

					//PrintSet(tempSet);
					cliques.insert(tempSet);

					//#pragma omp critical
					//cliques.insert(cliques.end(), localBuffer.begin(), localBuffer.end());
					//localBuffer.clear();
				}	
		}	
	}
	PrintEmbedding(cliques);
	return cliques;
}

int main(int argc, char* argv[])
{
	CLBase cli(argc, argv, "subgraph isomorphism");
	if (!cli.ParseArgs()){
		return -1;
	}
	
	Builder b(cli);
	Graph g = b.MakeGraph();
	//g.PrintTopology();
	vector<vector<NodeID>> neighborhood = GetNeigh(g);
	auto start = std::chrono::system_clock::now();
	//vector<vector<NodeID>> embedding = CF(g, atoi(argv[4]), neighborhood);
	set<set<NodeID>> embedding = CF(g, atoi(argv[4]), neighborhood);
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	//auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	//cout << "Number of cliques: " << embedding.size() << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
