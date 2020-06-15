#include <algorithm>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
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

vector<vector<NodeID>> InitEmbed(const Graph &g)
{
	vector<vector<NodeID>> twoVertEmbed;
	for (NodeID u=0; u < g.num_nodes(); u++)
	{	
		for (NodeID v : g.out_neigh(u))
		{
			if(u <= v)
			{
				vector<NodeID> temp = {u, v};
				//PrintVec(temp);
				twoVertEmbed.push_back(temp);
			}	

		}
	}

	return twoVertEmbed;
}	

bool Exists(vector<NodeID> vec, NodeID obj)
{
	for(NodeID u : vec)
	{
		if(u == obj)
		{
			return true;
		}
	}

	return false;
}

bool IsAutomorph(vector<int> temp, vector<vector<NodeID>> embed)
{	
	sort(temp.begin(), temp.end());
	for(auto element : embed)
	{
		sort(element.begin(), element.end());
		if(temp == element)
		{
			return true;
		}
	}

	return false;
}

vector<vector<NodeID>> Extend(const Graph &g, int maxEmbeddingSize)
{

	vector<vector<NodeID>> embedding = InitEmbed(g);

	//#pragma omp parallel for
	for(int u = 0; u < embedding.size(); u++)
	{
		NodeID extVert = embedding[u].back();

		//#pragma omp parallel for
		for(NodeID v : g.out_neigh(extVert))
		{
			vector<NodeID> temp = embedding[u];

			if(!Exists(temp, v) && temp.size() < maxEmbeddingSize)
			{
				temp.push_back(v);
				//if(!IsAutomorph(temp, embedding))
				{
					//PrintVec(temp);
					embedding.push_back(temp);
				}
			}
		}
	}

	return embedding;
}

vector<vector<NodeID>> GetNeigh(Graph &g){
	vector<vector<NodeID>> neighborhood(g.num_nodes());
	for(NodeID u = 0; u < g.num_nodes(); u++){
		for(NodeID node: g.out_neigh(u))
		{	
			neighborhood[u].push_back(node);
		}
	}
	return neighborhood;	
}

bool IsConnected(vector<NodeID> list, NodeID target){
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

vector<vector<NodeID>> CF(const Graph &g, int size){
	vector<vector<NodeID>> cliques;
	//#pragma omp parallel for shared(g, numCliques, size) private(u, v, temp, vertex, count, localBuffer)
	#pragma omp parallel for
	for(NodeID u = 0; u < g.num_nodes(); u++){
		if (g.out_degree(u) < size - 1){
			continue;
		}
		else{
			vector<vector<NodeID>> localBuffer;
			vector<NodeID> temp;
			temp.push_back(u);
			if(temp.size() < size){
				for(NodeID v: g.out_neigh(u)){	
					if(g.out_degree(v) >= size - 1){
						if(temp.size() > 2){
							//check if each vertex in temp is connected with every other vertex
							int count = 0;
							for(NodeID vertex: temp){
								if(IsNeigh(g, v, vertex) && u < v){
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
					}
				}
			}
			/*
			if(temp.size() == size){
				localBuffer.push_back(temp);
			}
			*/
			#pragma omp critical (ListCliques)
			if(temp.size() == size){
			//if(localBuffer.size() > 100){
				//PrintVec(temp);
				cliques.push_back(temp);
				//#pragma omp critical
				//cliques.insert(cliques.end(), localBuffer.begin(), localBuffer.end());
				//localBuffer.clear();
			}	
		}	
	}
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
	//vector<vector<NodeID>> embedding = Extend(g, atoi(argv[3]));
	//vector<vector<NodeID>> embedding = InitEmbed(g);
	vector<vector<NodeID>> embedding = CF(g, atoi(argv[3]));
	//cout << "Size: " << embedding.size() << endl;
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	//auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	cout << "Number of cliques: " << embedding.size() << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl; 
	
	return 0;
}
