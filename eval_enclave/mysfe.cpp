/*
 * SFE programs running in the enclave
 * Based on mil.cpp by Dr. Ben Mood
 * May 3, 2016
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include "mysfe.h"
#include "util.h"
#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <deque>


/* For SFE Dijkstra */
static int edges[sfe_dj_Nodes*sfe_dj_Edges_per_node];
static int connections[sfe_dj_Nodes*sfe_dj_Edges_per_node];

/* For SFE ORAM DB */
using std::vector;
using std::deque;
using std::string;

static unsigned int makeSame(unsigned int value);
static int makeSame(int value);
static long makeSame(long value);

#define RDTSC ({unsigned long long res;  unsigned hi, lo;   __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi)); res =  ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );res;})
unsigned long startTime, endTime;

typedef struct PairNode
{
	long data;
	int index;

	bool added;

	PairNode * left;
	PairNode * right;
	PairNode * parent;
} PairNode;



PairNode * makeSame(PairNode * valuein)
{
	long value = (long) valuein;
	value = value | (value << 32);
	value = value | (value << 16);
	value = value | (value << 8);
	value = value | (value << 4);
	value = value | (value << 2);
	value = value | (value << 1);
	return (PairNode *)value;


}

/* fix SGX STL header bug...
 * Error is that there is no long long int rand overload
 * /opt/intel/sgxsdk/include/stlport/stl/_algo.c:540:20: error: call of overloaded 'abs(long long int&)' is ambiguous
 return abs(__rand) % __n;
 * /opt/intel/sgxsdk/include/stlport/stl/_cstdlib.h:139:13: note: long int abs(long int)
 */
long int rand_func(long long int a)
{
	long int __rand;
	if (SGX_SUCCESS != sgx_read_rand(reinterpret_cast<unsigned char *>(&__rand), sizeof(__rand))) {
		// XXX FAIL HERE
		return 0;
	}

	return abs(__rand) % a;
}

int rand()
{
	int val;

	if (SGX_SUCCESS != sgx_read_rand(reinterpret_cast<unsigned char *>(&val), sizeof(val))) {
		// XXX FAIL HERE
		return 0;
	}

	return abs(val);
}

#define NOTPRESENT 0x7FFFFFFFFFFFFFFF

class ORAMpre
{
	public:



		ORAMpre()
		{
			root=0;
		}


		/*this function must be on 1 cache page*/
		vector<int> indexes;
		void set(int index, long value, int isreal)
		{
			PairNode * node = findset(index,value, isreal);

		}

		long toget;
		long get(int index)
		{
			toget = NOTPRESENT;
			findget(index);
			return toget;


			/*PairNode * found = find(index);

			  if(found == 0)
			  {

			  return NOTPRESENT;
			  }

			  int v =found->data;
			  return v;

			  return found;*/
		}





		void addNode(int index, long value)
		{
			if(root == 0)
			{
				root = getBlankNode();
				root->index = index;
				root->data = value;
			}
			else
			{
				add_decend(root,index,value);
			}
		}

		void add_decend(PairNode * parent, int index, long value)
		{
			if(index > parent->index)
			{
				if(parent->right == 0)
				{
					parent->right = getBlankNode();
					parent->right->index = index;
					parent->right->data = value;
					parent->right->parent = parent;
				}
				else
				{
					add_decend(parent->right,index,value);
				}
			}
			else if(index < parent->index)
			{
				if(parent->left == 0)
				{
					parent->left = getBlankNode();
					parent->left->index = index;
					parent->left->data = value;
					parent->left->parent = parent;
				}
				else
				{
					add_decend(parent->left,index,value);
				}
			}
			else
			{
				//cout << "ERROR1: ORAM ADDING AN INDEX ALREADY CONTAINED\n";
			}
		}


		void findget(int index)
		{
			//cout << "find start\n";
			foundnode=0;

			if(root == 0)
			{
				//cout << "find end1\n";
				return ;
			}

			//toBeReordered.clear();


			/*int num = (int)log2(oramsize);

			  int realindex = rand()%num;
			  int indexes[num];
			  for(int i=0;i<num;i++)
			  {
			  indexes[i] = rand()%oramsize;

			  int s = i == realindex;
			  s = makeSame(s);

			//get results -> this statement selects a if s is true and b if s is false.
			indexes[i] = (s & index) | (~s & indexes[i]);
			}



			PairNode * queryResults[num];

			for(int i=0;i<num;i++)
			{
			find_decend(root,indexes[i]);
			queryResults[i] = foundnode;
			}

			for(int i=0;i<num;i++)
			{
			PairNode * res5 = (PairNode *) (i == realindex);
			res5 = makeSame(res5);
			foundnode = (PairNode *) ( (((long)res5) & ((long)(queryResults[i]))) | (~((long)res5) & ((long)foundnode)));
			}*/



			findget_decend(root,index);

		}

