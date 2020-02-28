#include <algorithm>
#include <iostream>
#include <vector>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "pvector.h"

using namespace std;

vector<Graph> initEmbed(Graph g)
{

}	

int main(int argc, char* argv[])
{
  CLBase cli(argc, argv, "subgraph isomorphism");
  if (!cli.ParseArgs())
    return -1;
  Builder b(cli);
  Graph g = b.MakeGraph();
  //g.PrintTopology(); 
  
  return 0;
}
