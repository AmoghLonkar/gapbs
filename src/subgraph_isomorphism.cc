#include <iostream>
#include <string>
#include <fstream>
#include <set>
#include <vector>

using namespace std;

vector<set<int>> constructGraph(string fileName)
{
	vector<set<int>> graph;
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
			graph[node1].insert(node2);
                }
        }
        else
        {
                cout<<"Error: File not found!"<<endl;
        }

	return graph;
}	

void extend(vector<set<int>> graph, int maxEmbeddingSize)
{

}

int main(int argc, char* argv[])
{


	string fileName = argv[1];
	int maxSize = atoi(argv[2]);
	
	vector<set<int>> graph = constructGraph(fileName);
	
	
	extend(graph, maxSize);	
	return 0;
}