		void findget_decend(PairNode * parent, int tofindindex)
		{
			if(parent == 0)
			{
				return;
			}



			if(!parent->added)
			{
				toBeReordered.push_back(parent);
				parent->added = 1;
			}


			PairNode * todecend;

			int node = rand()%2;
			//sgx_read_rand((unsigned char *)&node, sizeof(node));
			//node = node % 2;


			/*todecend = parent->right;*/
			PairNode * res1 = (PairNode *) (tofindindex > parent->index);

			res1 = makeSame(res1);

			todecend = (PairNode *) ( (((long)res1) & ((long)(parent->right))) | (~((long)res1) & ((long)todecend)));



			PairNode * res2 = (PairNode *) (tofindindex < parent->index);

			res2 = makeSame(res2);

			todecend = (PairNode *) ( (((long)res2) & ((long)(parent->left))) | (~((long)res2) & ((long)todecend)));


			PairNode * res3 = (PairNode *) (tofindindex == parent->index);
			res3 = makeSame(res3);

			foundnode = (PairNode *) ( (((long)res3) & ((long)(parent))) | (~((long)res3) & ((long)foundnode)));


			long res6 = (tofindindex == parent->index);

#ifdef ORAM_CTRL_CHANNEL
			trace_mem_read("TreeORAM::Get", parent);

			trace_mem_read("TreeORAM::Get", parent->right);
			trace_mem_read("TreeORAM::Get", &parent->index);

			trace_mem_read("TreeORAM::Get", parent->left);
			trace_mem_read("TreeORAM::Get", &parent->index);

			trace_mem_read("TreeORAM::Get", parent);
			trace_mem_read("TreeORAM::Get", &parent->index);
#endif


			//(s & a) | (~s & b);
			res6 = makeSame(res6);
			toget = (res6 & parent->data) | (~res6 & toget);


#ifdef ORAM_CTRL_CHANNEL
			trace_mem_rw("TreeORAM::Get2", &parent->data);
#endif


			PairNode * res4 = (PairNode *) (node == 1);
			res4 = makeSame(res4);
			res4 = (PairNode *) ( (long)res4 & (long)res3 );

			todecend = (PairNode *) ( (((long)res4) & ((long)(parent->right))) | (~((long)res4) & ((long)todecend)));


			PairNode * res5 = (PairNode *) (node != 1);
			res5 = makeSame(res5);
			res5 = (PairNode *) ( (long)res5 & (long)res3 );

			todecend = (PairNode *) ( (((long)res5) & ((long)(parent->left))) | (~((long)res5) & ((long)todecend)));

#ifdef ORAM_CTRL_CHANNEL
			trace_mem_read("TreeORAM::Get3", parent->right);
			trace_mem_read("TreeORAM::Get3", parent->left);
#endif


			findget_decend(todecend,tofindindex);
		}



		PairNode * foundnode;
		PairNode * findset(int index, long value, int isreal)
		{
			//cout << "find start\n";
			foundnode=0;

			if(root == 0)
			{
				//cout << "find end1\n";
				return 0;
			}

			//toBeReordered.clear();


			/*int num = (int)log2(oramsize);

			  int realindex = rand()%num;
			  int indexes[num];
			  for(int i=0;i<num;i++)
			  {
			  indexes[i] = rand()%oramsize;

			  int s = i == realindex;
			  s = makeSame(s);

			//get results -> this statement selects a if s is true and b if s is false.
			indexes[i] = (s & index) | (~s & indexes[i]);
			}



			PairNode * queryResults[num];

			for(int i=0;i<num;i++)
			{
			find_decend(root,indexes[i]);
			queryResults[i] = foundnode;
			}

			for(int i=0;i<num;i++)
			{
			PairNode * res5 = (PairNode *) (i == realindex);
			res5 = makeSame(res5);
			foundnode = (PairNode *) ( (((long)res5) & ((long)(queryResults[i]))) | (~((long)res5) & ((long)foundnode)));
			}*/



			findset_decend(root,index, value, isreal);

			//cout << "find end2\n";
			return foundnode;
		}

		void findset_decend(PairNode * parent, int tofindindex, long value, int isreal)
		{
			if(parent == 0)
			{
				return;
			}


			if(!parent->added)
			{
				toBeReordered.push_back(parent);
				parent->added = 1;
			}

			PairNode * todecend;

			int node = rand()%2;
			//sgx_read_rand((unsigned char *)&node, sizeof(node));
			//node = node % 2;


			/*todecend = parent->right;*/
			PairNode * res1 = (PairNode *) (tofindindex > parent->index);

#ifdef ORAM_CTRL_CHANNEL
			trace_mem_read("TreeORAM::Set", parent);

			trace_mem_read("TreeORAM::Set", parent->right);
			trace_mem_read("TreeORAM::Set", &parent->index);

			trace_mem_read("TreeORAM::Set", parent->left);
			trace_mem_read("TreeORAM::Set", &parent->index);

			trace_mem_read("TreeORAM::Set", parent);
			trace_mem_read("TreeORAM::Set", &parent->index);
#endif

			res1 = makeSame(res1);

			todecend = (PairNode *) ( (((long)res1) & ((long)(parent->right))) | (~((long)res1) & ((long)todecend)));



			PairNode * res2 = (PairNode *) (tofindindex < parent->index);

			res2 = makeSame(res2);

			todecend = (PairNode *) ( (((long)res2) & ((long)(parent->left))) | (~((long)res2) & ((long)todecend)));


			PairNode * res3 = (PairNode *) (tofindindex == parent->index);
			res3 = makeSame(res3);

			foundnode = (PairNode *) ( (((long)res3) & ((long)(parent))) | (~((long)res3) & ((long)foundnode)));


			long res6 = (tofindindex == parent->index);


			//(s & a) | (~s & b);
			res6 = makeSame(res6);

			res6 = res6 & isreal;

			parent->data = (res6 & value) | (~res6 & parent->data);


#ifdef ORAM_CTRL_CHANNEL
			trace_mem_rw("TreeORAM::Set2", &parent->data);
#endif


			PairNode * res4 = (PairNode *) (node == 1);
			res4 = makeSame(res4);
			res4 = (PairNode *) ( (long)res4 & (long)res3 );

			todecend = (PairNode *) ( (((long)res4) & ((long)(parent->right))) | (~((long)res4) & ((long)todecend)));


			PairNode * res5 = (PairNode *) (node != 1);
			res5 = makeSame(res5);
			res5 = (PairNode *) ( (long)res5 & (long)res3 );

			todecend = (PairNode *) ( (((long)res5) & ((long)(parent->left))) | (~((long)res5) & ((long)todecend)));

#ifdef ORAM_CTRL_CHANNEL
			trace_mem_read("TreeORAM::Set3", parent->right);
			trace_mem_read("TreeORAM::Set3", parent->left);
#endif

			/*if(tofindindex > parent->index)
			  {
			  todecend = parent->right;
			  }
			  else if(tofindindex < parent->index)
			  {
			  todecend = parent->left;
			  }
			  else if(tofindindex == parent->index)
			  {

			  foundnode = parent;

			  if(node)
			  {
			  todecend = parent->right;
			  }
			  else
			  {
			  todecend = parent->left;
			  }
			  }*/
			findset_decend(todecend,tofindindex,value, isreal);
		}


