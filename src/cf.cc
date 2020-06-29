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

/*
//vector<vector<NodeID>> CF(const Graph &g, int size, vector<vector<NodeID>> neighborhood){
vector<vector<NodeID>> CF(const Graph &g, int size){
	vector<vector<NodeID>> cliques;
	#pragma omp parallel for
	for(NodeID u = 0; u < g.num_nodes(); u++){
		if (g.out_degree(u) < size - 1){
			continue;
		}
		else{
			vector<NodeID> temp;
			vector<vector<NodeID>> localBuffer;
			//cout << u << ": " << endl;
			temp.push_back(u);
			for(NodeID v: g.out_neigh(u)){	
				//cout << v << endl;
				if(temp.size() < size){
				//for(NodeID v: g.out_neigh(u)){	
					//if(g.out_degree(v) >= size - 1){
					if(temp.size() > 2){
						//check if each vertex in temp is connected with every other vertex
						int count = 0;
						for(NodeID vertex: temp){
							if(IsNeigh(g, v, vertex) && (u < v)){
							//if((IsNeigh(g, v, vertex) || IsNeigh(g, vertex, v)) && (u < v)){
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
				
				   if(temp.size() == size){
				   	localBuffer.push_back(temp);
				   }
				   
				//cout << u << ": ";
				//PrintVec(temp);	
				#pragma omp critical (ListCliques)
				if(temp.size() == size){
					//if(localBuffer.size() > 100){
					//PrintVec(temp);
					cliques.push_back(temp);
					//#pragma omp critical
					//cliques.insert(cliques.end(), localBuffer.begin(), localBuffer.end());
					//localBuffer.clear();
				//}	
		}	
		}
	}
	return cliques;
}
*/

vector<vector<NodeID>> GetEdges(const Graph &g, int size){
	vector<vector<NodeID>> edges;
	//#pragma parallel for
	for (NodeID u = 0; u < g.num_nodes(); u++)
	{	
		//if(g.out_degree(u) < size - 1){
		if(g.out_degree(u) < size - 1){
			continue;
		}

		vector<vector<NodeID>> localBuffer;
		for (NodeID v : g.out_neigh(u))
		{	
			//if(u <= v && g.out_degree(v) >= size - 2){
			if(u <= v && g.out_degree(v) >= size - 1){
				vector<NodeID> temp = {u, v};
				edges.push_back(temp);
				/*
				#pragma omp critical
				{
				edges.push_back(temp);
				}
				*/
				//localBuffer.push_back(temp);
				
			}
			else{
				continue;
			}
			/*
			#pragma omp critical
			{
			
				if(localBuffer.size() > 10){
					edges.insert(edges.end(), localBuffer.begin(), localBuffer.end());
					localBuffer.clear();
				}
					
			}*/		

		}
	}

	return edges;
}

bool Connected(const Graph &g, vector<NodeID> vec, NodeID vertex){
	int count = 0;
	for(NodeID i: vec){
		//if(IsNeigh(g, i, vertex) || IsNeigh(g, vertex, i)){
		if(IsNeigh(g, i, vertex)){
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
	vector<vector<NodeID>> embeddings = GetEdges(g, size);
	
	#pragma omp parallel for
	for(int u = 0; u < embeddings.size(); u++)
	{
		NodeID extVert = embeddings[u].back();

		for(NodeID v : g.out_neigh(extVert)){
			vector<NodeID> temp = embeddings[u];
			vector<vector<NodeID>> localCliques;
			vector<vector<NodeID>> localEmbeddings;

			if((extVert < v) && (temp.size() < size) && Connected(g, temp, v)){
				temp.push_back(v);
				
				/*
				if(temp.size() != size){
					//PrintVec(temp);
					localEmbeddings.push_back(temp);
				}
				else{
					//PrintVec(temp);
					localCliques.push_back(temp);
				}*/
					
				if(temp.size() != size){
					//PrintVec(temp);
					#pragma omp critical
					{
						embeddings.push_back(temp);
					}
				}
				else{
					//PrintVec(temp);
					#pragma omp critical
					{
						cliques.push_back(temp);
					}
				}

					/*
					if(localEmbeddings.size() > 10){
						embeddings.insert(embeddings.end(), localEmbeddings.begin(), localEmbeddings.end());
						localEmbeddings.clear();
					}
					if(localCliques.size() > 10){
						cliques.insert(cliques.end(), localCliques.begin(), localCliques.end());
						localCliques.clear();
						
					}*/
				

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
	//vector<vector<NodeID>> neighborhood = GetNeigh(g);
	auto start = std::chrono::system_clock::now();
	//vector<vector<NodeID>> embedding = GetEdges(g, atoi(argv[3]));
	vector<vector<NodeID>> embedding = CF(g, atoi(argv[3]));
	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
	//auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	cout << "Number of cliques: " << embedding.size() << endl;
	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "s" << endl;
	return 0;
}
