/*
 * ClawFree.h
 *
 *  Created on: Nov 7, 2010
 *      Author: shench
 */

#ifndef CLAWFREE_H_
#define CLAWFREE_H_

#include "Algebra.h"
#include "Bytes.h"
#include "Prng.h"

class ClawFree
{

	G m_g, m_h; // generators of the Diffie-Helman-based claw-free collections

public:
	ClawFree() {}

	void init()
	{
		m_g.random();
		m_h.random();
		m_h.fast_exp();
	}

	Bytes to_bytes() const
	{
		return m_g.to_bytes() + m_h.to_bytes();
	}

	void from_bytes(const Bytes &b)
	{
		std::vector<Bytes> chunks = b.split(b.size()/2);
		m_g.from_bytes(chunks[0]);
		m_h.from_bytes(chunks[1]);
		m_h.fast_exp();
	}

	size_t size_in_bytes() const
	{
		return m_g.length_in_bytes() + m_h.length_in_bytes();
	}

	Z D() const
	{
		Z z;
		z.random();
		return z;
	}

	Z D(Prng &prng) const
	{
		Z z;
		z.random(prng);
		return z;
	}

	// R(m) = h^m
	G R(const Z &m)
	{
		return m_h^m;
	}

	// F(b,m) = g^b h^m
	G F(bool b, const Z &m)
	{
		G g = R(m);
		if (b) { g *= m_g; }
		return g;
	}

	static void claw_free_pool(const bool b, const bool p, Bytes &k);
	static void claw_free_pool(const bool b, const bool p, Z &m, Bytes &k);
	static void claw_free_pool(const bool b, const bool p, Z &m, G &M, Bytes &k);

	static void claw_free_pool(const bool b, const bool p, Bytes &k, Prng &prng);
	static void claw_free_pool(const bool b, const bool p, Z &m, Bytes &k, Prng &prng);
	static void claw_free_pool(const bool b, const bool p, Z &m, G &M, Bytes &k, Prng &prng);
};


#endif /* CLAWFREE_H_ */
