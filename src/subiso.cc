#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include <chrono>

using namespace std;

vector<vector<int>> constructGraph(string fileName)
{
	vector<vector<int>> graph;
	graph.resize(32);
	ifstream fileInput(fileName);
	string line; 
	if(fileInput.is_open())
	{
		while(getline(fileInput, line))
		{
			int space = line.find(" ");

			int node1 = stoi(line.substr(0, space));
			int node2 = stoi(line.substr(space+1));
			graph[node1].push_back(node2);
		}
	}
	else
	{
		cout<<"Error: File not found!"<<endl;
	}

	return graph;
}	

vector<vector<int>> initEmbedding(vector<vector<int>> graph)
{
	vector<vector<int>> twoVertEmbed;
	for(int i = 0; i < graph.size(); i++)
	{
		for(int j = 0; j <  graph[i].size(); j++)
		{
			if(i <= graph[i][j])
			{
				vector<int> temp = {i, graph[i][j]};
				twoVertEmbed.push_back(temp);
			}	

		}
	}

	return twoVertEmbed;
}

bool exists(vector<int> vec, int obj)
{
	for(int i = 0; i < vec.size(); i++)
	{
		if(vec[i] == obj)
		{
			return true;
		}
	}

	return false;
}

bool isAutomorph(vector<int> temp, vector<vector<int>> embed)
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

vector<vector<int>> extend(vector<vector<int>> graph, int maxEmbeddingSize)
{

	vector<vector<int>> twoVertEmbed = initEmbedding(graph);
	vector<vector<int>> embedding = twoVertEmbed;

	for(int i = 0; i < embedding.size(); i++)
	{
		int graphIndex = embedding[i].back();
		for(int j = 0; j < graph[graphIndex].size(); j++)
		{
			vector<int> temp = embedding[i];

			if(!exists(temp, graph[graphIndex][j]) && temp.size() < maxEmbeddingSize)
			{
				temp.push_back(graph[graphIndex][j]);
				if(!isAutomorph(temp, embedding))
				{
					embedding.push_back(temp);
				}

			}
		}

	}

	return embedding;
}

void printEmbedding(vector<vector<int>> embedding)
{
	for(auto subgraph: embedding)
	{
		cout << subgraph[0] << ": ";
		for(auto vertex: subgraph)
		{
			cout << vertex << " ";
		}
		cout << endl;
	}
}

int main(int argc, char* argv[])
{

	if(argc != 3)
	{
		cout << "Correct usage is ./subiso [FILENAME] [MAXSIZE]" << endl;
	}
	string fileName = argv[1];
	int maxSize = atoi(argv[2]);

	vector<vector<int>> graph = constructGraph(fileName);
	
	auto start = std::chrono::system_clock::now();
	vector<vector<int>> twoVertEmbed = initEmbedding(graph);	
	vector<vector<int>> embedding = extend(graph, maxSize);	
	auto end = std::chrono::system_clock::now();

	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    	cout << "Time to calculate possible subgraph isomorphisms: " <<elapsed.count() << "ms" << endl;


	//printEmbedding(embedding);
	return 0;
}
