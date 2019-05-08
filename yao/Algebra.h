#ifndef ALGEBRA_H_
#define ALGEBRA_H_

#include <pbc/pbc.h>

#include "Bytes.h"
#include "Prng.h"


class G_Base
{
	class Init
	{
	public:
		Init();
		~Init();
		pairing_t 		          m_p;
		element_t                 m_r;
		element_pp_t           m_g_pp;

		Prng                   m_prng;
	};

protected:
	static Init I;

	element_t m_e;

public:
	static const char *PARAMS_FILE;

	int length_in_bytes() const
	{
		return element_length_in_bytes(const_cast<element_s*>(&m_e[0]));
	}

	Bytes to_bytes() const
	{
		Bytes ret(length_in_bytes());
		element_to_bytes(&ret[0], const_cast<element_s*>(&m_e[0]));
		return ret;
	}

	void from_bytes(const Bytes &b)
	{
		element_from_bytes(const_cast<element_s*>(&m_e[0]), const_cast<byte*>(&b[0]));
	}

	virtual void random() = 0;
	virtual void random(Prng &prng) = 0;

	virtual ~G_Base()
	{
		element_clear(m_e);
	}
};

class G;

class Z : public G_Base
{
	friend const G &operator ^(const G &lhs, const Z &rhs);
	friend void    exp(G &out, const G &lhs, const Z &rhs);
	friend void pp_exp(G &out, const G &lhs, const Z &rhs);

	void init()
	{
		element_init_Zr(m_e, I.m_p);
	}
public:
	Z()
	{
		init();
	}

	Z(const Z &z)
	{
		init();
		element_set(m_e, const_cast<element_s*>(&(z.m_e[0])));
	}

	Z(const Bytes &b)
	{
		init();
		element_from_bytes(m_e, const_cast<byte*>(&b[0]));
	}

	Z(signed long int i)
	{
		init();
		element_set_si(m_e, i);
	}

	virtual void random() { random(I.m_prng); }

	virtual void random(Prng &prng)
	{
		Bytes rnd = prng.rand(element_length_in_bytes(m_e)*8);
		element_from_bytes(m_e, &rnd[0]);
	}

	Z &operator=(const Z &rhs)
	{
		element_set(m_e, const_cast<element_s*>(&(rhs.m_e[0])));
		return *this;
	}

	Z &operator+= (const Z &rhs)
	{
		element_add(m_e, m_e, const_cast<element_s*>(&(rhs.m_e[0])));
		return *this;
	}

	Z &operator-= (const Z &rhs)
	{
		element_sub(m_e, m_e, const_cast<element_s*>(&(rhs.m_e[0])));
		return *this;
	}

	Z &operator*= (const Z &rhs)
	{
		element_mul(m_e, m_e, const_cast<element_s*>(&(rhs.m_e[0])));
		return *this;
	}
};

inline Z operator+(const Z &lhs, const Z &rhs)
{
	Z ret(lhs);
	ret += rhs;
	return ret;
}

inline Z operator-(const Z &lhs, const Z &rhs)
{
	Z ret(lhs);
	ret -= rhs;
	return ret;
}

inline Z operator*(const Z &lhs, const Z &rhs)
{
	Z ret(lhs);
	ret *= rhs;
	return ret;
}

extern void    exp(G &out, const G &lhs, const Z &rhs);
extern void pp_exp(G &out, const G &lhs, const Z &rhs);

// multiplicative group
class G : public G_Base
{
	element_pp_t                        m_e_pp;
	bool		                m_is_e_pp_init;
	void (*m_fptr_exp)(G&, const G&, const Z&); // use to call exp or pp_exp

	friend const G &operator ^(const G &lhs, const Z &rhs);
	friend void    exp(G &out, const G &lhs, const Z &rhs);
	friend void pp_exp(G &out, const G &lhs, const Z &rhs);

	void init()
	{
		m_fptr_exp = &exp;
		m_is_e_pp_init = false;
		element_init_G1(m_e, I.m_p);
	}

	static G ret;

public:
	G()
	{
		init();
	}

	G(const G &g)
	{
		init();
		element_set(m_e, const_cast<element_s*>(&(g.m_e[0])));
	}

	G(const Bytes &b)
	{
		init();
		element_from_bytes(m_e, const_cast<byte*>(&b[0]));
	}

	void from_bytes(const Bytes &b)
	{
		if (m_is_e_pp_init) element_pp_clear(m_e_pp);
		element_clear(m_e);

		init();
		G_Base::from_bytes(b);
	}

	// it's worth-preprocessing only if 5+ exps will be executed
	void fast_exp()
	{
		if (m_is_e_pp_init) element_pp_clear(m_e_pp);
		element_pp_init(m_e_pp, m_e);
		m_is_e_pp_init = true;
		m_fptr_exp = &pp_exp;
	}

	virtual void random() { random(I.m_prng); }

	virtual void random(Prng &prng)
	{
		m_fptr_exp = &exp; // preprocessing needs to be redone

//		element_random(I.m_r);
		Bytes rnd = prng.rand(element_length_in_bytes(I.m_r)*8);
		element_from_bytes(I.m_r, &rnd[0]);
		element_pp_pow_zn(m_e, I.m_r, I.m_g_pp);
	}

	G &operator=(const G &rhs)
	{
		m_fptr_exp = &exp; // preprocessing needs to be redone
		element_set(m_e, const_cast<element_s*>(&(rhs.m_e[0])));
		return *this;
	}

	G &operator*= (const G &rhs)
	{
		m_fptr_exp = &exp; // preprocessing needs to be redone
		element_mul(m_e, m_e, const_cast<element_s*>(&(rhs.m_e[0])));
		return *this;
	}

	G &operator/= (const G &rhs)
	{
		m_fptr_exp = &exp; // preprocessing needs to be redone
		element_div(m_e, m_e, const_cast<element_s*>(&(rhs.m_e[0])));
		return *this;
	}

	bool operator==(const G &rhs) const
	{
		return element_cmp
		(
			const_cast<element_s*>(&(this->m_e[0])),
			const_cast<element_s*>(&(rhs.m_e[0]))
		) == 0;
	}

	virtual ~G()
	{
		if (m_is_e_pp_init) element_pp_clear(m_e_pp);
	}
};

inline G operator*(const G &lhs, const G &rhs)
{
	G ret(lhs);
	ret *= rhs;
	return ret;
}

inline const G &operator^(const G &lhs, const Z &rhs)
{
	lhs.m_fptr_exp(G::ret, lhs, rhs);
	return G::ret;
}

#endif /* ALGEBRA_H_ */