		//takes accessed nodes, removes, and then adds them back in.
		void oramclean()
		{
			//cout <<"------------------------ clean start\n";

			//cout << "clean start\n";

			//printTree();

			//cout <<"Removing: ";



			sort( toBeReordered.begin(), toBeReordered.end() );
			toBeReordered.erase( unique( toBeReordered.begin(), toBeReordered.end() ), toBeReordered.end() );



			//remove all touched nodes from tree
			for(int i=0;i<toBeReordered.size();i++)
			{
				removeNode(toBeReordered.at(i));
				//cout << " "<<toBeReordered.at(i)->index<<" ";
				//printTree();

			}
			//cout <<"\n";
			//printTree();

			tempdata.clear();
			//change memory of all nodes
			for(int i=0;i<toBeReordered.size();i++)
			{
				tempdata.push_back(getBlankNode());
			}

			std::random_shuffle ( toBeReordered.begin(), toBeReordered.end(), rand_func);


			for(int i=0;i<tempdata.size();i++)
			{
				tempdata.at(i)->data = toBeReordered.at(i)->data;
				tempdata.at(i)->index = toBeReordered.at(i)->index;

				data.push_back(toBeReordered.at(i));
			}

			toBeReordered.clear();

			//add new nodes back in
			for(int i=0;i<tempdata.size();i++)
			{
				putNode(tempdata.at(i));
				//cout <<"putting node\n";
			}

			tempdata.clear();

			//cout <<"------------------------ clean end\n";
		}

		PairNode * getBlankNode()
		{
			if(data.size() == 0)
			{
				createNewMemory();
			}

			PairNode * t = data.front();
			t->left = 0;
			t->right = 0;
			t->parent = 0;
			t->added = 0;
			data.pop_front();
			return t;
		}

		void createNewMemory()
		{
			for(int i=0;i<10;i++)
			{
				data.push_back(new PairNode());
			}
		}

		void removeNode(PairNode * n)
		{
			if(n == 0)
			{
				return;
			}

			removeNode(n,root);

		}

		void removeNode(PairNode * n, PairNode * currentcheck)
		{
			if(n == 0)
			{
				return;
			}
			if(currentcheck == 0)
			{
				return;
			}



			if(n->index > currentcheck->index)
			{
				removeNode(n,currentcheck->right);
			}
			else if(n->index < currentcheck->index)
			{
				removeNode(n,currentcheck->left);
			}
			else if(currentcheck->index == n->index)
			{
				//cout << "to remove: "<<n->index<<"\n";

				if(currentcheck->left==0)
				{
					//cout <<"RN: C1\n";

					if(currentcheck->parent!= 0)
					{
						if(currentcheck == currentcheck->parent->left)
						{
							currentcheck->parent->left = currentcheck->right;

							if(currentcheck->right != 0)
							{
								currentcheck->right->parent = currentcheck->parent;
							}
						}
						else if(currentcheck == currentcheck->parent->right)
						{
							currentcheck->parent->right = currentcheck->right;

							if(currentcheck->right != 0)
							{
								currentcheck->right->parent = currentcheck->parent;
							}
						}
					}
					else //is root
					{
						root = currentcheck->right;
						if(root != 0)
						{
							root->parent = 0;
						}
					}

					return;
				}
				else if(currentcheck->right == 0)
				{
					//cout <<"RN: C2\n";

					if(currentcheck->parent!= 0)
					{


						if(currentcheck == currentcheck->parent->left)
						{
							currentcheck->parent->left = currentcheck->left;

							if(currentcheck->left != 0)
							{
								currentcheck->left->parent = currentcheck->parent;
							}
						}
						else if(currentcheck == currentcheck->parent->right)
						{
							currentcheck->parent->right = currentcheck->left;

							if(currentcheck->left != 0)
							{
								currentcheck->left->parent = currentcheck->parent;
							}
						}
					}
					else //is root
					{
						root = currentcheck->left;
						if(root != 0)
						{
							root->parent = 0;
						}
					}

					return;
				}





				PairNode * minnewnode;

				//int leftoright=rand()%2;

				//if(leftoright)
				{
					minnewnode = minNode(currentcheck->right);
				}
				/*else
				  {
				  minnewnode = maxNode(currentcheck->left);
				  }*/



				//PairNode * maxnewnode = maxNode(currentcheck->left);

				PairNode * minnoderight = minnewnode->right;
				PairNode * minnodeleft = minnewnode->left;
				PairNode * minnodeparent = minnewnode->parent;


				PairNode * oldparent = currentcheck->parent;


				minnewnode->parent = oldparent;


				if(root == currentcheck)
				{
					root = minnewnode;
					root->parent = 0;
				}

				if(oldparent != 0)
				{
					if(oldparent->left == currentcheck)
					{
						oldparent->left = minnewnode;
					}
					else if(oldparent->right == currentcheck)
					{
						oldparent->right = minnewnode;
					}
				}
				else
				{
					root = minnewnode;
					root->parent = 0;
				}

				minnewnode->left = currentcheck->left;


				if(minnewnode->left != 0)
				{
					if(minnewnode->left != minnewnode)
					{
						minnewnode->left->parent = minnewnode;
					}
					else
					{
						minnewnode->left=0;
					}
				}



				//if(leftoright)
				{
					minnewnode->right = currentcheck->right;

					if(minnewnode->right != 0)
					{
						if(minnewnode->right != minnewnode)
						{
							minnewnode->right->parent = minnewnode;
						}
						else
						{
							minnewnode->right = minnoderight;

						}
					}

					if(minnodeparent!= 0)
					{
						if(minnodeparent->left == minnewnode)
						{
							minnodeparent->left = minnoderight;
							if(minnodeparent->left != 0)
							{
								minnodeparent->left->parent = minnodeparent;
							}
						}
					}
				}


			}
		}


