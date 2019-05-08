/*
 * Implementations for Hybrid model
 * Only used by the evaluator
 * May 20, 2016
 * daveti
 */
#include <stdio.h>
#include <vector>
#include "util.h"
#include "sgx_trts.h"
#include "../include/smcsgxyaoparam.h"

using std::vector;


class EvalInput
{
	public:
		//for eval
		vector<vector<char> > evalgc;
		vector<char> permubits; //this will alway contain 0s 
};

class GenInput
{
	public:
		//for gen/seder
		vector<vector<char> > gengc;
		vector<char> permubits;

};

class Input
{
	public:
		EvalInput evalInput;
		GenInput genInput;
};


class Output
{
	public:
		vector<vector<char> > output;
};


static long makeSame(long value)
{
	value = value | (value << 32);
	value = value | (value << 16);
	value = value | (value << 8);
	value = value | (value << 4);
	value = value | (value << 2);
	value = value | (value << 1);
	return value;
}

static unsigned int makeSame(unsigned int value)
{
        value = value | (value << 16);
        value = value | (value << 8);
        value = value | (value << 4);
        value = value | (value << 2);
        value = value | (value << 1);


        return value;
}

static int makeSame(int value)
{
	value = value | (value << 16);
	value = value | (value << 8);
	value = value | (value << 4);
	value = value | (value << 2);
	value = value | (value << 1);


	return value;
}

long makeSameLong(long value)
{
    value = value | (value << 32);
    value = value | (value << 16);
    value = value | (value << 8);
    value = value | (value << 4);
    value = value | (value << 2);
    value = value | (value << 1);


    return value;
}

static int rand()
{
        int val;

        if (SGX_SUCCESS != sgx_read_rand(reinterpret_cast<unsigned char *>(&val), sizeof(val))) {
                // XXX FAIL HERE
                return 0;
        }

        return abs(val);
}


class ORAM2_YAO
{
	public:
		unsigned int oramsize;
		ORAM2_YAO() {}
		ORAM2_YAO(int size)
		{
			data.resize(size);
			vsize = size;
			oramsize = size;
			flags.resize(yao_or_secretsize);	
		}

		long get(int index)
		{
			int result = yao_or_NOTPRESENT;

			for(int i=0;i<vsize;i++)
			{
				long x = (i == index);
				x = makeSame(x);

				result = (x & data.at(i)) | (~x & result);
				//result = data.at(i);
			}
			for(int i=vsize;i<vsize+yao_or_secretsize;i++)
			{
				bool x = (i == index);
				//x = makeSame(x);
				flags[i-vsize] = (x & 1) | ((~x) & flags[i-vsize]);  
			}

			return result;
		}
		void set(int index, long value)
		{
			for(int i=0;i<vsize;i++)
			{
				long x = (i == index);
				x = makeSame(x);


				data.at(i) = (x & value) | (~x & data.at(i));

			}
		}

		//use this function only if the index is known at compiletime.
		void setIndexNOSECURITY(int index, long value)
		{
			data.at(index) = value;
		}

		int getSize()
		{
			return vsize;
		}

		vector<long> data;
		int vsize;
		vector<bool> flags;
};

class ORAM2_YAO_SPLIT
{
public:
    unsigned int oramsize;
     ORAM2_YAO_SPLIT() {}
     ORAM2_YAO_SPLIT(int size)
     {
         data.resize(size);
         vsize = size;
         oramsize = size;
	//flags.resize(secretsize);	
     }

    
     //#define NOTPRESENT 0x7FFFFFFFFFFFFFFF
     
     long get(int index)
     {
         long result = yao_or_split_NOTPRESENT;
         
         for(int i=0;i<vsize;i++)
         {
             long x = (i == index);
             x = makeSameLong(x);
             
             result = (x & data.at(i)) | (~x & result);
             //result = data.at(i);
         }
	//cout <<"results: "<< result <<" "<<data.at(index)<<"\n";
	 /*for(int i=vsize;i<vsize+secretsize;i++)
	 {
	     bool x = (i == index);
             //x = makeSame(x);
             flags[i-vsize] = (x & 1) | ((~x) & flags[i-vsize]);  
	 }*/
     
         return result;
     }
     void set(int index, long value)
     {
//cout << "value: "<<value<<"\n";
         for(int i=0;i<vsize;i++)
         {
             long x = (i == index);
             x = makeSameLong(x);
             
             
             data.at(i) = (x & value) | (~x & data.at(i));
             
         }
//cout << "value at index: "<<data.at(index)<<"\n";
     }
    
