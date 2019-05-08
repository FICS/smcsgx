
#include <iostream> //how to use external files/packages
#include <string>
#include <cmath>
#include <vector>
#include <deque>
#include <algorithm>
#include <stdlib.h>

using namespace std;

#define RDTSC ({unsigned long long res;  unsigned hi, lo;   __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi)); res =  ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );res;})
unsigned long startTime, endTime;

long makeSame(long value)
{
    value = value | (value << 32);
    value = value | (value << 16);
    value = value | (value << 8);
    value = value | (value << 4);
    value = value | (value << 2);
    value = value | (value << 1);
    return value;
}


//takes 1 and 0 and returns a mask with all 1s or all 0s depending upon the entered value: NOTE: no if statements in this code
int makeSame(int value)
{
    value = value | (value << 16);
    value = value | (value << 8);
    value = value | (value << 4);
    value = value | (value << 2);
    value = value | (value << 1);
    
    
    return value;
}






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
            cout << "ERROR1: ORAM ADDING AN INDEX ALREADY CONTAINED\n";
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
        
        
        //(s & a) | (~s & b);
        res6 = makeSame(res6);
        toget = (res6 & parent->data) | (~res6 & toget);
        
        
        
        
        PairNode * res4 = (PairNode *) (node == 1);
        res4 = makeSame(res4);
        res4 = (PairNode *) ( (long)res4 & (long)res3 );
        
        todecend = (PairNode *) ( (((long)res4) & ((long)(parent->right))) | (~((long)res4) & ((long)todecend)));
        
        
        PairNode * res5 = (PairNode *) (node != 1);
        res5 = makeSame(res5);
        res5 = (PairNode *) ( (long)res5 & (long)res3 );
        
        todecend = (PairNode *) ( (((long)res5) & ((long)(parent->left))) | (~((long)res5) & ((long)todecend)));
        

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
        
        
        //(s & a) | (~s & b);
        res6 = makeSame(res6);
        
        res6 = res6 & isreal;
        
        parent->data = (res6 & value) | (~res6 & parent->data);
        
        
        
        
        PairNode * res4 = (PairNode *) (node == 1);
        res4 = makeSame(res4);
        res4 = (PairNode *) ( (long)res4 & (long)res3 );
        
        todecend = (PairNode *) ( (((long)res4) & ((long)(parent->right))) | (~((long)res4) & ((long)todecend)));
        
        
        PairNode * res5 = (PairNode *) (node != 1);
        res5 = makeSame(res5);
        res5 = (PairNode *) ( (long)res5 & (long)res3 );
        
        todecend = (PairNode *) ( (((long)res5) & ((long)(parent->left))) | (~((long)res5) & ((long)todecend)));
        
  
        
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
        
        std::random_shuffle ( toBeReordered.begin(), toBeReordered.end() );

        
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
            cout << "PUT: index already in tree: "<<n->index<<"\n";
            exit(1);
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
        cout << genspace(depth) << n->index<<" "<<n->data/*<<" "<<n<<" "<<n->parent*/<<"\n";
        printTree(n->right,depth+1);
    }
    

    
public:
    void printTree()
    {
        cout << "printTree() is a TEST AND DEBUG FUNCTION ONLY, not for use in enclaves\nCycling through the tree in-order may reveal extra information\n";
        
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
        int indexes[num];
        for(int i=0;i<num;i++)
        {
            indexes[i] = rand()%oramsize;
            
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
        int indexes[num];
        for(int i=0;i<num;i++)
        {
            indexes[i] = rand()%oramsize;

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
        
        int sizexy = allnodes.size();
        
        for(int i=0;i<sizexy;i+=3)
        {
            for(int j=0;j<5;j++)
            {
                int newplace = (rand()%10)+i-5;
                int oldplace = (rand()%10)+i-5;
                
                while(newplace < 0)
                {
                    newplace = 0;
                }
                while(oldplace < 0)
                {
                    oldplace = (rand()%10)+i-5;
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
        
        /*endTime = RDTSC;
        cout << "b: " << (endTime - startTime)<<"\n";
        startTime = RDTSC;*/
        
        std::random_shuffle ( allnodes.begin(), allnodes.end() );
        

        
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


#include <math.h>

#define queries 900
#define sizeOfDatabase 900


int main()
{
    
    
    
    
    
    
   ///if statements should be like this:
    
    int a = 10;
    int b = 5;
    
    
    
    //if(5 == 6)
    //{
    //result = a;
    //}
    //else
    //{
    //result = b;
    //}
    
    //if
    int temp = a;
    //else
    int temp2 = b;

    //condition
    int s = 5 != 6;
    s = makeSame(s);
    
    //get results -> this statement selects a if s is true and b if s is false.
    int result = (s & a) | (~s & b);
    
    
    //cout << "result: "<<result <<"\n";
    
    
    
    
    vector<unsigned int> alicein; //queries indexes
    vector<unsigned long> bobin; //database
    
    vector<unsigned long> output; //output
    
    
    
    ORAM o(sizeOfDatabase);
    bobin.resize(sizeOfDatabase);
    
    
    alicein.resize(queries);
    output.resize(queries);
    

    //daveti: fake inputs
    for (int i = 0; i < queries; i++)
	alicein[i] = i;
    for (int i = 0; i < sizeOfDatabase; i++)
	bobin[i] = i;    
    
    for(int i=0;i<sizeOfDatabase;i++)
    {
        o.set(i,bobin.at(i));
    }
    
    
    for(int i=0;i<queries;i++)
    {
        //o.set(i%o.oramsize,i);
        
        output.at(i) = o.get(alicein.at(i));

    }

    //daveti: dump output
    for (int i = 0; i < queries; i++)
	cout << output[i] << " ";
    cout << endl;
    

}