		PairNode * minNode(PairNode * n)
		{

			if(n->left == 0)
			{
				return n;
			}
			else
			{
				return minNode(n->left);
			}
		}


		PairNode * maxNode(PairNode * n)
		{
			if(n->right == 0)
			{
				return n;
			}
			else
			{
				return maxNode(n->right);
			}
		}


		void balance()
		{

		}

		void putNode(PairNode * n)
		{
			if(root == 0)
			{
				root = n;
				root->parent = 0;
			}
			else
			{
				putNode_decend(n,root);
			}
		}

		void putNode_decend(PairNode * n, PairNode * parent)
		{
			if(n->index > parent->index)
			{
				if(parent->right == 0)
				{
					parent->right = n;
					n->parent = parent;
					return;
				}
				else
				{
					putNode_decend(n,parent->right);
				}
			}
			else if(n->index < parent->index)
			{
				if(parent->left == 0)
				{
					parent->left = n;
					n->parent = parent;
					return;
				}
				else
				{
					putNode_decend(n,parent->left);
				}
			}
			else
			{
				//cout << "PUT: index already in tree: "<<n->index<<"\n";
				//exit(1);
				return;
			}
		}




		string genspace(int num)
		{
			string s;
			for(int i=0;i<num;i++)
			{
				s=s+"-";
			}
			return s;
		}

		void printTree(PairNode * n, int depth)
		{
			if(n == 0)
			{
				return;
			}


			printTree(n->left,depth+1);
			//cout << genspace(depth) << n->index<<" "<<n->data/*<<" "<<n<<" "<<n->parent*/<<"\n";
			printTree(n->right,depth+1);
		}



	public:
		void printTree()
		{
			//cout << "printTree() is a TEST AND DEBUG FUNCTION ONLY, not for use in enclaves\nCycling through the tree in-order may reveal extra information\n";

			printTree(root,0);
		}







		PairNode * root;
		deque<PairNode *> data;
		deque<PairNode *> toBeReordered;
		deque<PairNode *> tempdata;

};



class ORAM
{
	public:
		vector<ORAMpre> trees;

#define NUMTREES 2

		ORAM(int size)
		{
			trees.resize(NUMTREES);


			oramsize = size;



			//std::random_shuffle ( data.begin(), data.end() );

			vector<int> indexes;
			for(int i=0;i<size;i++)
			{
				indexes.push_back(i);
			}

			std::random_shuffle ( indexes.begin(), indexes.end() );

			for(int i=0;i<size;i++)
			{
				trees[i%NUMTREES].addNode(indexes.at(i),0);
			}
		}


		long get(int index)
		{

			int num = (int)log2(oramsize);

			int realindex = rand()%num;
			//sgx_read_rand((unsigned char *)&realindex, sizeof(realindex));
			//realindex = realindex % num;

			int indexes[num];
			for(int i=0;i<num;i++)
			{
				indexes[i] = rand()%oramsize;
				//sgx_read_rand((unsigned char *)&(indexes[i]), sizeof(indexes[i]));
				//indexes[i] = indexes[i] % oramsize;

				int s = i == realindex;
				s = makeSame(s);

				//get results -> this statement selects a if s is true and b if s is false.
				indexes[i] = (s & index) | (~s & indexes[i]);
			}

			for(int i=0;i<NUMTREES;i++)
			{
				trees[i].toBeReordered.clear();
			}

			//cout <<"\n";
			long value=NOTPRESENT;
			for(int j=0;j<num;j++)
			{
				long isreal = j == realindex;

				isreal = makeSame(isreal);

				for(int i=0;i<NUMTREES;i++)
				{
					long val = trees[i].get(indexes[j]);

					//cout << val <<"\n";

					long res1 = val != NOTPRESENT;
					res1 = makeSame(res1);

					res1 = res1 & isreal;

					value = (res1 & val) | (~res1 & value);

				}
			}
			oramclean();

			return value;
		}

		void set(int index, long value)
		{

			int num = (int)log2(oramsize);

			int realindex = rand()%num;
			//sgx_read_rand((unsigned char *)&realindex, sizeof(realindex));
			//realindex = realindex % num;

			int indexes[num];
			for(int i=0;i<num;i++)
			{
				indexes[i] = rand()%oramsize;
				//sgx_read_rand((unsigned char *)&(indexes[i]), sizeof(indexes[i]));
				//indexes[i] = indexes[i] % oramsize;

				int s = i == realindex;
				s = makeSame(s);

				//get results -> this statement selects a if s is true and b if s is false.
				indexes[i] = (s & index) | (~s & indexes[i]);
			}

			for(int i=0;i<NUMTREES;i++)
			{
				trees[i].toBeReordered.clear();
			}

			for(int j=0;j<num;j++)
			{
				long isreal = j == realindex;

				isreal = makeSame(isreal);

				for(int i=0;i<NUMTREES;i++)
				{
					trees[i].set(indexes[j],value,isreal);
				}
			}
			oramclean();
		}


