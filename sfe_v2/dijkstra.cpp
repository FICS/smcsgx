#include <iostream>
//#include <oram.h>

using namespace std;

#define Nodes 6
#define Edges_per_node 4
#define MaxIterations Nodes

#define MaxWeight 0x7FFFFFF

using namespace std;
#include <vector>

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
    vector<int> edges; edges.resize(Nodes*Edges_per_node);
    vector<int> connections; connections.resize(Nodes*Edges_per_node);
    
    //input bob;
    int startnode;
    int endnode;
    
    
    for(int i=0;i<Nodes*Edges_per_node;i++)
    {
        edges.at(i) = 0;
        connections.at(i) = 0xFFFF;
    }
    
    
    edges.at(0) = 10;
    connections.at(0) = 1;
    
    edges.at(1) = 1;
    connections.at(1) = 2;
    
    
    
    edges.at(4) = 10;
    connections.at(4) = 0;
    
    edges.at(5) = 1;
    connections.at(5) = 2;
    
    edges.at(6) = 1;
    connections.at(6) = 3;
    
    

    edges.at(8) = 1;
    connections.at(8) = 0;
    
    edges.at(9) = 1;
    connections.at(9) = 1;
    
    edges.at(10) = 1;
    connections.at(10) = 4;
    
    
    
    edges.at(12) = 1;
    connections.at(12) = 1;
    
    edges.at(13) = 1;
    connections.at(13) = 4;
    
    edges.at(14) = 1;
    connections.at(14) = 5;
    
    
    edges.at(16) = 1;
    connections.at(16) = 2;
    
    edges.at(17) = 1;
    connections.at(17) = 3;
    
    edges.at(18) = 10;
    connections.at(18) = 5;
    
    
    
    edges.at(20) = 1;
    connections.at(20) = 3;
    
    edges.at(21) = 10;
    connections.at(21) = 4;
    
    
    
    
    
    startnode = 5;
    endnode = 0;
    
    
    
    //other
    vector<int> currentweight; currentweight.resize(Nodes);
    vector<int> inqueue; inqueue.resize(Nodes);
    
    
    vector<int> selectedconnections; selectedconnections.resize(Edges_per_node);
    vector<int> selectededges; selectededges.resize(Edges_per_node);
    vector<int> NodeLinks; NodeLinks.resize(Nodes);
    vector<int> Paths; Paths.resize(Nodes);
    
    for(int i=0;i<Nodes;i++)
    {
        NodeLinks.at(i) = 0x0000FFFF;
        Paths.at(i) = 0x0000FFFF;
        currentweight.at(i) = MaxWeight;
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
        
        
        for (int j =0; j < Nodes; j++)
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
            
            selectedconnections.at(0) = (t & connections.at(j*Edges_per_node)) | ((~t) & selectedconnections.at(0));
            selectedconnections.at(1) = (t & connections.at(j*Edges_per_node+1)) | ((~t) & selectedconnections.at(1));
            selectedconnections.at(2) = (t & connections.at(j*Edges_per_node+2)) | ((~t) & selectedconnections.at(2));
            selectedconnections.at(3) = (t & connections.at(j*Edges_per_node+3)) | ((~t) & selectedconnections.at(3));
            
            selectededges.at(0) = (t & edges.at(j*Edges_per_node)) |   ((~t) & selectededges.at(0));
            selectededges.at(1) = (t & edges.at(j*Edges_per_node+1)) | ((~t) & selectededges.at(1));
            selectededges.at(2) = (t & edges.at(j*Edges_per_node+2)) | ((~t) & selectededges.at(2));
            selectededges.at(3) = (t & edges.at(j*Edges_per_node+3)) | ((~t) & selectededges.at(3));
            
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
            int cweight = selectededges.at(j);
            cconnection = selectedconnections.at(j);
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
                oldweight = (t & currentweight.at(k)) | ((~t) & oldweight);
                oldlink = (t & NodeLinks.at(k)) | ((~t) & oldlink);
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
                currentweight.at(k) = (t & newweight) | ((~t) & currentweight.at(k));
                NodeLinks.at(k) = (t & newlink) | ((~t) & NodeLinks.at(k));
                
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
                inqueue.at(k) = (t & 1) | ((~t) & inqueue.at(k));
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
            nodeToGet = (t & NodeLinks.at(j)) | ((~t) & nodeToGet);
            Paths.at(i) = (t & j) | ((~t) & Paths.at(i));
            goit = (t & 1) | ((~t) & goit);
        }
    }
    
    
    for(int i=0;i<Nodes;i++)
    {
        cout << Paths.at(i) <<" ";
    }
    cout << endl;
}















