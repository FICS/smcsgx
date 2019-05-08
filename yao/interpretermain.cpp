//#include "exprtest.hh"
//#include "traverse.h"
#include <iostream>
#include <cstdlib>
//#include "ast.h"
//#include "defines.h"
//#include "includes.h"
//#include "typegenerate.h"
//#include "circuitoutput.h"
#include "interpreter.h"

using namespace std;


/*
 arguments:
 -i - run interpreter
 -i_io see interpreter io
 -i_fg see interpreter all gates
 -i_nostat no stats on the interpreter
 -i_header print header info
 -pgc - print gate counts
 -no_ctimer - print compile time
 -sco - see output
 -nowarn
 -notypes - do not print output type file
 */


#include <sys/time.h>


bool printCompileTime=true;
bool printGates = false;
bool runInterpreter = false;
bool printWarnings = true;
bool printInterpreterIO = false;
bool printInterpreterGates = false;
bool printInterpreterStats = true;
bool printInterpreterHeader = false;
bool useInterpreterValidationOption = false;
bool printFullGateList = false;
string gatelistfilename;

void processArgs(int argc, char *argv[])
{
    for(int i=2;i<argc;i++)
    {
        string s = argv[i];
        
        /*if(s == "-nowarn")
        {
            printWarnings = false;
        }
        else if(s == "-i")
        {
            runInterpreter = true;
        }*/
        if(s == "-i_io")
        {
            printInterpreterIO = true;
        }
        else if(s == "-i_fg")
        {
            printInterpreterGates = true;
        }
        else if(s == "-i_nostats")
        {
            printInterpreterStats = false;
        }
        else if(s == "-i_header")
        {
            printInterpreterHeader = true;
        }
        else if(s == "-i_validation")
        {
            useInterpreterValidationOption = true;
        }
        else if(s == "-i_output")
        {
            i++;
            
            if(i >= argc)
            {
                cerr << "i_output must have another argument\nExiting...\n";
                exit(1);
            }
            
            printFullGateList = true;
            
            gatelistfilename = argv[i];
        }
        /*else if(s == "-pgc")
        {
            printGates = true;
        }
        else if(s == "-no_ctimer")
        {
            printCompileTime = false;
        }
        else if(s == "-sco")
        {
            //see output
            setSeeOutput(true);
        }
        else if(s == "-notypes")
        {
            //do not print output type file
            setPrintIOTypes(false);
        }*/
        else
        {
            cout << "Undefined Arguement \""<< s <<"\"\n";
        }
    }
}

int main(int argc, char *argv[])
{
    ///process args
    processArgs(argc,argv);

    
    string file = argv[1];
    
    
    {
        Interpreter interpret(printInterpreterGates,printInterpreterIO,printInterpreterHeader,printInterpreterStats,useInterpreterValidationOption,printFullGateList,gatelistfilename);
        interpret.readyProgram(file);
        interpret.runprogram();
    }
    
    
}