		vector<long> tempdatanonpointer;

		vector<PairNode *> tempdata;
		void oramclean()
		{
			vector<PairNode *> allnodes;

			//a.insert(a.end(), b.begin(), b.end());

			//startTime = RDTSC;

			allnodes.resize(trees.at(0).toBeReordered.size()+trees.at(1).toBeReordered.size());

			int currents = 0;

			for(int j=0;j<NUMTREES;j++)
			{
				for(int i=0;i<trees.at(j).toBeReordered.size();i++)
				{
					allnodes[currents++] = trees.at(j).toBeReordered[i];
				}
			}

			for(int j=0;j<NUMTREES;j++)
			{
				//allnodes.insert(allnodes.end(),trees.at(j).toBeReordered.begin(),trees.at(j).toBeReordered.end());

				//remove all touched nodes from tree
				for(int i=0;i<trees.at(j).toBeReordered.size();i++)
				{
					trees.at(j).removeNode(trees.at(j).toBeReordered.at(i));
				}
			}

			/*endTime = RDTSC;
			  cout << "a: " << (endTime - startTime)<<"\n";
			  startTime = RDTSC;*/


			/*sort( allnodes.begin(), allnodes.end() );
			  allnodes.erase( unique( allnodes.begin(), allnodes.end() ), allnodes.end() );*/


			tempdata.clear();
			tempdata.resize(allnodes.size());
			//change memory of all nodes
			for(int i=0;i<allnodes.size();i++)
			{
				tempdata[i] = (trees.at(i%2).getBlankNode());
			}


			tempdatanonpointer.resize(allnodes.size()*2);

			for(int i=0;i<allnodes.size();i++)
			{
				tempdatanonpointer.at(i*2) = allnodes.at(i)->data;
				tempdatanonpointer.at(i*2+1) = allnodes.at(i)->index;
			}

			/* daveti: update according to Dr. Mood
			   int sizexy = allnodes.size();

			   for(int i=0;i<sizexy;i+=3)
			   {
			   for(int j=0;j<5;j++)
			   {
			   int newplace = (rand()%10)+i-5;
			   int oldplace = (rand()%10)+i-5;
			//sgx_read_rand((unsigned char *)&newplace, sizeof(newplace));
			//newplace = (newplace % 10) + i - 5;
			//sgx_read_rand((unsigned char *)&oldplace, sizeof(oldplace));
			//oldplace = (oldplace % 10) + i - 5;

			while(newplace < 0)
			{
			newplace = 0;
			}
			while(oldplace < 0)
			{
			oldplace = (rand()%10)+i-5;
			//sgx_read_rand((unsigned char *)&oldplace, sizeof(oldplace));
			//oldplace = (oldplace % 10) + i - 5;
			}
			while(newplace >= sizexy)
			{
			newplace--;
			}
			while(oldplace >= sizexy)
			{
			oldplace--;
			}

			//cout << newplace*2<<" "<<tempdatanonpointer.size()<< "\n";

			long t1 = tempdatanonpointer[newplace*2];
			long t2 = tempdatanonpointer[newplace*2+1];


			tempdatanonpointer[newplace*2] = tempdatanonpointer[oldplace*2];
			tempdatanonpointer[newplace*2+1] = tempdatanonpointer[oldplace*2+1];

			tempdatanonpointer[oldplace*2] = t1;
			tempdatanonpointer[oldplace*2+1] = t2;

			}
			}

			for(int i=0;i<allnodes.size();i++)
			{
			allnodes.at(i)->data = tempdatanonpointer.at(i*2);
			allnodes.at(i)->index = tempdatanonpointer.at(i*2+1);
			}
daveti: update end */

			/*endTime = RDTSC;
			  cout << "b: " << (endTime - startTime)<<"\n";
			  startTime = RDTSC;*/

			/* daveti: update
			   std::random_shuffle ( allnodes.begin(), allnodes.end() );
			 */

			/* daveti: new code from Ben */
			unsigned int q = rand();

			vector<int> indexes;
			indexes.resize(allnodes.size());
			for(int i=0;i<allnodes.size();i++)
			{
				indexes[i]=(i+q)%allnodes.size();
			}

			std::random_shuffle ( indexes.begin(), indexes.end() );

			unsigned int sizeM = allnodes.size();

			for(int i=0;i<sizeM;i++)
			{
				for(int j=0;j<sizeM;j++)
				{
					/*if(j == indexes[i])
					  {
					  allnodes[i]->data = tempdatanonpointer.at(j*2);
					  allnodes[i]->index = tempdatanonpointer.at(j*2+1);
					  }*/

					long qres = makeSame(j == indexes[i]);
					allnodes[i]->data = (qres & tempdatanonpointer[j*2]) |  ((~qres) & allnodes[i]->data);
					allnodes[i]->index = (qres & tempdatanonpointer[j*2+1]) |  ((~qres) & allnodes[i]->index);
				}
			}
			/* daveti: new code from Ben done */


			for(int i=0;i<tempdata.size();i++)
			{
				tempdata.at(i)->data = allnodes.at(i)->data;
				tempdata.at(i)->index = allnodes.at(i)->index;

				trees.at(i%2).data.push_back(allnodes.at(i));
			}

			allnodes.clear();

			/*endTime = RDTSC;
			  cout << "c: " << (endTime - startTime)<<"\n";
			  startTime = RDTSC;*/

			//add new nodes back in
			for(int i=0;i<tempdata.size();i++)
			{
				int index = i+rand();
				//sgx_read_rand((unsigned char *)&index, sizeof(index));
				//index += i;

				if(index < 0)
					index*=-1;
				index = index % NUMTREES;

				//int index = (Math.abs(i+rand()))%NUMTREES;
				//cout << "sizes: "<<trees.size()<<"\n";
				//cout << "index: "<<index<<"\n";


				trees.at(index).putNode(tempdata.at(i));
			}

			/* endTime = RDTSC;
			   cout << "d: " << (endTime - startTime)<<"\n";
			   startTime = RDTSC;*/

			tempdata.clear();

			/*endTime = RDTSC;
			  cout << "e: " << (endTime - startTime)<<"\n";
			  startTime = RDTSC;*/


		}

