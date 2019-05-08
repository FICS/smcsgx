#include <cstdio>
#include <cstring>

#include "Algebra.h"

static FILE *fp = 0;

// excerpt from PBC. super slow due to frequent IO access (commented lines)
static void my_file_mpz_random(mpz_t r, mpz_t limit, void *data) {
//  char *filename = (char *) data;
//  FILE *fp;
  int n, bytecount, leftover;
  unsigned char *bytes;
  mpz_t z;
  mpz_init(z);
//  fp = fopen(filename, "rb");
//  if (!fp) return;
  n = mpz_sizeinbase(limit, 2);
  bytecount = (n + 7) / 8;
  leftover = n % 8;
  bytes = (unsigned char *) pbc_malloc(bytecount);
  for (;;) {
    if (!fread(bytes, 1, bytecount, fp)) {
      pbc_warn("error reading source of random bits");
      return;
    }
    if (leftover) {
      *bytes = *bytes % (1 << leftover);
    }
    mpz_import(z, bytecount, 1, 1, 0, 0, bytes);
    if (mpz_cmp(z, limit) < 0) break;
  }
//  fclose(fp);
  mpz_set(r, z);
  mpz_clear(z);
  pbc_free(bytes);
}

static void my_random_set_file(char *filename)
{
	fp = fopen(filename, "rb");
	if (!fp) pbc_die("random open failed");

	pbc_random_set_function(my_file_mpz_random, filename);
}

static void my_close_random_file()
{
	fclose(fp);
}

const char *G_Base::PARAMS_FILE = "CCS.params";
G_Base::Init G_Base::I;

#include <pbc/pbc_a_param.h>
#include <iostream>

G_Base::Init::Init()
{
    // initialize pairing_t from PARAMS_FILE
	char s[16384];
	FILE *f = fopen(G_Base::PARAMS_FILE, "r");
	if (!f) pbc_die("params open failed");

	size_t count = fread(s, 1, 16384, f);
	if (!count) pbc_die("input error");
	fclose(f);

	if (pairing_init_set_buf(m_p, s, count)) pbc_die("pairing init failed");

    // set random source
	my_random_set_file(const_cast<char*>(Prng::RANDOM_FILE));

	// fast random element selection
	element_t g;
	element_init_G1(g, m_p);
	element_random(g);
	element_pp_init(m_g_pp, g);
	element_clear(g);

	// temporary variable
	element_init_Zr(m_r, m_p);
}

G_Base::Init::~Init()
{
    element_clear(m_r);
    element_pp_clear(m_g_pp);
    my_close_random_file();
    //pairing_clear(p); // can't be executed. there are other static element_t
}

G G::ret;

void exp(G &out, const G &lhs, const Z &rhs)
{
	element_s *non_const_lhs_e = const_cast<element_s*>(&(lhs.m_e[0]));
	element_s *non_const_rhs_e = const_cast<element_s*>(&(rhs.m_e[0]));
	element_pow_zn(out.m_e, non_const_lhs_e, non_const_rhs_e);
}

void pp_exp(G &out, const G &lhs, const Z &rhs)
{
	element_pp_s *non_const_lhs_e_pp = const_cast<element_pp_s*>(&(lhs.m_e_pp[0]));
	element_s *non_const_rhs_e = const_cast<element_s*>(&(rhs.m_e[0]));
	element_pp_pow_zn(out.m_e, non_const_rhs_e, non_const_lhs_e_pp);
}
