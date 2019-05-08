#include <iostream>
using namespace std;

#define gatecount 10000
#define numinputs 250
#define numoutputs 1000
#define poolsize  350

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

class Program
{
public:
    vector<short> x;
    vector<short> y;
    vector<char> tt;
    vector<short> d;

    vector<short> output;
    
    void blank()
    {
        x.resize(gatecount);
        y.resize(gatecount);
        tt.resize(gatecount);
        d.resize(gatecount);
        
        output.resize(numoutputs);
        
        for(int i=0;i<gatecount;i++)
        {
            x.at(i) = y.at(i) = tt.at(i) = d.at(i)=0;
        }
        for(int i=0;i<numoutputs;i++)
        {
            output.at(i) = 0;
        }
    }
};


int  main()
{
    Program input1;
    input1.blank();
    
    
    
    /* this is a tempory test programy */
    /*for(int i=0;i<gatecount;i++)
    {
        input1.x[i] = i;
        input1.y[i] = i;
        input1.tt[i] = 5;
        input1.d[i] = i;
        input1.output[i] = i;
    }*/
    
    
    
    
    
    vector<bool> input2; input2.resize(numinputs);
    
    /* temp inputs */
    /*input2[0] = 0;
    input2[1] = 1;
    input2[2] = 1;
    input2[3] = 0;*/
    
    
    vector<bool> destinations; destinations.resize(poolsize);
    vector<bool> outputs; outputs.resize(numoutputs);

    for(int i=0;i<numinputs;i++)
    {
        destinations.at(i) = input2.at(i);
    }

    for(int i=0;i<gatecount;i++)
    {
        short xad = input1.x.at(i), yad = input1.y.at(i), dad = input1.d.at(i);

        char truth = input1.tt.at(i);

        
            bool x,y;

            for(int j=0;j<poolsize;j++)
            {
                /*if(xad == j)
                {
                    x = destinations[j];
                }*/
                
                unsigned int t = xad == j;
                t = makeSame(t);
                x = (t & destinations.at(j)) | ((~t) & x);
                
                t = yad == j;
                t = makeSame(t);
                y = (t & destinations.at(j)) | ((~t) & y);
                
                /*if(yad == j)
                {
                    y = destinations[j];
                }*/
            }


            char ttnum = y | (x <<1);

            bool res;
        
            res = (truth >> ttnum) & 0x1;

            for(short j=0;j<poolsize;j++)
            {
                unsigned int t = dad == j;
                t = makeSame(t);
                destinations.at(j) = (t & res) | ((~t) & destinations.at(j));
                
            }
        
    }

    for(short i=0;i<numoutputs;i++)
    {
        for(short j=0;j<poolsize;j++)
        {
            unsigned int t = input1.output.at(i) == j;
            t = makeSame(t);
            outputs.at(i) = (t & destinations.at(j)) | ((~t) & outputs.at(i));
            
        }
    }


    cout << "Outputs:\n";
    for(int i=0;i<numoutputs;i++)
    {
        cout << outputs.at(i)<<"\n";
    }
}






