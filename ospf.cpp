/* This is code for which implements the ospf node
 * There is a Makefile which compiles this code
 * Command to run after compilation './ospf -i id -f infile -o outfile -h hi -a lsai -s spfi'
 * This node communicates with neighbouring nodes and learn the topology, also find the
 * shortest path using djikstra's algorithm
 * cost of every link changes every lsa seconds
 * example: ./ospf -i 5 -f infile7.txt -o outfile -h 10 -a 15 -s 70
 */
 
#include "ospf.h"
#define SENDBYTES 400
void *sendhello_function(void* dilip){
	char send_data[SENDBYTES] = "HELLO ";
	stringstream ss;
	ss << myself.id;
	string s_id = ss.str() + " ";//added for geni
	strcat(send_data,s_id.c_str());
	struct hostent *host;
	char neighhost[30];
	//cout << " this is send data "<<send_data <<endl;
	struct sockaddr_in server_addr = myself.my_addr;
	useconds_t s_time = myself.hi * 1000000;
	while(1){
		usleep(s_time);
		for(int i = 0; i< myself.no_neigh; i++){
			//host name mauplation
			int nid = myself.neigh[i].id;
			stringstream s1;
			s1 << nid;
			s_id = "node-"+s1.str();
    		strcpy(neighhost,s_id.c_str());
    		host = (struct hostent *) gethostbyname((char *)neighhost);
			server_addr.sin_addr = *((struct in_addr *) host->h_addr);//manuplate to neigh address

    		//adding time stamp for geni implementation
    		struct timespec now;//dilip
			clock_gettime(CLOCK_REALTIME,&now);
			double time_sent =(1000.0*(now.tv_sec-myself.start_t.tv_sec)+((now.tv_nsec-myself.start_t.tv_nsec)/1000000.0));
			stringstream s2;
			s2 << time_sent;
			s_id = s2.str();
			strcat(send_data,s_id.c_str());
			//cout<< " senddata " <<send_data<<endl;

			send_data[SENDBYTES-1] = '#';
			sendto(myself.my_sock, send_data, SENDBYTES, 0,(struct sockaddr *) &server_addr, sizeof (struct sockaddr));
        }
    }
}
void *sendlsa_function(void* dilip){
	//LSA | srcid | Seq. Number | No. Entries | Neigh1 | Cost1 | Neigh2 | Cost2 |
	struct sockaddr_in server_addr = myself.my_addr;
	struct hostent *host;
	char neighhost[30];
	useconds_t s_time = myself.lsai * 1000000;
	while(1){
		char send_data[SENDBYTES] = "LSA ";
		stringstream ss;
		ss << myself.id;
		string s_id = ss.str();
		strcat(send_data,s_id.c_str());
		strcat(send_data," ");

		usleep(s_time);//sleep

		stringstream s1;
		pthread_mutex_lock(&myself.mtx);
		s1 << ++myself.all_nodes[myself.id].seq_nu;
		s_id = s1.str();//sequence number
		//strcat(send_data,s_id.c_str());
		//strcat(send_data," ");
		int no_entries = myself.all_nodes[myself.id].no_neigh;
		stringstream s2;
		s2 << no_entries;
		s_id = s_id + " " + s2.str();
		//strcat(send_data,s_id.c_str());
		//strcat(send_data," ");
		for(int i=0; i< no_entries; i++){
			stringstream s3;
			stringstream s4;
			s3 << myself.all_nodes[myself.id].neighs[i].id;
			s_id = s_id + " " + s3.str();
			//strcat(send_data,s_id.c_str());
			//strcat(send_data," ");
			s4 << myself.all_nodes[myself.id].neighs[i].cost;
			s_id = s_id + " " + s4.str();
		}
		pthread_mutex_unlock(&myself.mtx);
		strcat(send_data,s_id.c_str());
		//cout <<" send_data at lsa message : "<<send_data<<endl;
		//sending lsa message to all neighbours
		for(int i = 0; i< myself.no_neigh; i++){
			int nid = myself.neigh[i].id;
			//added for geni
			stringstream sd;
			sd << nid;
			s_id = "node-"+sd.str();
    		strcpy(neighhost,s_id.c_str());
    		host = (struct hostent *) gethostbyname((char *)neighhost);//getting neighbours address
		server_addr.sin_addr = *((struct in_addr *) host->h_addr);//manuplate to neigh address

			send_data[SENDBYTES-1] = '#';
		sendto(myself.my_sock, send_data, SENDBYTES, 0,(struct sockaddr *) &server_addr, sizeof (struct sockaddr));
        }
    }
}
//reciever can recieve hello message, helloreply message or LSA message.
void *reciever_function(void* dilip){
	int bytes_read;
	char recv_data[SENDBYTES];
	struct sockaddr_in client_addr;
	unsigned int sock_len = sizeof(struct sockaddr);
	while(1){
		bytes_read = recvfrom(myself.my_sock, recv_data, SENDBYTES, 0,(struct sockaddr *) &client_addr, &sock_len);
		if(bytes_read != SENDBYTES)
			cout<<"\n some error in reciever : "<<bytes_read<<endl;
		int firstspace;
		for(int i=0; i<bytes_read ;i++){
			if( recv_data[i] == ' ' ){
				firstspace = i;
				break;
			}
		}
	//		switch(firstspace){
	//			case 5://send hello reply //HELLOREPLY j i timestampforgeni
		if(firstspace == 5){
				char temp_one[100];
				strcpy(temp_one,&recv_data[6]);

				//changed a lot for geni
				char send_data[SENDBYTES] = "HELLOREPLY ";
				stringstream ss;
				ss << myself.id;
				string s_id = ss.str();
				strcat(send_data,s_id.c_str());
				strcat(send_data," ");
				strcat(send_data,temp_one);

				//changed for geni
				send_data[SENDBYTES-1] = '#';//sending hello reply to respective client_addr
				sendto(myself.my_sock, send_data, SENDBYTES, 0,(struct sockaddr *) &client_addr, sizeof (struct sockaddr));

		}
		else if(firstspace == 10){
	//				break;
	//			case 10://recived helloreply message, we update neighbours of our node.
				//HELLOREPLY j i value for link ij(replaced by timestamp in geni)
				char* ext;
				char temp_data[100];
				char extractchar[3][30];
				int extract[3];
				strcpy(temp_data,&recv_data[11]);
				ext = strtok(temp_data," ");
				int u = 0;
				while( ext != NULL ){
					//extract[u] = atoi(ext);
					strcpy(extractchar[u],ext);
					ext = strtok(NULL," ");
					u++;
				}
				extract[0] = atoi(extractchar[0]);
				extract[1] = atoi(extractchar[1]);
				double timesent = atof(extractchar[2]);
    			struct timespec now;//dilip
				clock_gettime(CLOCK_REALTIME,&now);
				double time_recv =(1000.0*(now.tv_sec-myself.start_t.tv_sec)+((now.tv_nsec-myself.start_t.tv_nsec)/1000000.0));
				extract[2] = (int)(time_recv - timesent);
				//if(myself.id == extract[1])
				//	cout << " recived correct helloreply : "<< recv_data <<endl;
				int duplicate = -1;
				
				for(int j=0; j < myself.all_nodes[extract[1]].neighs.size(); j++){
					if( myself.all_nodes[extract[1]].neighs[j].id == extract[0] ){
						duplicate = j;
						break;
					}
				}
				pthread_mutex_lock(&myself.mtx);
				if( duplicate == -1 ){	
					Neigh_neigh sam;
					sam.id = extract[0];
					sam.cost = extract[2];
					myself.all_nodes[extract[1]].neighs.push_back(sam);
				}
				else{
					myself.all_nodes[extract[1]].neighs[duplicate].cost = extract[2];
				}
				myself.all_nodes[extract[1]].no_neigh = myself.all_nodes[extract[1]].neighs.size();
				pthread_mutex_unlock(&myself.mtx);
		}
		else if(firstspace == 3){
	//				break;
	//			case 3://got lsa message need to broadcast lsa message
				//LSA | srcid | Seq. Number | No. Entries | Neigh1 | Cost1 | Neigh2 | Cost2 |
				//cout <<" got lsa message: "<< recv_data<<endl;
				char* ext;
				char temp_data[SENDBYTES];
				vector<int> extract;
				strcpy(temp_data,&recv_data[4]);
				ext = strtok(temp_data," ");
				//int m =0;
				while( ext != NULL ){
					extract.push_back(atoi(ext));
				//	cout << " lsa extract : "<<m<<" "<< extract[m]<<endl;
					ext = strtok(NULL," ");
				//	m++;
				}
				//cout <<" sequence number comaparison : "<< myself.all_nodes[extract[0]].seq_nu << " "<< extract[1]<<endl;
				if(myself.all_nodes[extract[0]].seq_nu < extract[1]){ 
					//cout << "** got lsa message **"<<recv_data<<endl;
					pthread_mutex_lock(&myself.mtx1);
					myself.all_nodes[extract[0]].seq_nu = extract[1];
					myself.all_nodes[extract[0]].neighs.clear();
					myself.all_nodes[extract[0]].no_neigh = extract[2];
					Neigh_neigh sam;
					for(int j=0; j < 2*extract[2]; j=j+2){
						sam.id = extract[3+j];
						sam.cost = extract[4+j];
						myself.all_nodes[extract[0]].neighs.push_back(sam);
					}
					pthread_mutex_unlock(&myself.mtx1);
				}

				//broadcasting lsa message
				struct sockaddr_in server_addr = myself.my_addr;
				struct hostent *host;
				char neighhost[30];
				for(int i = 0; i< myself.no_neigh; i++){
					int nid = myself.neigh[i].id;
					//modifications for geni
					stringstream s1k;
					s1k << nid;
					string s_id = "node-"+s1k.str();
    				strcpy(neighhost,s_id.c_str());
    				host = (struct hostent *) gethostbyname((char *)neighhost);
				server_addr.sin_addr = *((struct in_addr *) host->h_addr);//manuplate to neigh address

					if( strcmp(inet_ntoa(server_addr.sin_addr),inet_ntoa(client_addr.sin_addr)) == 0 )
						continue;
					//server_addr.sin_port = htons(20000+nid);//manuplate neigh id
					//send_data[SENDBYTES-1] = '#';
					sendto(myself.my_sock, recv_data, SENDBYTES, 0,(struct sockaddr *) &server_addr, sizeof (struct sockaddr));
        		}
        }
        else{
	//				break;
	//			default:
				cout<< " recieve function came to default error "<<endl;
	//				break;
	//		}
		}
	}
}