    //use this function only if the index is known at compiletime.
    void setIndexNOSECURITY(int index, long value)
    {
        data.at(index) = value;
    }
    
    int getSize()
    {
        return vsize;
    }
    
    vector<long> data;
    int vsize;
    //vector<bool> flags;
};


/* Internal APIs */
void transformToGarbled(const vector<char> & realbits, vector<vector<char> > & genvalues, vector<vector<char> > & evalvalues, vector<char> & permutes)
{
	genvalues.resize(realbits.size()*2);
	evalvalues.resize(realbits.size());

	permutes.resize(realbits.size());

	//srand(3);	

	//swap this out for sgx hardware rand function
	//	Prng m_prng;
	//	;

	for( int i=0;i<genvalues.size();i+=2)
	{
		genvalues[i].resize(10);
		genvalues[i+1].resize(10);

		for(int j=0;j<10;j++)
		{
			genvalues[i][j] = rand();//m_prng.rand(8)[0];
			genvalues[i+1][j] = rand();//m_prng.rand(16)[1];
		}

		if(((genvalues[i][0])&0x01) == ((genvalues[i+1][0])&0x01))
		{
			(genvalues[i][0])^= 0x01;
		}

		evalvalues[i/2].resize(10);

		//cout << "bit: "<<i/2<<" "<<realbits[i/2]<<"\n";

		if (((genvalues[i][0])&0x01) == 0)
		{
			permutes[i/2] = 0;
		}
		else
		{
			permutes[i/2] = 1;
		}


		for(int j=0;j<10;j++)
		{
			evalvalues[i/2][j] = realbits[i/2] == 0 ? genvalues[i][j] : genvalues[i+1][j]; 
		}

		//cout << "rb: "<<realbits[i/2]<<"\n";
	}	
}


Input generateInput(char input[], int size)
{

	Input toReturn;

	vector<vector<char> > evalgc;
	vector<vector<char> > gengc;

	vector<char> permutes;
	vector<char> iinputs; //{1,1,0,0,0,0,0,0  ,1,0,1,0,0,0,0,0};


	iinputs.resize(size);
	for(int i=0;i<size;i++){iinputs[i] = input[i];}


	transformToGarbled(iinputs, gengc,evalgc,permutes);

	toReturn.evalInput.evalgc = evalgc;
	toReturn.genInput.gengc = gengc;
	toReturn.genInput.permubits = permutes;

	return toReturn;
}



/* Global vars */
ORAM2_YAO yao_db;
ORAM2_YAO_SPLIT yao_db_split;


/* APIs */
void yao_or_init_db(unsigned int *input)
{
	int i;
	long a;

	yao_db = ORAM2_YAO(yao_or_datasize);

	for (i = 0; i < yao_or_datasize; i++) {
		a = input[i*2];
		a = a << 32;
		yao_db.set(i, a || input[i*2+1]);
	}
}

void yao_or_get_sgx_output(unsigned long *output, unsigned int *queries)
{
	int i;

	for(i = 0; i < yao_or_queries; i++)
		output[i] = yao_db.get(queries[i]); 
}

/* Used internally now */
void yao_or_get_bit_input(char *input)
{
	int i;

	for(i = 0; i < yao_or_secretsize; i++)
		input[i] = yao_db.flags[i];
}