		unsigned int getSize()
		{
			return oramsize;
		}

		int vsize;
		unsigned int oramsize;

		void printTrees()
		{
			for(int i=0;i<NUMTREES;i++)
			{
				trees[i].printTree();
			}
		}
};



class ORAM2
{
	public:
		unsigned int oramsize;
		ORAM2(int size)
		{
			data.resize(size);
			vsize = size;
			oramsize = size;
		}


		long get(int index)
		{
			int result;

			for(int i=0;i<vsize;i++)
			{
				long x = (i == index);
				x = makeSame(x);

#ifdef ORAM_CTRL_CHANNEL
				trace_mem_read("LinORAM::Get", &data[i]);
#endif

				result = (x & data.at(i)) | (~x & result);
				//result = data.at(i);
			}

			return result;
		}
		void set(int index, long value)
		{
			for(int i=0;i<vsize;i++)
			{
				long x = (i == index);
				x = makeSame(x);


#ifdef ORAM_CTRL_CHANNEL
				trace_mem_rw("LinORAM::Set", &data[i]);
#endif
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


};
/* End SFE ORAM DB */

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

	/* Init using the hardcode */
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

		unsigned int t = (a1 < (b1 + b));
		t = makeSame(t);
		borrow = (t & 0x1) | ((~t) & borrow);

		i++;
	}

	unsigned int res = (borrow == 0);

	res = makeSame(res);
	x = (res & 0x1) | ((~res) & x);

	res = ~res;
	x = (res & 0x0) | ((~res) & x);

	return x;
}

void sfe_entry(unsigned int alice[sfe_mil_size],
		unsigned int bob[sfe_mil_size],
		unsigned int *alice_out,
		unsigned int *bob_out)
{
	unsigned int outputalice = lessequalto(alice,bob);
	unsigned int outputbob = lessequalto(bob,alice);

	unsigned int res = (outputalice == 1) & (outputbob == 1);

	outputbob = (res & 0x0) | ((~res) & outputbob);
	outputalice = (res & 0x0) | ((~res) & outputalice);

	memcpy(alice_out, &outputalice, sizeof(unsigned int));
	memcpy(bob_out, &outputbob, sizeof(unsigned int));
}

void sfe_uc_entry(sfe_uc_input_program *prog,
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

	for(int i=0;i<sfe_uc_numinputs;i++)
	{
		destinations[i] = input[i];
	}

	for(int i=0;i<sfe_uc_numoutputs;i++)
	{
		outputs[i] = 0;
	}

	for(int i=0;i<sfe_uc_gatecount;i++)
	{
		short xad = mysfe_uc_input.x[i],
		      yad = mysfe_uc_input.y[i],
		      dad = mysfe_uc_input.d[i];

		char truth = mysfe_uc_input.tt[i];
		bool x,y;

		for(int j=0;j<sfe_uc_poolsize;j++)
		{
			/*if(xad == j)
			  {
			  x = destinations[j];
			  }*/

			unsigned int t = xad == j;
			t = makeSame(t);
			x = (t & destinations[j]) | ((~t) & x);

			t = yad == j;
			t = makeSame(t);
			y = (t & destinations[j]) | ((~t) & y);

			/*if(yad == j)
			  {
			  y = destinations[j];
			  }*/
		}


		char ttnum = y | (x <<1);

		bool res;

		res = (truth >> ttnum) & 0x1;

		for(short j=0;j<sfe_uc_poolsize;j++)
		{
			unsigned int t = dad == j;
			t = makeSame(t);
			destinations[j] = (t & res) | ((~t) & destinations[j]);

		}

	}

	for(short i=0;i<sfe_uc_numoutputs;i++)
	{
		for(short j=0;j<sfe_uc_poolsize;j++)
		{
			unsigned int t = mysfe_uc_input.output[i] == j;
			t = makeSame(t);
			outputs[i] = (t & destinations[j]) | ((~t) & outputs[i]);

		}
	}

	/* Copy out the outputs */
	memcpy(output, outputs, SMC_SGX_SFE_UC_OUTPUT_LEN);
}