void *createspf_function(void* dilip){
	useconds_t s_time = myself.spfi * 1000000;
	int loopcount = 0;
	while(1){
		loopcount++;
		usleep(s_time);
		vector<Node> allnodescopy;
		pthread_mutex_lock(&myself.mtx1);
		pthread_mutex_lock(&myself.mtx);
		allnodescopy = myself.all_nodes;/*
		stringstream sod;
		sod << myself.id;
		string outdel = "delete-" + sod.str();
		ofstream dil(outdel.c_str());
		for(int u=0; u<myself.all_nodes.size();u++){
			//print_node(allnodescopy[u],myself.all_nodes[u]);
			
			dil << " cost " << allnodescopy[u].cost <<"  "<<myself.all_nodes[u].cost<<endl;
			//dil << " parent " << allnodescopy[u].parent <<"  "<<myself.all_nodes[u].parent<<endl;
			//dil << " parmanent "<< allnodescopy[u].parmanent << "  "<<myself.all_nodes[u].parmanent<<endl;
			dil << " no_neigh "<< allnodescopy[u].no_neigh <<"  "<<myself.all_nodes[u].no_neigh<<endl;
			dil << " printing neighbours size: "<<allnodescopy[u].neighs.size()<<" "<<myself.all_nodes[u].neighs.size()<<endl;
			for(int k=0; k<myself.all_nodes[u].neighs.size(); k++){
				dil << " cost "<<k<<" one "<<allnodescopy[u].neighs[k].cost<<" two "<<myself.all_nodes[u].neighs[k].cost<<endl;
				dil << " id "<<k<<" one "<<allnodescopy[u].neighs[k].id<<" two "<<myself.all_nodes[u].neighs[k].id<<endl;
			}
		}
		dil.close();*/
		pthread_mutex_unlock(&myself.mtx);
		pthread_mutex_unlock(&myself.mtx1);
		//cout << " In Shortest path function : size of vector allnodescopy : "<< allnodescopy.size() <<endl;
		int perma = 1;
		int recent_node = myself.id;
		//label for A node
		allnodescopy[recent_node].cost = 0;
		allnodescopy[recent_node].parent = recent_node;
		allnodescopy[recent_node].parmanent = true;

		stringstream so;
		so << myself.id;
		string outfilename = myself.outfile + "-" + so.str();
		ofstream ofile(outfilename.c_str());

		//this loop updates the cost, parent and permanent fields in the Node structure
		while(myself.total_nodes != perma){//condition to halt
			//ofile<<" current node from perma : "<<recent_node<<endl;
			//recent_node is the node that got resently added to permanent list
			//update labels of all the neighbors of recent_node
			//ofile<<" number of neighbours in recent_node : "<<allnodescopy[recent_node].no_neigh<<endl;
			for(int i=0; i<allnodescopy[recent_node].no_neigh; i++){
				//ofile<<" current node index: "<<i<<" id "<<allnodescopy[recent_node].neighs[i].id<<endl;
				if(allnodescopy[allnodescopy[recent_node].neighs[i].id].parmanent)
					continue;
				if(allnodescopy[allnodescopy[recent_node].neighs[i].id].cost >=
						 allnodescopy[recent_node].cost + allnodescopy[recent_node].neighs[i].cost){
					//ofile <<" node "<<allnodescopy[recent_node].neighs[i].id<< " cost edited to ";
					allnodescopy[allnodescopy[recent_node].neighs[i].id].cost =
						 allnodescopy[recent_node].cost + allnodescopy[recent_node].neighs[i].cost;
					//ofile <<allnodescopy[allnodescopy[recent_node].neighs[i].id].cost<<endl;
					allnodescopy[allnodescopy[recent_node].neighs[i].id].parent = recent_node;
				}
				//else
				//	ofile << "current node cost not updated "<<endl;
			}
			//chose the node with least_cost that is leastcostindex and add it to permanent.
			int leastcostindex;
			int least_cost = INT_MAX;
			for(int i=0; i<allnodescopy.size(); i++){
				if(!allnodescopy[i].parmanent){
					if(allnodescopy[i].cost < least_cost){
						least_cost = allnodescopy[i].cost;
						leastcostindex = i;
					}
				}
			}
			perma++;
			allnodescopy[leastcostindex].parmanent = true;
			recent_node = leastcostindex;
		}
		//write the results onto output file

		ofile << "Routing Table for Node No."<< myself.id <<" at Time " << loopcount*myself.spfi <<endl;
		ofile << "Destination	path	cost"<<endl;
		cout << "Routing Table for Node No."<< myself.id <<" at Time " << loopcount*myself.spfi <<endl;
		cout << "Destination	path	cost"<<endl;
		for(int node_nu = 0; node_nu < myself.total_nodes; node_nu++){
			if(node_nu == myself.id)
				continue;
			string nodepath;
			int temp_nu = node_nu;
			stringstream gj;
			gj << node_nu;
			nodepath = gj.str();
			int p_len = 1;
			while(p_len < myself.total_nodes+10){
				nodepath = "-" + nodepath;
				stringstream tmp_sa;
				tmp_sa << allnodescopy[temp_nu].parent;
				nodepath = tmp_sa.str() + nodepath;
				if(allnodescopy[temp_nu].parent == myself.id)
					break;
				else
					temp_nu = allnodescopy[temp_nu].parent;
				p_len++;
				//cout<< " nodepath check for infi loop " << nodepath <<endl;
			}
			ofile << "  " << node_nu <<"         "<<nodepath<<"      "<<allnodescopy[node_nu].cost<<endl;
			cout << "  " << node_nu <<"         "<<nodepath<<"      "<<allnodescopy[node_nu].cost<<endl;
		}
		ofile.close();
	}
}

