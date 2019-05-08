
#include <iostream>


#define size 32

int makeSame(int value)
{
    value = value | (value << 16);
    value = value | (value << 8);
    value = value | (value << 4);
    value = value | (value << 2);
    value = value | (value << 1);
    
    
    return value;
}


int lessequalto(unsigned int alice[size], unsigned int bob[size])
{
    
    
    unsigned int x=0;
    unsigned int i;
    unsigned int borrow=0;
    
    for(i = 0; i < size;)
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


int main()
{
    
    //input arrays!!!!!
    unsigned int alice[size];
    unsigned int bob[size];
    
    for(int i=0;i<size;i++)
    {
        alice[i] = bob[i] = 0;
    }
    
    alice[0] = 4;
    alice[1] = 1;
    bob[0] = 5;
    bob[1] = 1;
    
    
    
    
    
    
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
    
    
    
    
    //then output!
    std::cout << outputalice << " "<< outputbob<<"\n";
}