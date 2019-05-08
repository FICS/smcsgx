

#include <iostream>


#include "BetterYao.h"

void startProgram(int argc, char **argv);


class EvalInput
{
public:
//for eval
	vector<vector<char>> evalgc;
 
};

class GenInput
{
public:
//for gen/seder
	vector<vector<char>> gengc;
	vector<char> permubits;

};

class Input
{
public:
	EvalInput evalInput;
	GenInput genInput;
};


class Output
{
	public:
	vector<vector<char>> output;
};

Input generateInput(char input[], int size);
Output  runProgram(string programname,EvalInput evalInput, GenInput genInput);


int main(int argc, char **argv)
{
	startProgram(argc,argv);
	char bitInput[] = {0,0,0,0, 0,0,0,0, 1,1,0,0, 1,0,0,0};		
	int inputLength = 16;

	Input programInput = generateInput(bitInput,inputLength);
	Output out = runProgram("./TestPrograms/arrayinit.wir", programInput.evalInput,programInput.genInput);


	cout << "\noutput:\n";
	for(int i=0;i<out.output.size();i++)
	{
		for(int j=0;j<10;j++)
		{
		//	cout << (unsigned char)out.output[i][j];
		printf("%hhu ",out.output[i][j]);
		}
		cout << "\n";
	}	

	return 0;
}



//real bits contains an array of "real", non garbled, values to be transformed into values for both the generator and evaluator. each char of realbits contains 1 bit (the least significaunt bit) of data- the other 7 bits are ignored. Both these second two arrays are passed by reference 
void transformToGarbled(const vector<char> & realbits, vector<vector<char>> & genvalues, vector<vector<char>> & evalvalues, vector<char> & permutes)
{
	genvalues.resize(realbits.size()*2);
	evalvalues.resize(realbits.size());

	permutes.resize(realbits.size());

	srand(3);	

	//swap this out for sgx hardware rand function
//	Prng m_prng;
//	;

	for( int i=0;i<genvalues.size();i+=2)
	{
		genvalues[i].resize(10);
		genvalues[i+1].resize(10);
	
		for(int j=0;j<10;j++)
		{
			genvalues[i][j] = rand();//m_prng.rand(8)[0];
			genvalues[i+1][j] = rand();//m_prng.rand(16)[1];
		}

		if(((genvalues[i][0])&0x01) == ((genvalues[i+1][0])&0x01))
		{
			(genvalues[i][0])^= 0x01;
		}

		evalvalues[i/2].resize(10);

		//cout << "bit: "<<i/2<<" "<<realbits[i/2]<<"\n";

		if (((genvalues[i][0])&0x01) == 0)
		{
			permutes[i/2] = 0;
		}
		else
		{
			permutes[i/2] = 1;
		}


		for(int j=0;j<10;j++)
		{
			evalvalues[i/2][j] = realbits[i/2] == 0 ? genvalues[i][j] : genvalues[i+1][j]; 
		}

	}	
}


Bytes transform_to_bytes(vector<char> inarray)
{
	Bytes b;
	b.resize(16);
	
	for(int i=0;i<10;i++)
	{
		b[i] = inarray[i];
	}

	b[10] = 0;
	b[11] = 0;
	b[12] = 0;
	b[13] = 0;
	b[14] = 0;
	b[15] = 0;

	return b;
}

void transform_all_to_bytes_vector(vector<vector<char> > & input,  vector<Bytes> & out)
{
	out.resize(input.size());
	for(int i=0;i<input.size();i++)
	{
		out[i] = transform_to_bytes(input[i]);
	}
}



EnvParams * paramsGlobal=0;

BetterYao * sysC=0;

void startProgram(int argc, char **argv)
{
	std::cout <<"evl is client, gen is host:::";

	if (argc < 8)
	{
		std::cout << "Usage:" << std::endl
			<< "\tbetteryao [secu_param] [stat_param] [circuit_file] [input_file] [ip_server] [port_base] [setting]" << std::endl
			<< std::endl
			<< "[secu_param]: multiple of 8 but 128 at most" << std::endl
			<< "[stat_param]: multiple of the cluster size" << std::endl
			<< " [ip_server]: the IP (not domain name) of the IP exchanger" << std::endl
			<< " [port_base]: for the \"IP address in use\" hassles" << std::endl
			<< "   [setting]: 0 = honest-but-curious; 1 = malicious (I+2C); 2 = malicious (I+C)" << std::endl
			<< std::endl;
		exit(EXIT_FAILURE);
	}


	paramsGlobal = new EnvParams();

		paramsGlobal->secu_param   = atoi(argv[1]);
	paramsGlobal->stat_param   = atoi(argv[2]);

	paramsGlobal->circuit_file = argv[3];
	paramsGlobal->private_file = argv[4];
	paramsGlobal->ipserve_addr = argv[5];

	paramsGlobal->port_base    = atoi(argv[6]);

	int setting         = atoi(argv[7]);

	switch (setting)
	{
	case 0:
		//sys = new Yao(params);
		break;

	case 1:
		sysC = new BetterYao(*paramsGlobal);
		break;

	case 2:
		//sys = new BetterYao2(params);
		break;
	}
	sysC->initSGXProgram();
}


Input generateInput(char input[], int size)
{

Input toReturn;

vector<vector<char>> evalgc;
vector<vector<char>> gengc;

vector<char> permutes;
vector<char> iinputs = {1,1,0,0,0,0,0,0  ,1,0,1,0,0,0,0,0};



transformToGarbled(iinputs, gengc,evalgc,permutes);

	toReturn.evalInput.evalgc = evalgc;
	toReturn.genInput.gengc = gengc;
	toReturn.genInput.permubits = permutes;

	return toReturn;
}

Output runProgram(string programname,EvalInput evalInput, GenInput genInput)
//functino header here
{

vector<Bytes> input;
vector<Bytes> output;
vector<char> permutes;

vector<Bytes> evalin;
transform_all_to_bytes_vector(evalInput.evalgc,evalin);

vector<Bytes> genin;
 transform_all_to_bytes_vector(genInput.gengc,genin);




permutes = genInput.permubits;


GEN_BEGIN
	sysC->runSGXProgram(programname,&genin,&output, &permutes);
GEN_END

EVL_BEGIN
	sysC->runSGXProgram(programname,&evalin,&output, &permutes);
EVL_END



/*cout << "output:\n";
for(int i=0;i<output.size();i++)
{
	cout << output[i].to_hex()<<"\n";
}
cout << "\n";
*/

Output out;
out.output.resize(output.size());

for(int i=0;i<output.size();i++)
{
	out.output[i].resize(10);
	for(int j=0;j<10;j++)
	{
		out.output[i][j] = output[i][j];
	}	
}


return out;


}