/* SFE Dijk entry point */
void sfe_dj_entry(sfe_dj_input_eval *evl_input,
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
	} else {
		startnode = 0;
		endnode = sfe_dj_Nodes-1;
	}
	memcpy(edges, gen_input->edges, SMC_SGX_SFE_DJ_INPUT_EDGES_LEN);
	memcpy(connections, gen_input->connections,
			SMC_SGX_SFE_DJ_INPUT_CONNS_LEN);
	//daveti: Init done

	for(int i=0;i<sfe_dj_Nodes;i++)
	{
		NodeLinks[i] = 0x0000FFFF;
		Paths[i] = 0x0000FFFF;
		currentweight[i] = sfe_dj_MaxWeight;
		inqueue[i] = 0;

		/*if(i == startnode)
		  {
		  inqueue[i] = 1;
		  currentweight[i] =0;
		  }*/

		unsigned int t = i == startnode;
		t = makeSame(t);
		inqueue[i] = (t & 1) | ((~t) & inqueue[i]);
		currentweight[i] = (t & 0) | ((~t) & currentweight[i]);
	}

	int isDone;

	for(int i=0;i<sfe_dj_MaxIterations;i++)
	{
		int selectedWeight=sfe_dj_MaxWeight;
		int selectedNode=0xFFFFFF;
		int cconnection;

		for(int j=0;j<sfe_dj_Nodes;j++)
		{
			/*if(inqueue[j] && selectedWeight > currentweight[j])
			  {
			  selectedNode = j;
			  selectedWeight = currentweight[j];
			  }*/

			unsigned int t = (inqueue[j] & (selectedWeight > currentweight[j]));
			t = makeSame(t);
			selectedNode = (t & j) | ((~t) & selectedNode);
			selectedWeight = (t & currentweight[j]) | ((~t) & selectedWeight);

		}

		/*if(selectedNode == endnode)
		  {
		  isDone = 1;
		  }*/

		unsigned int t = (selectedNode == endnode);
		t = makeSame(t);
		isDone = (t & 1) | ((~t) & isDone);


		for (int j =0; j < sfe_dj_Nodes; j++)
		{
			/*if ( selectedNode == j)
			  {
			  inqueue[j] = 0;
			  }*/

			t = (selectedNode == j);
			t = makeSame(t);
			inqueue[j] = (t & 0) | ((~t) & inqueue[j]);
		}

		selectedconnections[0] = selectedconnections[1] = selectedconnections[2] = selectedconnections[3] = 0xFFFFFF;


		for(int j=0;j< sfe_dj_Nodes;j++)
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

			selectedconnections[0] = (t & connections[j*sfe_dj_Edges_per_node]) | ((~t) & selectedconnections[0]);
			selectedconnections[1] = (t & connections[j*sfe_dj_Edges_per_node+1]) | ((~t) & selectedconnections[1]);
			selectedconnections[2] = (t & connections[j*sfe_dj_Edges_per_node+2]) | ((~t) & selectedconnections[2]);
			selectedconnections[3] = (t & connections[j*sfe_dj_Edges_per_node+3]) | ((~t) & selectedconnections[3]);

			selectededges[0] = (t & edges[j*sfe_dj_Edges_per_node]) |   ((~t) & selectededges[0]);
			selectededges[1] = (t & edges[j*sfe_dj_Edges_per_node+1]) | ((~t) & selectededges[1]);
			selectededges[2] = (t & edges[j*sfe_dj_Edges_per_node+2]) | ((~t) & selectededges[2]);
			selectededges[3] = (t & edges[j*sfe_dj_Edges_per_node+3]) | ((~t) & selectededges[3]);

		}

		/*cout << "selectednode: "<<selectedNode<<"\n";
		  for(int i=0;i<4;i++)
		  {
		  cout << selectedconnections[i] <<" "<<selectededges[0]<<"\n";
		  }*/



		int newweight = 0;
		int oldweight;
		int oldlink;

		for(int j=0;j<sfe_dj_Edges_per_node;j++)
		{
			int cweight = selectededges[j];
			cconnection = selectedconnections[j];
			newweight = selectedWeight + cweight;

			//cout << "cw: "<<cweight<<" "<<selectedWeight<<"\n";

			for(int k=0;k<sfe_dj_Nodes;k++)
			{
				/*if(k == cconnection)
				  {
				  oldweight = currentweight[k];
				  oldlink = NodeLinks[k];
				  }*/

				t = (k == cconnection);
				t = makeSame(t);
				oldweight = (t & currentweight[k]) | ((~t) & oldweight);
				oldlink = (t & NodeLinks[k]) | ((~t) & oldlink);
			}

			int change = 1;
			int newlink = selectedNode;

			//cout << "oldwieght: "<<oldweight <<" "<<newweight<<"\n";

			/*if(oldweight < newweight)
			  {
			  newlink = oldlink;
			  newweight = oldweight;
			  change = 0;
			  }*/

			t = (oldweight < newweight);
			t = makeSame(t);
			newlink = (t & oldlink) | ((~t) & newlink);
			newweight = (t & oldweight) | ((~t) & newweight);
			change = (t & 0) | ((~t) & change);


			for(int k=0;k<sfe_dj_Nodes;k++)
			{
				/*if(k == cconnection)
				  {
				  currentweight[k] = newweight;
				  NodeLinks[k] = newlink;
				  }*/

				t = (k == cconnection);
				t = makeSame(t);
				currentweight[k] = (t & newweight) | ((~t) & currentweight[k]);
				NodeLinks[k] = (t & newlink) | ((~t) & NodeLinks[k]);

			}

			//cout << "new conc: "<<cconnection<<"\n";

			for(int k=0;k<sfe_dj_Nodes;k++)
			{
				/*if(k == cconnection)
				  {
				  if(change == 1)
				  {
				  inqueue[k] = 1;
				//cout << "quing: "<<k<<"\n";
				}
				}*/

				t = ((k == cconnection) & (change == 1));
				t = makeSame(t);
				inqueue[k] = (t & 1) | ((~t) & inqueue[k]);
			}
		}
	}


	int nodeToGet = endnode;
	for(int i=0; i<sfe_dj_Nodes; i++)
	{
		int goit =0;
		for(int j=0;j<sfe_dj_Nodes;j++)
		{
			/*if(goit == 0)
			  {
			  if(j == nodeToGet)
			  {
			  nodeToGet = NodeLinks[j];
			  Paths[i] = j;
			  goit = 1;
			  }
			  }*/

			unsigned int t = ((goit == 0) & (j == nodeToGet));
			t = makeSame(t);
			nodeToGet = (t & NodeLinks[j]) | ((~t) & nodeToGet);
			Paths[i] = (t & j) | ((~t) & Paths[i]);
			goit = (t & 1) | ((~t) & goit);
		}
	}

	//should be output
	/*
	   for(int i=0;i<sfe_dj_Nodes;i++)
	   {
	   cout << Paths[i] <<" ";
	   }
	   cout << endl;
	 */
	memcpy(output, Paths, SMC_SGX_SFE_DJ_OUTPUT_LEN);
}

