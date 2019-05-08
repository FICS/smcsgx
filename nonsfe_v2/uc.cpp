#include <iostream>
using namespace std;

#define gatecount 4
#define numinputs 4
#define numoutputs 4
#define poolsize  8


int makeSame(int value)
{
    value = value | (value << 16);
    value = value | (value << 8);
    value = value | (value << 4);
    value = value | (value << 2);
    value = value | (value << 1);
    
    
    return value;
}

class Program
{
public:
    short x[gatecount];
    short y[gatecount];
    char tt[gatecount];
    short d[gatecount];

    short output[numoutputs];
    
    void blank()
    {
        for(int i=0;i<gatecount;i++)
        {
            x[i] = y[i] = tt[i] = d[i]=0;
        }
        for(int i=0;i<numoutputs;i++)
        {
            output[i] = 0;
        }
    }
};


int  main()
{
    Program input1;
    input1.blank();
    
    
    
    /* this is a tempory test programy */
    for(int i=0;i<gatecount;i++)
    {
        input1.x[i] = i;
        input1.y[i] = i;
        input1.tt[i] = 5;
        input1.d[i] = i;
        input1.output[i] = i;
    }
    
    
    
    
    
    bool input2[numinputs];
    
    /* temp inputs */
    input2[0] = 0;
    input2[1] = 1;
    input2[2] = 1;
    input2[3] = 0;
    
    
    bool destinations[poolsize];
    bool outputs[numoutputs];

    for(int i=0;i<numinputs;i++)
    {
        destinations[i] = input2[i];
    }

    for(int i=0;i<gatecount;i++)
    {
        short xad = input1.x[i], yad = input1.y[i], dad = input1.d[i];

        char truth = input1.tt[i];

        
            bool x,y;

            /*for(int j=0;j<poolsize;j++)
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

            /*for(short j=0;j<poolsize;j++)
            {
                unsigned int t = dad == j;
                t = makeSame(t);
                destinations[j] = (t & res) | ((~t) & destinations[j]);
                

            }*/
        
            destinations[dad] = res;
        
    }

    for(short i=0;i<numoutputs;i++)
    {
        /*for(short j=0;j<poolsize;j++)
        {
            unsigned int t = input1.output[i] == j;
            t = makeSame(t);
            outputs[i] = (t & destinations[j]) | ((~t) & outputs[i]);
            
            if(input1.output[i] == j)
            {
                outputs[i] = destinations[j];
            }
        }*/
        
        outputs[i] = destinations[input1.output[i]];
    }


    cout << "Outputs:\n";
    for(int i=0;i<numoutputs;i++)
    {
        cout << outputs[i]<<"\n";
    }
}






