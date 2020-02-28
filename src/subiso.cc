#include <iostream>
#include <string>
#include <fstream>
#include <set>
#include <vector>
#include <algorithm>
#include <typeinfo>

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

bool duplicate(vector<set<int>> container, set<int> element)
{
	for(int i = 0; i < container.size(); i++)
	{
		if(container[i] == element)
		{
			return true;
		}

	}
	return false;
}

vector<set<int>> initEmbedding(vector<set<int>> graph)
{
	vector<set<int>> twoVertEmbed;
	for(int i = 0; i < graph.size(); i++)
	{
		for(auto setElem : graph[i])
		{
			set<int> temp = {setElem, i};
			//cout << typeid(setElem).name() << endl;
			//cout << setElem << endl;
			if(!duplicate(twoVertEmbed, temp))
			{
				twoVertEmbed[i].push_back(temp);
			}	
			cout << i << ", " << setElem << endl;
			
		}
 	}

	return twoVertEmbed;
}

void extend(vector<set<int>> graph, int maxEmbeddingSize)
{
	vector<set<int>> twoVertEmbed = initEmbedding(graph);


}

int main(int argc, char* argv[])
{

	if(argc != 3)
	{
		cout << "Correct usage is ./subiso [FILENAME] [MAXSIZE]" << endl;
	}
	string fileName = argv[1];
	int maxSize = atoi(argv[2]);
	
	vector<set<int>> graph = constructGraph(fileName);
		
	extend(graph, maxSize);	
	return 0;
}
