#include <algorithm>
#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"

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

bool isNeigh(const Graph &g, NodeID u, NodeID v) {
	//For nodes in neighborhood, check if u exists
	vector<NodeID> neighborhood;
	for(NodeID node: g.out_neigh(u))
	{
		if (node == v)
		{
			return true;
		}
	}
	return false;
}

vector<vector<NodeID>> CF(const Graph &g, int size){
	vector<vector<NodeID>> cliques;
	for(NodeID u = 0; u < g.num_nodes(); u++){
		if (g.out_degree(u) < size - 1){
			continue;
		}
		else{
			vector<NodeID> temp;
			temp.push_back(u);
			if(temp.size() < size){
				for(NodeID v: g.out_neigh(u)){	
					if(g.out_degree(v) >= size - 1){
						if(temp.size() > 2){
							//check if each vertex in temp is connected with every other vertex
							int count = 0;
							for(NodeID vertex: temp){
								if(isNeigh(g, v, vertex) && u < v){
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
			if(temp.size() == size){
				//PrintVec(temp);
				cliques.push_back(temp);
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
	/*
	 *To add -cf as a command line flag
	bool callCF = 0;
	string commandFlag = argv[3];
	if (commandFlag.compare("-cf")){
		callCF = 1;
	}
	*/

	auto start = std::chrono::system_clock::now();
	//vector<vector<NodeID>> embedding = Extend(g, atoi(argv[3]));
	//vector<vector<NodeID>> embedding = InitEmbed(g);
	vector<vector<NodeID>> embedding = CF(g, atoi(argv[3]));
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "ms" << endl; 
	
	return 0;
}
