/*
 * SFE naive programs running in the enclave
 * May 16, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include "mysfe.h"
#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <deque>

static int use_bens_hacking = 1;

/* for SFE DJ */
static int edges[sfe_dj_Nodes*sfe_dj_Edges_per_node];
static int connections[sfe_dj_Nodes*sfe_dj_Edges_per_node];

/* for SFE UC */
class Program
{
	public:
		short x[sfe_uc_gatecount];
		short y[sfe_uc_gatecount];
		char tt[sfe_uc_gatecount];
		short d[sfe_uc_gatecount];

		short output[sfe_uc_numoutputs];

		void blank()
		{
			for(int i=0;i<sfe_uc_gatecount;i++)
			{
				x[i] = y[i] = tt[i] = d[i]=0;
			}
			for(int i=0;i<sfe_uc_numoutputs;i++)
			{
				output[i] = 0;
			}
		}
};

/* Global vars */
static Program mysfe_uc_input;


static void init_mysfe_uc_input(sfe_uc_input_program *prog)
{
	if (use_bens_hacking) {
		/* this is a tempory test programy */
		for(int i=0;i<sfe_uc_gatecount;i++)
		{
			mysfe_uc_input.x[i] = i % sfe_uc_poolsize;
			mysfe_uc_input.y[i] = i % sfe_uc_poolsize;
			mysfe_uc_input.tt[i] = 5;
			mysfe_uc_input.d[i] = i % sfe_uc_poolsize;
			if (i < sfe_uc_numoutputs)
				mysfe_uc_input.output[i] = i % sfe_uc_poolsize;
		}
	
		return;
	}

	if (prog) {
		/* Init using user's input */
		memcpy(mysfe_uc_input.x, prog->x,
				sfe_uc_gatecount*sizeof(short));
		memcpy(mysfe_uc_input.y, prog->y,
				sfe_uc_gatecount*sizeof(short));
		memcpy(mysfe_uc_input.tt, prog->tt,
				sfe_uc_gatecount*sizeof(char));
		memcpy(mysfe_uc_input.d, prog->d,
				sfe_uc_gatecount*sizeof(short));
		memcpy(mysfe_uc_input.output, prog->output,
				sfe_uc_numoutputs*sizeof(short));
		return;
	}

	/* Hardcode the input */
	mysfe_uc_input.blank();

	/* this is a tempory test programy */
	for(int i=0;i<sfe_uc_gatecount;i++)
	{
		mysfe_uc_input.x[i] = i;
		mysfe_uc_input.y[i] = i;
		mysfe_uc_input.tt[i] = 5;
		mysfe_uc_input.d[i] = i;
		//daveti: segv line
		//input1.output[i] = i;
		if (i < sfe_uc_numoutputs)
			mysfe_uc_input.output[i] = i;
	}
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

static int makeSame(int value)
{
	value = value | (value << 16);
	value = value | (value << 8);
	value = value | (value << 4);
	value = value | (value << 2);
	value = value | (value << 1);

	return value;
}

static int lessequalto(unsigned int alice[sfe_mil_size],
		unsigned int bob[sfe_mil_size])
{
	unsigned int x=0;
	unsigned int i;
	unsigned int borrow=0;

	for(i = 0; i < sfe_mil_size;)
	{
		unsigned int a1 = alice[i];
		unsigned int b1 = bob[i];
		unsigned int b = borrow;
		borrow = 0;

		/*unsigned int t = (a1 < (b1 + b));
		  t = makeSame(t);
		  borrow = (t & 0x1) | ((~t) & borrow);*/

		if(a1 < (b1 + b))
		{
			borrow = 1;
		}

		i++;
	}

	/*unsigned int res = (borrow == 0);

	  res = makeSame(res);
	  x = (res & 0x1) | ((~res) & x);

	  res = ~res;
	  x = (res & 0x0) | ((~res) & x);*/

	//x = 0;
	if(borrow == 0)
	{
		x = 1;
	}
	else
	{
		x = 0;
	}

	return x;
}

void sfe_naive_entry(unsigned int alice[sfe_mil_size],
		unsigned int bob[sfe_mil_size],
		unsigned int *alice_out,
		unsigned int *bob_out)
{
	unsigned int outputalice = lessequalto(alice,bob);
	unsigned int outputbob = lessequalto(bob,alice);

	/*unsigned int res = (outputalice == 1) & (outputbob == 1);

	  outputbob = (res & 0x0) | ((~res) & outputbob);
	  outputalice = (res & 0x0) | ((~res) & outputalice);*/

	if((outputalice == 1) & (outputbob == 1))
	{
		outputbob = 0;
		outputalice = 0;
	}

	memcpy(alice_out, &outputalice, sizeof(unsigned int));
	memcpy(bob_out, &outputbob, sizeof(unsigned int));
}

void sfe_naive_uc_entry(sfe_uc_input_program *prog,
		bool input[sfe_uc_numinputs],
		bool output[sfe_uc_numoutputs])
{
	init_mysfe_uc_input(prog);

	bool destinations[sfe_uc_poolsize];
	bool outputs[sfe_uc_numoutputs];

	for(int i=0;i<sfe_uc_poolsize;i++)
	{
		destinations[i] = 0;
	}

	for(int i=0;i<sfe_uc_numoutputs;i++)
	{
		outputs[i] = 0;
	}

	for(int i=0;i<sfe_uc_numinputs;i++)
	{
		destinations[i] = input[i];
	}

	for(int i=0;i<sfe_uc_gatecount;i++)
	{
		short xad = mysfe_uc_input.x[i],
		      yad = mysfe_uc_input.y[i],
		      dad = mysfe_uc_input.d[i];

		char truth = mysfe_uc_input.tt[i];
		bool x,y;

		/*for(int j=0;j<sfe_uc_poolsize;j++)
		  {

		  unsigned int t = xad == j;
		  t = makeSame(t);
		  x = (t & destinations[j]) | ((~t) & x);

		  t = yad == j;
		  t = makeSame(t);
		  y = (t & destinations[j]) | ((~t) & y);

		  }*/

		x = destinations[xad];
		y = destinations[yad];

		char ttnum = y | (x <<1);
		bool res;

		/*if(ttnum == 0)
		  {
		  res = truth&0x1;
		  }
		  if(ttnum == 1)
		  {
		  res = (truth&0x3) >> 1;
		  }
		  if(ttnum == 2)
		  {
		  res = (truth&0x7) >> 2;
		  }
		  if(ttnum == 3)
		  {
		  res = (truth&0xF) >> 3;
		  }*/

		res = (truth >> ttnum) & 0x1;

		/*for(short j=0;j<sfe_uc_poolsize;j++)
		  {
		  unsigned int t = dad == j;
		  t = makeSame(t);
		  destinations[j] = (t & res) | ((~t) & destinations[j]);

		  }*/

		destinations[dad] = res;

	}

	for(short i=0;i<sfe_uc_numoutputs;i++)
	{
		/*for(short j=0;j<sfe_uc_poolsize;j++)
		  {
		  unsigned int t = mysfe_uc_input.output[i] == j;
		  t = makeSame(t);
		  outputs[i] = (t & destinations[j]) | ((~t) & outputs[i]);

		  if(input1.output[i] == j)
		  {
		  outputs[i] = destinations[j];
		  }
		  }*/

		outputs[i] = destinations[mysfe_uc_input.output[i]];
	}

	/* Copy out the outputs */
	memcpy(output, outputs, SMC_SGX_SFE_UC_OUTPUT_LEN);
}

/* SFE Dijk entry point */
void sfe_naive_dj_entry(sfe_dj_input_eval *evl_input,
		sfe_dj_input_send *gen_input,
		int output[sfe_dj_Nodes])
{
	//input alice - sender
	/* daveti: avoid stack overflow
	int edges[sfe_dj_Nodes*sfe_dj_Edges_per_node];
	int connections[sfe_dj_Nodes*sfe_dj_Edges_per_node];
	*/

	//input bob - evaluator
	int startnode;
	int endnode;

	//other
	int currentweight[sfe_dj_Nodes];
	int inqueue[sfe_dj_Nodes];

	int selectedconnections[sfe_dj_Edges_per_node];
	int selectededges[sfe_dj_Edges_per_node];
	int NodeLinks[sfe_dj_Nodes];
	int Paths[sfe_dj_Nodes];

	//daveti: Init the inputs using the passed value
	if (evl_input) {
		startnode = evl_input->startnode;
		endnode = evl_input->endnode;
		memcpy(edges, gen_input->edges, SMC_SGX_SFE_DJ_INPUT_EDGES_LEN);
		memcpy(connections, gen_input->connections,
				SMC_SGX_SFE_DJ_INPUT_CONNS_LEN);
	} else {
		// Hardcode the input
		startnode = 5;
		endnode = 0;

		for(int i=0;i<sfe_dj_Nodes*sfe_dj_Edges_per_node;i++)
		{
			edges[i] = 0;
			connections[i] = 0xFFFF;
		}


		edges[0] = 10;
		connections[0] = 1;

		edges[1] = 1;
		connections[1] = 2;



		edges[4] = 10;
		connections[4] = 0;

		edges[5] = 1;
		connections[5] = 2;

		edges[6] = 1;
		connections[6] = 3;



		edges[8] = 1;
		connections[8] = 0;

		edges[9] = 1;
		connections[9] = 1;

		edges[10] = 1;
		connections[10] = 4;



		edges[12] = 1;
		connections[12] = 1;

		edges[13] = 1;
		connections[13] = 4;

		edges[14] = 1;
		connections[14] = 5;


		edges[16] = 1;
		connections[16] = 2;

		edges[17] = 1;
		connections[17] = 3;

		edges[18] = 10;
		connections[18] = 5;



		edges[20] = 1;
		connections[20] = 3;

		edges[21] = 10;
		connections[21] = 4;
	}
	//daveti: Init done

	for(int i=0;i<sfe_dj_Nodes;i++)
	{
		NodeLinks[i] = 0x0000FFFF;
		Paths[i] = 0x0000FFFF;
		currentweight[i] = sfe_dj_MaxWeight;
		inqueue[i] = 0;

	}

	inqueue[startnode] = 1;
	currentweight[startnode] = 1;

	int isDone = 0;

	for(int i=0;i<sfe_dj_MaxIterations;i++)
	{
		int selectedWeight=sfe_dj_MaxWeight;
		int selectedNode=0xFFFFFF;
		int cconnection;

		for(int j=0;j<sfe_dj_Nodes;j++)
		{
			if(inqueue[j] && selectedWeight > currentweight[j])
			{
				selectedNode = j;
				selectedWeight = currentweight[j];
			}
		}


		unsigned int t = (selectedNode == endnode);
		t = makeSame(t);
		isDone = (t & 1) | ((~t) & isDone);

		if(selectedNode< sfe_dj_Nodes)
			inqueue[selectedNode] = 0;


		selectedconnections[0] = selectedconnections[1] = selectedconnections[2] = selectedconnections[3] = 0xFFFFFF;


		//for(int j=0;j< Nodes;j++)
		{
			if(selectedNode < sfe_dj_Nodes)
			{


				selectedconnections[0] = connections[selectedNode*sfe_dj_Edges_per_node];
				selectedconnections[1] = connections[selectedNode*sfe_dj_Edges_per_node+1];
				selectedconnections[2] = connections[selectedNode*sfe_dj_Edges_per_node+2];
				selectedconnections[3] = connections[selectedNode*sfe_dj_Edges_per_node+3];

				selectededges[0] = edges[selectedNode*sfe_dj_Edges_per_node];
				selectededges[1] = edges[selectedNode*sfe_dj_Edges_per_node+1];
				selectededges[2] = edges[selectedNode*sfe_dj_Edges_per_node+2];
				selectededges[3] = edges[selectedNode*sfe_dj_Edges_per_node+3];
			}


		}

		/*cout << "selectednode: "<<selectedNode<<"\n";
		  for(int i=0;i<4;i++)
		  {
		  cout << selectedconnections[i] <<" "<<selectededges[0]<<"\n";
		  }*/



		int newweight = 0;
		int oldweight;
		int oldlink;

		for(int j=0;j<sfe_dj_Edges_per_node ;j++)
		{

			int cweight = selectededges[j];
			cconnection = selectedconnections[j];
			newweight = selectedWeight + cweight;

			if(cconnection >= sfe_dj_Nodes)
			{
				continue;
			}


			//cout << "cw: "<<cweight<<" "<<selectedWeight<<"\n";

			if(cconnection < sfe_dj_Nodes)
			{
				oldweight = currentweight[cconnection];
				oldlink = NodeLinks[cconnection];
			}


			int change = 1;
			int newlink = selectedNode;

			//cout << "oldwieght: "<<oldweight <<" "<<newweight<<"\n";

			if(oldweight < newweight)
			{
				newlink = oldlink;
				newweight = oldweight;
				change = 0;
			}

			if(cconnection < sfe_dj_Nodes)
			{
				currentweight[cconnection] = newweight;
				NodeLinks [ cconnection] = newlink;
				if(change)
				{
					inqueue[cconnection] = 1;
				}
			}
		}
	}


	int nodeToGet = endnode;
	for(int i=0; i<sfe_dj_Nodes; i++)
	{
		if(nodeToGet < sfe_dj_Nodes)
		{
			Paths[i] = nodeToGet;
			nodeToGet = NodeLinks[nodeToGet];
		}
	}

	memcpy(output, Paths, SMC_SGX_SFE_DJ_OUTPUT_LEN);
}

