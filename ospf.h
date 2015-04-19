#ifndef _OSPF_H
#define _OSPF_H
//created Margam Dilip Kumar
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <vector>
#include <fstream>
using namespace std;
typedef struct _Neighbours Neighbours;
struct _Neighbours{
	//int maxcost;
	//int mincost;
	int id;
};
struct Neigh_neigh{
	int cost;
	int id;
};
typedef struct _Node Node;
struct _Node{
	long long int seq_nu;
	int cost;
	int parent;
	bool parmanent;
	int no_neigh;
	vector<Neigh_neigh> neighs;
};
class MyNode{//data structure which stores command line arguments
   public:
	int total_nodes;
	int total_edges;
	int id;
	int my_sock;
	struct sockaddr_in my_addr;
	int no_neigh;
	vector<Neighbours> neigh;//all neighbours details
	vector<Node> all_nodes;
	pthread_mutex_t mtx;
	pthread_mutex_t mtx1;
	struct timespec start_t;
	string infile;
	string outfile;
	int hi; //hello intervel in seconds
	int lsai; //link state advertisement in sec
	int spfi; //shortest path finding intervel

	MyNode(){	//set to default values as given in problem statement
		hi = 1;
		lsai = 5;
		spfi = 20;
		clock_gettime(CLOCK_REALTIME,&start_t);
	}
	~MyNode(){}
	void set_values(int count, char** command){
		int i = 1;//I didn't really take care of all cases, this can get into seg faults if commandline input is not according to format.
		while( i < count && i%2 == 1){
			if( command[i][0] == '-'){
				switch( command[i][1] ){//chances of seg faults if input has only -
				case 'i':
					id = atoi(command[++i]);
					break;
				case 'f':
					infile = string(command[++i]);
					break;
				case 'o':
					outfile = string(command[++i]);
					break;
				case 'h':
					hi = atoi(command[++i]);
					break;
				case 'a':
					lsai = atoi(command[++i]);
					break;
				case 's':
					spfi = atoi(command[++i]);
					break;
				default:
					cout << "Error in input"<<endl;
					exit(1);
				}
			}
			i++;
		}

		ifstream i_file(infile.c_str());
		string line;
  		if (i_file.is_open())
  		{
 
			int nu = 0;
    			while ( getline (i_file,line) )
    			{
				nu++;
      				process_line(line,nu);
			//	cout << line <<endl;
    			}
    		i_file.close();
  		}
		no_neigh = neigh.size();
		
	}
    private:
	void process_line(string line,int nu){
		char * one;
		char str[100];
		strcpy(str,line.c_str());
		int extract[2];
  		one = strtok (str," ");
		int i = 0;
  		while (one != NULL)
  		{	
			extract[i] = atoi(one);
    		one = strtok (NULL," ");
			i++;
  		}
		if( nu == 1 ){
			total_nodes = extract[0];
			Node temp_node;
			temp_node.seq_nu = -1;
			temp_node.cost = INT_MAX;
			temp_node.parent = -1;
			temp_node.parmanent = false;
			for(int j=0; j < total_nodes; j++)
				all_nodes.push_back(temp_node);
			total_edges = extract[1];
			return;
		}
		Neighbours sample;
		if( extract[0] == id ){
			sample.id = extract[1];
			neigh.push_back(sample);
		}
		else if( extract[1] == id ){
			sample.id = extract[0];
			neigh.push_back(sample);
		}
	}
}myself;
#endif
