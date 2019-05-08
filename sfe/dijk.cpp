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
    
    for(int i=0;i<MaxIterations;i++)
    {
        int selectedWeight=MaxWeight;
        int selectedNode=0xFFFFFF;
        int cconnection;
        
        for(int j=0;j<Nodes;j++)
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
        
        
        for (int j =0; j < Nodes; j++)
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
        
        
        for(int j=0;j< Nodes;j++)
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
            
            selectedconnections[0] = (t & connections[j*Edges_per_node]) | ((~t) & selectedconnections[0]);
            selectedconnections[1] = (t & connections[j*Edges_per_node+1]) | ((~t) & selectedconnections[1]);
            selectedconnections[2] = (t & connections[j*Edges_per_node+2]) | ((~t) & selectedconnections[2]);
            selectedconnections[3] = (t & connections[j*Edges_per_node+3]) | ((~t) & selectedconnections[3]);
            
            selectededges[0] = (t & edges[j*Edges_per_node]) |   ((~t) & selectededges[0]);
            selectededges[1] = (t & edges[j*Edges_per_node+1]) | ((~t) & selectededges[1]);
            selectededges[2] = (t & edges[j*Edges_per_node+2]) | ((~t) & selectededges[2]);
            selectededges[3] = (t & edges[j*Edges_per_node+3]) | ((~t) & selectededges[3]);
            
        }
        
        /*cout << "selectednode: "<<selectedNode<<"\n";
        for(int i=0;i<4;i++)
        {
            cout << selectedconnections[i] <<" "<<selectededges[0]<<"\n";
        }*/
        
        
        
        int newweight = 0;
        int oldweight;
        int oldlink;
        
        for(int j=0;j<Edges_per_node;j++)
        {
            int cweight = selectededges[j];
            cconnection = selectedconnections[j];
            newweight = selectedWeight + cweight;
            
            //cout << "cw: "<<cweight<<" "<<selectedWeight<<"\n";
            
            for(int k=0;k<Nodes;k++)
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
            
            
            for(int k=0;k<Nodes;k++)
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
            
            for(int k=0;k<Nodes;k++)
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
    for(int i=0; i<Nodes; i++)
    {
        int goit =0;
        for(int j=0;j<Nodes;j++)
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
    for(int i=0;i<Nodes;i++)
    {
        cout << Paths[i] <<" ";
    }
    cout << endl;
}