int main(int argc, char** argv){
	
	if( argc < 8 ){
		cout << "usage: './ospf -i id -f infile -o outfile -h hi -a lsai -s spfi'"<<endl;
		exit(1);
	}
	myself.set_values(argc,argv);
	int sock;
	struct sockaddr_in server_addr;//server_addr is my address
    //struct hostent *host;
    //char local_host[10] = "localhost";
    //host = (struct hostent *) gethostbyname((char *)local_host);
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        	perror("socket");
        	exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(20042);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(server_addr.sin_zero), 8);
    if (bind(sock, (struct sockaddr *) &server_addr,
            sizeof (struct sockaddr)) == -1) {
        perror("Bind");
        exit(1);
    }
    myself.my_sock = sock;
    myself.my_addr = server_addr;
	srand(time(NULL));
	pthread_t sendhello,reciever,sendlsa,createspf;
	int fail=0;
	int dilip[1];
	fail = pthread_create(&sendhello,NULL,sendhello_function,(void*)dilip);
	if(fail){
		cout << " Failed to create sendhello thread \n";
		exit(1);
	}
	fail = pthread_create(&sendlsa,NULL,sendlsa_function,(void*)dilip);
	if(fail){
		cout << " Failed to create sendlsa thread \n";
		exit(1);
	}
	fail = pthread_create(&reciever,NULL,reciever_function,(void*)dilip);
	if(fail){
		cout << " Failed to create reciever thread \n";
		exit(1);
	}
	fail = pthread_create(&createspf,NULL,createspf_function,(void*)dilip);
	if(fail){
		cout << " Failed to create createspf thread \n";
		exit(1);
	}
	pthread_join(sendhello,NULL);
	close(sock);
	return 0;
}