int yao_or_gen_program_input(char *gengc, char *evlgc, char *permutes)
{
	int i, j;
	int inputLength;
	char *ptr;
	char bitInput[yao_or_secretsize];

	yao_or_get_bit_input(bitInput);
	inputLength = yao_or_secretsize;
	Input programInput = generateInput(bitInput,inputLength);
	
	/* Defensive checking ? */
#ifdef DEBUG_ENCLAVE_GC
	char hexval[10];
	std::string debugsz;
	
	for (i = 0; i < yao_or_secretsize; i++) {
		for (j = 0; j < 10; j++) {
			snprintf(hexval, 10, "%02X", (unsigned char)programInput.evalInput.evalgc[i][j]);
			debugsz += hexval;
			debugsz += " ";
		}
	}
	debugsz += "==========";
	for (i = 0; i < yao_or_secretsize*2; i++) {
		for (j = 0; j < 10; j++) {
			snprintf(hexval, 10, "%02X", (unsigned char)programInput.genInput.gengc[i][j]);
                        debugsz += hexval;
                        debugsz += " ";
		}
	}
	debugsz += "==========";
        for (i = 0; i < yao_or_secretsize; i++) {
		snprintf(hexval, 10, "%02X", (unsigned char)programInput.genInput.permubits[i]);
		debugsz += hexval;
		debugsz += " ";
        }

	eprintf("DEBUG: %s", debugsz.c_str());
#endif

	/* Copy out these vectors */
	ptr = evlgc;
	for (i = 0; i < yao_or_secretsize; i++) {
		memcpy(ptr, (char *)&(programInput.evalInput.evalgc[i][0]),
			HYBRID_OR_MAGIC_J);
		ptr += HYBRID_OR_MAGIC_J;
	}
	ptr = gengc;
	for (i = 0; i < yao_or_secretsize*2; i++) {
		memcpy(ptr, (char *)&(programInput.genInput.gengc[i][0]),
			HYBRID_OR_MAGIC_J);
		ptr += HYBRID_OR_MAGIC_J;
	}
	ptr = permutes;
	memcpy(ptr, (char *)&(programInput.genInput.permubits[0]),
		yao_or_secretsize);

	return 0;
}

void yao_or_split_entry(unsigned int *db,
			unsigned long *output,
			unsigned int *queries,
			char *gengc, char *evlgc, char *permutes)
{
	int i, j;
	long a;
	char *ptr;
	vector<long> dataofdb;
	char bitInput[yao_or_split_datasize*64];
	int inputLength = yao_or_split_datasize*64;

	/* Init DB */
        dataofdb.resize(yao_or_split_datasize);
	yao_db_split = ORAM2_YAO_SPLIT(yao_or_split_datasize);
	for (i = 0; i < yao_or_split_datasize; i++) {
		a = db[i*2];
		a = a << 32;
		dataofdb.at(i) = a | db[i*2+1];
		yao_db_split.set(i, dataofdb.at(i));
	}

	/* Generate SGX output */
	for(i = 0; i < yao_or_split_queries; i++)
		output[i] = yao_db_split.get(queries[i]);

	/* Generate GC input */
	for(i = 0; i < yao_or_split_datasize; i++)
 		for(j = 0; j < 64; j++)
			bitInput[i*64+j] = (dataofdb[i]>>j)&0x01;
 	Input programInput = generateInput(bitInput,inputLength);

	/* Copy out these vectors */
	ptr = evlgc;
	for (i = 0; i < yao_or_split_datasize*64; i++) {
		memcpy(ptr, (char *)&(programInput.evalInput.evalgc[i][0]),
			HYBRID_OR_MAGIC_J);
		ptr += HYBRID_OR_MAGIC_J;
	}
	ptr = gengc;
	for (i = 0; i < yao_or_split_datasize*64*2; i++) {
		memcpy(ptr, (char *)&(programInput.genInput.gengc[i][0]),
			HYBRID_OR_MAGIC_J);
		ptr += HYBRID_OR_MAGIC_J;
	}
	ptr = permutes;
	memcpy(ptr, (char *)&(programInput.genInput.permubits[0]),
		yao_or_split_datasize*64);
}

