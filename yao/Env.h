#ifndef ENV_H_
#define ENV_H_

#include <cassert>

#include "Algebra.h"
#include "Bytes.h"
#include "ClawFree.h"
#include "Circuit.h"
#include "NetIO.h"


struct EnvParams
{
	EnvParams() :
		secu_param(0), stat_param(0),
		wrld_rank(0),
		node_rank(0), node_load(0), node_amnt(0),
		port_base(0), remote(0), server(0),
		circuit_file(0), private_file(0),
		ipserve_addr(0) {}

	~EnvParams() { delete remote; delete server; }

	size_t        secu_param;    // security parameter
	size_t        stat_param;    // statistical security parameter

	int           wrld_rank;

	int           node_rank;
	int           node_load;
	int           node_amnt;

	int           port_base;

	Socket       *remote;
	ServerSocket *server;

	Circuit       circuit;
	ClawFree      claw_free;

	const char   *circuit_file; // boolean circuit computing f(x,y) -> (f1, f2)
	const char   *private_file;

	const char   *ipserve_addr;
};

class Env
{
	Env (EnvParams &params) : m_params(params)
	{
		exp_length = Z().length_in_bytes();
		elm_length = G().length_in_bytes();
		key_length = (params.secu_param+7)/8;
	}

	// prohibited member functions
	Env(const Env &);
	Env &operator=(const Env &);

	EnvParams  &m_params;

	size_t      key_length;
	size_t      exp_length;
	size_t      elm_length;

	static Env *instance;  // singleton instance


public:
	enum { GEN, EVL }; // for ipserver

	static const int IP_SERVER_PORT;

	static void init(EnvParams &params)
	{
		if (!instance)
		{
			instance = new Env(params);
		}
	}

	static void destroy()
	{
		delete instance;
	}

	static size_t k()
	{
		assert(instance != 0);
		return instance->m_params.secu_param;
	}

	static uint32_t s()
	{
		assert(instance != 0);
		return instance->m_params.stat_param;
	}

	static size_t key_size_in_bytes()
	{
		assert(instance != 0);
		return instance->key_length;
	}

	static size_t exp_size_in_bytes()
	{
		assert(instance != 0);
		return instance->exp_length;
	}

	static size_t elm_size_in_bytes()
	{
		assert(instance != 0);
		return instance->elm_length;
	}

	static Circuit &circuit()
	{
		assert(instance != 0);
		return instance->m_params.circuit;
	}

	static ClawFree &clawfree()
	{
		assert(instance != 0);
		return instance->m_params.claw_free;
	}

	static void claw_free_from_bytes(const Bytes b)
	{
		assert(instance != 0);
		instance->m_params.claw_free.from_bytes(b);
	}

	static int world_rank()
	{
		assert(instance != 0);
		return instance->m_params.wrld_rank;
	}

	static bool is_evl()
	{
		assert(instance != 0);
		return instance->m_params.wrld_rank % 2;
	}

	static bool is_root()
	{
		assert(instance != 0);
		return instance->m_params.node_rank == 0;
	}

	static int group_rank()
	{
		assert(instance != 0);
		return instance->m_params.node_rank;
	}

	static int node_load()
	{
		assert(instance != 0);
		return instance->m_params.node_load;
	}

	static int node_amnt()
	{
		assert(instance != 0);
		return instance->m_params.node_amnt;
	}

	static Socket *remote()
	{
		assert(instance != 0);
		return instance->m_params.remote;
	}

	virtual ~Env() {}
};


#endif /* ENV_H_ */
