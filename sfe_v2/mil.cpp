
#include <iostream>
#include <vector>

using namespace std;

#define sizex 32

int makeSame(int value)
{
    value = value | (value << 16);
    value = value | (value << 8);
    value = value | (value << 4);
    value = value | (value << 2);
    value = value | (value << 1);
    
    
    return value;
}


int lessequalto(vector<unsigned int> alice, vector<unsigned int> bob)
{
    
    
    unsigned int x=0;
    unsigned int i;
    unsigned int borrow=0;
    
    for(i = 0; i < alice.size();)
    {
        unsigned int a1 = alice.at(i);
        unsigned int b1 = bob.at(i);
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


int main()
{
    
    //input arrays!!!!!
    vector<unsigned int> alice;
    vector<unsigned int> bob;
    
    alice.resize(sizex);
    bob.resize(sizex);
    
    for(int i=0;i<sizex;i++)
    {
        alice.at(i) = bob.at(i) = 0;
    }
    
    alice.at(0) = 4;
    alice.at(1) = 0;
    bob.at(0) = 5;
    bob.at(1) = 1;
    
    
    
    
    
    
    unsigned int outputalice = lessequalto(alice,bob);
    unsigned int outputbob = lessequalto(bob,alice);
    
    unsigned int res = (outputalice == 1) & (outputbob == 1);
    
    outputbob = (res & 0x0) | ((~res) & outputbob);
    outputalice = (res & 0x0) | ((~res) & outputalice);
    
    
    
    
    //then output!
    std::cout << outputalice << " "<< outputbob<<"\n";
}