void yao_dj_entry(unsigned int *alicein, unsigned int *bobin, int *sgxout,
		char *gengc, char *evlgc, char *permutes)
{
   //input alice;
    vector<int> edges; edges.resize(yao_dj_Nodes*yao_dj_Edges_per_node);
    vector<int> connections; connections.resize(yao_dj_Nodes*yao_dj_Edges_per_node);
   

	/* daveti
	vector<unsigned int> alicein;
	vector<unsigned int> bobin;

	alicein.resize(Nodes*Edges_per_node*2);
	bobin.resize(2);
	
	fileToVector("hybriddijain.txt",alicein, alicein.size());
	fileToVector("hybriddijbin.txt",bobin,bobin.size());
	*/


 
    //input bob;
    int startnode;
    int endnode;
    
    
    for(int i=0;i<yao_dj_Nodes*yao_dj_Edges_per_node;i++)
    {
        edges.at(i) = 100;
        connections.at(i) = 0xFFFF;
    }
   
    //input from vector
	for(int i=0;i<yao_dj_Nodes*yao_dj_Edges_per_node;i++)	
	{
		//daveti: change to array
		edges.at(i) = alicein[i + yao_dj_Nodes*yao_dj_Edges_per_node];
		connections.at(i) = alicein[i];
	}



		

/* 
    for(int i=0;i<Nodes*Edges_per_node;i+=4)
    {
        connections.at(i) = (i+1)/4+1;
    }
    
    for(int i=4;i<Nodes*Edges_per_node-4;i+=4)
    {
        connections.at(i+1) = (i+1)/4+1;
        connections.at(i) = (i+1)/4-1;
    }
   

   for(int i=Nodes-SuperNodeSize;i<Nodes;i++)
   {
	edges.at(i*4)=1;
	edges.at(i*4+1)=1;
	edges.at(i*4+2)=1;
	edges.at(i*4+3)=1;
   } 
 
  connections.at((Nodes-SuperNodeSize-2)*Edges_per_node+1) =  Nodes-SuperNodeSize;  
    //connections.at(0) = 1;
    connections.at(Nodes*Edges_per_node-4) = Nodes-2;
    connections.at(Nodes*Edges_per_node-3) = Nodes-SuperNodeSize-1;//Nodes-SuperNodeSize-1;
   
    connections.at((Nodes-SuperNodeSize-1)*Edges_per_node+4) = Nodes-SuperNodeSize-2;//Nodes-SuperNodeSize; 
 connections.at((Nodes-SuperNodeSize-2)*Edges_per_node+4) = Nodes-1;   
 connections.at((Nodes-SuperNodeSize-2)*Edges_per_node+4+1) = 0xFFFF;//Nodes-SuperNodeSize; 
  
// connections.at((Nodes-SuperNodeSize-1)*Edges_per_node+4) = Nodes-SuperNodeSize-2;//Nodes-SuperNodeSize; 
  */ 
 
/*	GEN_BEGIN 
    for(int i=0;i<Nodes;i++)
    {
  	cout << connections.at(i*4)<< " " << connections.at(i*4+1) <<" "<< connections.at(i*4+2)<<" "<<connections.at(i*4+3)<<" "<< (i) <<"\n";
    }
	GEN_END
*/


    startnode = bobin[0];//0;
    endnode = bobin[1];//Nodes-1-SuperNodeSize;
    
 
    /*cout << "----\n";
    
    for(int i=0;i<Nodes*Edges_per_node;i++)
    {
        cout << connections.at(i)<<"\n";
    }
    
    cout << "----\n";*/
    
    
    
    
    
    
    //other
    vector<int> currentweight; currentweight.resize(yao_dj_Nodes);
    vector<int> inqueue; inqueue.resize(yao_dj_Nodes);
    
    
    vector<int> selectedconnections; selectedconnections.resize(yao_dj_Edges_per_node);
    vector<int> selectededges; selectededges.resize(yao_dj_Edges_per_node);
    vector<int> NodeLinks; NodeLinks.resize(yao_dj_Nodes);
    vector<int> Paths; Paths.resize(yao_dj_Nodes);
    
    for(int i=0;i<yao_dj_Nodes;i++)
    {
        NodeLinks.at(i) = 0x0000FFFF;
        Paths.at(i) = 0x0000FFFF;
        currentweight.at(i) = yao_dj_MaxWeight;
        inqueue.at(i) = 0;
        
        /*if(i == startnode)
        {
            inqueue[i] = 1;
            currentweight[i] =0;
        }*/
        
        unsigned int t = i == startnode;
        t = makeSame(t);
        inqueue.at(i) = (t & 1) | ((~t) & inqueue.at(i));
        currentweight.at(i) = (t & 0) | ((~t) & currentweight.at(i));
    }
    
    int isDone;
    
    for(int i=0;i<yao_dj_MaxIterations;i++)
    {
        int selectedWeight=yao_dj_MaxWeight;
        int selectedNode=0xFFFFFF;
        int cconnection;
        
        for(int j=0;j<yao_dj_Nodes;j++)
        {
            /*if(inqueue[j] && selectedWeight > currentweight[j])
            {
                selectedNode = j;
                selectedWeight = currentweight[j];
            }*/
            
            unsigned int t = (inqueue.at(j) & (selectedWeight > currentweight.at(j)));
            t = makeSame(t);
            selectedNode = (t & j) | ((~t) & selectedNode);
            selectedWeight = (t & currentweight.at(j)) | ((~t) & selectedWeight);
            
        }
        
        /*if(selectedNode == endnode)
        {
            isDone = 1;
        }*/
        
        unsigned int t = (selectedNode == endnode);
        t = makeSame(t);
        isDone = (t & 1) | ((~t) & isDone);
        
        
        for (int j =0; j < yao_dj_Nodes; j++)
        {
            /*if ( selectedNode == j)
            {
                inqueue[j] = 0;
            }*/
            
            t = (selectedNode == j);
            t = makeSame(t);
            inqueue.at(j) = (t & 0) | ((~t) & inqueue.at(j));
        }
        
        selectedconnections.at(0) = selectedconnections.at(1) = selectedconnections.at(2) = selectedconnections.at(3) = 0xFFFFFF;
        
        
        for(int j=0;j< yao_dj_Nodes;j++)
        {
            /*if(j == selectedNode)
            {
                
                
                selectedconnections[0] = connections[j*Edges_per_node];
                selectedconnections[1] = connections[j*Edges_per_node+1];
                selectedconnections[2] = connections[j*Edges_per_node+2];
                selectedconnections[3] = connections[j*Edges_per_node+3];
                
                selectededges[0] = edges[j*Edges_per_node];
                selectededges[1] = edges[j*Edges_per_node+1];
                selectededges[2] = edges[j*Edges_per_node+2];
                selectededges[3] = edges[j*Edges_per_node+3];
            }*/
            
            t = (selectedNode == j);
            t = makeSame(t);
            
            selectedconnections.at(0) = (t & connections.at(j*yao_dj_Edges_per_node)) | ((~t) & selectedconnections.at(0));
            selectedconnections.at(1) = (t & connections.at(j*yao_dj_Edges_per_node+1)) | ((~t) & selectedconnections.at(1));
            selectedconnections.at(2) = (t & connections.at(j*yao_dj_Edges_per_node+2)) | ((~t) & selectedconnections.at(2));
            selectedconnections.at(3) = (t & connections.at(j*yao_dj_Edges_per_node+3)) | ((~t) & selectedconnections.at(3));
            
            selectededges.at(0) = (t & edges.at(j*yao_dj_Edges_per_node)) |   ((~t) & selectededges.at(0));
            selectededges.at(1) = (t & edges.at(j*yao_dj_Edges_per_node+1)) | ((~t) & selectededges.at(1));
            selectededges.at(2) = (t & edges.at(j*yao_dj_Edges_per_node+2)) | ((~t) & selectededges.at(2));
            selectededges.at(3) = (t & edges.at(j*yao_dj_Edges_per_node+3)) | ((~t) & selectededges.at(3));
            
        }
        
        /*cout << "selectednode: "<<selectedNode<<"\n";
        for(int i=0;i<4;i++)
        {
            cout << selectedconnections[i] <<" "<<selectededges[0]<<"\n";
        }*/
        
        
        
        int newweight = 0;
        int oldweight;
        int oldlink;
        
        for(int j=0;j<yao_dj_Edges_per_node;j++)
        {
            int cweight = selectededges.at(j);
            cconnection = selectedconnections.at(j);
            newweight = selectedWeight + cweight;
            
            //cout << "cw: "<<cweight<<" "<<selectedWeight<<"\n";
            
            for(int k=0;k<yao_dj_Nodes;k++)
            {
                /*if(k == cconnection)
                {
                    oldweight = currentweight[k];
                    oldlink = NodeLinks[k];
                }*/
                
                t = (k == cconnection);
                t = makeSame(t);
                oldweight = (t & currentweight.at(k)) | ((~t) & oldweight);
                oldlink = (t & NodeLinks.at(k)) | ((~t) & oldlink);
            }
            
            int change = 1;
            int newlink = selectedNode;
            
           
            t = (oldweight < newweight);
            t = makeSame(t);
            newlink = (t & oldlink) | ((~t) & newlink);
            newweight = (t & oldweight) | ((~t) & newweight);
            change = (t & 0) | ((~t) & change);
            
            
            for(int k=0;k<yao_dj_Nodes;k++)
            {
               
                t = (k == cconnection);
                t = makeSame(t);
                currentweight.at(k) = (t & newweight) | ((~t) & currentweight.at(k));
                NodeLinks.at(k) = (t & newlink) | ((~t) & NodeLinks.at(k));
                
            }
            
            //cout << "new conc: "<<cconnection<<"\n";
            
            for(int k=0;k<yao_dj_Nodes;k++)
            {
               
                t = ((k == cconnection) & (change == 1));
                t = makeSame(t);
                inqueue.at(k) = (t & 1) | ((~t) & inqueue.at(k));
            }
        }
    }
    
    
    int nodeToGet = endnode;
    for(int i=0; i<yao_dj_Nodes; i++)
    {
        int goit =0;
        for(int j=0;j<yao_dj_Nodes;j++)
        {
           
            unsigned int t = ((goit == 0) & (j == nodeToGet));
            t = makeSame(t);
            nodeToGet = (t & NodeLinks.at(j)) | ((~t) & nodeToGet);
            Paths.at(i) = (t & j) | ((~t) & Paths.at(i));
            goit = (t & 1) | ((~t) & goit);
        }
    }
    
	/* output to sender*/
/* daveti:
    GEN_BEGIN
    for(int i=0;i<Nodes;i++)
    {
        cout << Paths.at(i) <<" ";
    }
    cout << endl;
    GEN_END
*/
	//daveti: copy out the sgx output
	memcpy(sgxout, (char *)&Paths[0], HYBRID_DJ_SGX_OUTPUT_LEN);




	int start;
	int end;
	bool started=false;

	int prestarttemp,prestart;
	int postendtemp,postend;
	
	
	for(int i=0;i<yao_dj_Nodes;i++)
	{
		if(i > 0)
		{
			prestarttemp = Paths.at(i-1);
		}
		if( i < yao_dj_Nodes-1)
		{
			postendtemp = Paths.at(i+1);
		}

		int res = (Paths.at(i) >= (yao_dj_Nodes - yao_dj_SuperNodeSize)) & (Paths.at(i) < yao_dj_Nodes); 
	
		int res2 = (!started) & res;
		res2 = makeSame(res2);
		prestart = (res2 & prestarttemp) | ((~res2) & prestart);
		start = (res2 & Paths.at(i)) | ((~res2) & start);
		started = (res2 & 1) | ((~res2)& started);
		end = (res2 & Paths.at(i)) | ((~res2) & end);
		
		int res3 = (started) & res;
		res3 = makeSame(res3);
		end = (res3 & Paths.at(i)) | ((~res3) & end);
		

		/*if(Paths.at(i) >= Nodes - SuperNodeSize & Paths.at(i) < Nodes)
		{
			if(!started)
			{
				prestart = prestarttemp;
				start = Paths.at(i);
				started = true;
				end = Paths.at(i);
			}
			else
			{
				end = Paths.at(i);
			}
		}*/
		if(i > 0)
		{
		/*if(Paths.at(i) >= Nodes - SuperNodeSize & Paths.at(i) < Nodes & started)
		{
			
			
			postend = postendtemp;
			
		}*/
			int res4 = Paths.at(i) >= yao_dj_Nodes - yao_dj_SuperNodeSize & Paths.at(i) < yao_dj_Nodes & started;
			res4 = makeSame(res4);
			postend = (res4 & postendtemp) | ((~res4) & postend);
		}

	}

	int posnodes =yao_dj_SuperNodeSize*2+2;
	int startfan,endfan;	

	for(int i=0;i<yao_dj_Nodes;i++)
	{
		for(int j=0;j<yao_dj_Edges_per_node;j++)
		{
			int res = ((start == i) & (connections[i*yao_dj_Edges_per_node+j]==prestart));
			res = makeSame(res);
			startfan = (res & j) | ((~res) & startfan); 
			/*if((start == i) & (connections[i*Edges_per_node+j]==prestart))
			{
			//	cout << "startfan "<<j<<" "<< connections[i*Edges_per_node+j]<<"\n";
				
			//	if(connections[i*Edges_per_node+j]==prestart)
				{
					startfan = j;
				}
			}*/
			res = ((end == i) & (connections[i*yao_dj_Edges_per_node+j]==postend));
			res = makeSame(res);
			endfan = (res & j) | ((~res) & endfan);
			/*if((end == i) & (connections[i*Edges_per_node+j]==postend))
			{
			//	cout << "endfan "<<j<<" "<< connections[i*Edges_per_node+j]<<"\n";
				//if()
				{
					endfan = j;
				}
			}*/
		}
	}
	

	//cout << "start: "<< start << "/"<<prestart <<" end: "<<end<<"/"<<postend<<"\n" << startfan <<" "<<endfan<<"\n";

	start = start - (yao_dj_Nodes - yao_dj_SuperNodeSize);
	end = end - (yao_dj_Nodes - yao_dj_SuperNodeSize); 

	
	//cout << start <<" "<<startfan <<" "<<end<<" "<<endfan<<"\n";

	int startcount=0;
	
	int resx = start == 0;
	resx = makeSame(resx);
	startcount = (resx & startfan) | ((~resx) & startcount);

	resx = start > 0 & start < yao_dj_SuperNodeSize-1;
	resx = makeSame(resx);
	startcount = (resx & (3+(start-1)*2+startfan)) | ((~resx) & startcount);

	resx = start >= yao_dj_SuperNodeSize-1;
	resx = makeSame(resx);
	startcount =   3 + (yao_dj_SuperNodeSize-2)*2+startfan;

	/*if(start == 0)
	{
		startcount = startfan;
	}
	if(start > 0 & start < SuperNodeSize-1)
	{
		startcount = 3+(start-1)*2+startfan;
	}
	if(start >= SuperNodeSize-1)
	{
		startcount = 3 + (SuperNodeSize-2)*2+startfan;
	}*/

	int endcount=0;
	
	resx = end == 0;
	resx = makeSame(resx);
	endcount = (resx & endfan) | ((~resx)& endcount);

	resx = end > 0 & end < yao_dj_SuperNodeSize-1;
	resx = makeSame(resx);
	endcount = (resx & (3+(end-1)*2+endfan)) | ((~resx) & endcount);
	
	resx = end >= yao_dj_SuperNodeSize-1;
 	resx = makeSame(resx);
	endcount = (resx & ( 3 + (yao_dj_SuperNodeSize-2)*2+endfan )) | ((~resx) & endcount);


	/*if(end == 0)
	{
		endcount = endfan;
	}
	if(end > 0 & end < SuperNodeSize-1)
	{
		endcount = 3+(end-1)*2+endfan;
	}
	if(end >= SuperNodeSize-1)
	{
		endcount = 3 + (SuperNodeSize-2)*2+endfan;
	}*/

	//cout <<"counts: "<< startcount <<" "<<endcount<<"\n";

	char bitInput[yao_dj_dijsgxinput];


	for(int i=0;i<16;i++)
	{
		bitInput[i] = (startcount >> i)&1;
	}
	for(int i=0;i<16;i++)
	{
		bitInput[i+16] = (endcount >> i)&1;
	}

	//bitInput[16] = 1;

	int inputLength = yao_dj_dijsgxinput;
	
//cout << "here1\n";

	Input programInput = generateInput(bitInput,inputLength);

        /* daveti: copy out these vectors */
	char *ptr;
        ptr = evlgc;
	int i;
        for (i = 0; i < yao_dj_dijsgxinput; i++) {
                memcpy(ptr, (char *)&(programInput.evalInput.evalgc[i][0]),
                        HYBRID_DJ_MAGIC_J);
                ptr += HYBRID_DJ_MAGIC_J;
        }
        ptr = gengc;
        for (i = 0; i < yao_dj_dijsgxinput*2; i++) {
                memcpy(ptr, (char *)&(programInput.genInput.gengc[i][0]),
                        HYBRID_DJ_MAGIC_J);
                ptr += HYBRID_DJ_MAGIC_J;
        }
        ptr = permutes;
        memcpy(ptr, (char *)&(programInput.genInput.permubits[0]),
                yao_dj_dijsgxinput);
}