/* SFE ORAM database program */
void sfe_or_entry(unsigned long evl_input[sfe_or_sizeOfDatabase],
		unsigned int gen_input[sfe_or_queries],
		unsigned long output[sfe_or_queries],
		int oram)
{
	// evl_input - bob input - DB
	// gen_input - alice input - queries
	// output - output for alice
	int i;

	if (oram == 0) {
		/* Run the non-oram version of the DB */
		for (i = 0; i < sfe_or_queries; i++) {
#ifdef ORAM_CTRL_CHANNEL
			eprintf("GenInput %u", gen_input[i]);
			trace_mem_read("DB::GenInput", &gen_input[i]);
#endif
			output[i] = evl_input[gen_input[i]];
#ifdef ORAM_CTRL_CHANNEL
			eprintf("EvlInput %u", evl_input[i]);
			trace_mem_read("DB::EvlInput", &evl_input[gen_input[i]]);
			trace_mem_write("DB::Output", &output[i]);
			eprintf("Output %lu", output[i]);
#endif
		}
	}
	else if (oram == 1) {
		/* Run the tree ORAM */
		ORAM o(sfe_or_sizeOfDatabase);

		for (i = 0; i < sfe_or_sizeOfDatabase; i++) {
#ifdef ORAM_CTRL_CHANNEL
			eprintf("EvlInput %u", evl_input[i]);
			trace_mem_read("DB::EvlInput", &evl_input[i]);
#endif
			o.set(i, evl_input[i]);
		}

		for (i = 0; i < sfe_or_queries; i++) {
#ifdef ORAM_CTRL_CHANNEL
			eprintf("GenInput %u", gen_input[i]);
			trace_mem_read("DB::GenInput", &gen_input[i]);
#endif
			output[i] = o.get(gen_input[i]);
#ifdef ORAM_CTRL_CHANNEL
			trace_mem_write("DB::Output", &output[i]);
			eprintf("Output %lu", output[i]);
#endif
		}
	}
	else if (oram == 2) {
		/* Run the linear ORAM */
		ORAM2 o(sfe_or_sizeOfDatabase);

		for (i = 0; i < sfe_or_sizeOfDatabase; i++) {
#ifdef ORAM_CTRL_CHANNEL
			eprintf("EvlInput %u", evl_input[i]);
			trace_mem_read("DB::EvlInput", &evl_input[i]);
#endif
			o.set(i, evl_input[i]);
		}

		for (i = 0; i < sfe_or_queries; i++) {
#ifdef ORAM_CTRL_CHANNEL
			eprintf("GenInput %u", gen_input[i]);
			trace_mem_read("DB::GenInput", &gen_input[i]);
#endif
			output[i] = o.get(gen_input[i]);	
#ifdef ORAM_CTRL_CHANNEL
			trace_mem_write("DB::Output", &output[i]);
			eprintf("Output %lu", output[i]);
#endif
		}
	}
	else {
		/* We should not reach here */
		for (i = 0; i < sfe_or_queries; i++)
			output[i] = 777;
	}
}

#ifdef ORAM_TESTING

bool oram_test1();
bool oram_test2();
bool oram_test3();

#define EXPECT(t) if(!(t)) return false;
#define RUN_TEST(no) do { \
	if(!oram_test##no()) { \
		eprintf("Test %d: FAILED", no); \
		status = status && false; \
	} else { \
		eprintf("Test %d: PASSED", no); \
	} \
} while(0)

bool oram_run_tests()
{
	bool status = true;

	eprintf("Running ORAM tests");

	RUN_TEST(1);
	RUN_TEST(2);
	RUN_TEST(3);

	return status;
}

bool oram_test1()
{
	int testSize = 4;
	int i = 0;
	ORAM o(testSize);

	for (i = 0; i < testSize; i++)
		o.set(i, i);

	for (i = 0; i < testSize*10; i++) {
		//eprintf("O[0] %lu", o.get(0));
		//eprintf("O[1] %lu", o.get(1));
		//eprintf("O[2] %lu", o.get(2));
		//eprintf("O[3] %lu", o.get(3));
		EXPECT(o.get(0) == 0)
			EXPECT(o.get(1) == 1)
			EXPECT(o.get(2) == 2)
			EXPECT(o.get(3) == 3)
	}

	return true;
}

bool oram_test2()
{
	int testSize = 10;
	int i = 0;
	ORAM o(testSize);

	for (i = 0; i < testSize; i++)
		o.set(i, i);

	for (i = 0; i < testSize; i++)
		EXPECT(o.get(i) == i)

			return true;
}

bool oram_test3()
{
	EXPECT(sizeof(int) == 4);
	EXPECT(sizeof(long int) == 8);
	EXPECT(sizeof(long long int) == 8);
	EXPECT(sizeof(long) == 8);

	return true;
}

#endif
