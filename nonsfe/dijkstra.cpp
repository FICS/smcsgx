#include <iostream>
//#include <oram.h>

using namespace std;

#define Nodes 6
#define Edges_per_node 4
#define MaxIterations Nodes

#define MaxWeight 0x7FFFFFF


int makeSame(int value)
{
    value = value | (value << 16);
    value = value | (value << 8);
    value = value | (value << 4);
    value = value | (value << 2);
    value = value | (value << 1);
    
    
    return value;
}

int main()
{
    
    //input alice;
    int edges[Nodes*Edges_per_node];
    int connections[Nodes*Edges_per_node];
    
    //input bob;
    int startnode;
    int endnode;
    
    
    for(int i=0;i<Nodes*Edges_per_node;i++)
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
    
    
    
    
    
    startnode = 5;
    endnode = 0;
    
    
    
    //other
    int currentweight[Nodes];
    int inqueue[Nodes];
    
    
    int selectedconnections[Edges_per_node];
    int selectededges[Edges_per_node];
    int NodeLinks[Nodes];
    int Paths[Nodes];
    
    for(int i=0;i<Nodes;i++)
    {
        NodeLinks[i] = 0x0000FFFF;
        Paths[i] = 0x0000FFFF;
        currentweight[i] = MaxWeight;
        inqueue[i] = 0;

    }
    
    inqueue[startnode] = 1;
    currentweight[startnode] = 1;
    
    int isDone=0;
    
    for(int i=0;i<MaxIterations && (!isDone);i++)
    {
        int selectedWeight=MaxWeight;
        int selectedNode=0xFFFFFF;
        int cconnection;
        
        for(int j=0;j<Nodes;j++)
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

        inqueue[selectedNode] = 0;
        
        
        selectedconnections[0] = selectedconnections[1] = selectedconnections[2] = selectedconnections[3] = 0xFFFFFF;
        
        
        //for(int j=0;j< Nodes;j++)
        {
            //if(j == selectedNode)
            {
                
                
                selectedconnections[0] = connections[selectedNode*Edges_per_node];
                selectedconnections[1] = connections[selectedNode*Edges_per_node+1];
                selectedconnections[2] = connections[selectedNode*Edges_per_node+2];
                selectedconnections[3] = connections[selectedNode*Edges_per_node+3];
                
                selectededges[0] = edges[selectedNode*Edges_per_node];
                selectededges[1] = edges[selectedNode*Edges_per_node+1];
                selectededges[2] = edges[selectedNode*Edges_per_node+2];
                selectededges[3] = edges[selectedNode*Edges_per_node+3];
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
        
        for(int j=0;j<Edges_per_node ;j++)
        {

            int cweight = selectededges[j];
            cconnection = selectedconnections[j];
            newweight = selectedWeight + cweight;
            
            if(cconnection >= Nodes)
            {
                continue;
            }
            
            
            //cout << "cw: "<<cweight<<" "<<selectedWeight<<"\n";
            
            if(cconnection < Nodes)
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
            
            if(cconnection < Nodes)
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
    for(int i=0; i<Nodes; i++)
    {
        if(nodeToGet < Nodes)
        {
            Paths[i] = nodeToGet;
            nodeToGet = NodeLinks[nodeToGet];
        }
    }
    
    //should be output
    for(int i=0;i<Nodes;i++)
    {
        cout << Paths[i] <<" ";
    }
    cout << endl;
}















