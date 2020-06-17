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

	#pragma omp parallel for
	for(int u = 0; u < embedding.size(); u++)
	{
		NodeID extVert = embedding[u].back();

		for(NodeID v : g.out_neigh(extVert))
		{
			vector<NodeID> temp = embedding[u];

			//if(!Exists(temp, v) && temp.size() < maxEmbeddingSize)
			if((extVert < v) && (temp.size() < maxEmbeddingSize))
			{
				temp.push_back(v);
				//if(!IsAutomorph(temp, embedding))
				#pragma omp critical
				{
					PrintVec(temp);
					embedding.push_back(temp);
				}
			}
		}
	}

	return embedding;
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
	
	auto start = std::chrono::system_clock::now();
	vector<vector<NodeID>> embedding = Extend(g, atoi(argv[3]));
	//vector<vector<NodeID>> embedding = InitEmbed(g);
	cout << "Number of embeddings: " << embedding.size() << endl;
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	//auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl; 
	
	return 0;
}
