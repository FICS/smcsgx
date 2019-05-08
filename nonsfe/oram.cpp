
#include <iostream> //how to use external files/packages
#include <string>
#include <cmath>
#include <vector>
#include <deque>

using namespace std; 



#include <math.h>


#define queries 900
#define sizeOfDatabase 900

int main()
{
    
    

    
    
    
    vector<unsigned int> alicein; //queries indexes
    vector<unsigned long> bobin; //database
    
    vector<unsigned long> output; //output
    
    
    
    long o[sizeOfDatabase];
    bobin.resize(sizeOfDatabase);
    
    
    alicein.resize(queries);
    output.resize(queries);
    
    
    
    for(int i=0;i<sizeOfDatabase;i++)
    {
        o[i] = bobin[i];
    }
    
    
    for(int i=0;i<queries;i++)
    {
        //o.set(i%o.oramsize,i);
        
        output[i] = alicein[i];
        
    }
}






