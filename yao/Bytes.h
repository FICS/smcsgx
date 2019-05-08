#ifndef BYTES_H_
#define BYTES_H_

#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <string>
#include <vector>


typedef uint8_t byte;


class Bytes : public std::vector<byte>
{
	struct bitwise_xor{ byte operator() (byte l, byte r) const { return l ^ r;  }};
	struct bitwise_and{ byte operator() (byte l, byte r) const { return l & r;  }};

public:
	Bytes() {}
	Bytes(uint64_t n) : std::vector<byte>(n) {}
	Bytes(uint64_t n, byte b) : std::vector<byte>(n, b) {}
	Bytes(byte *begin, byte *end) : std::vector<byte>(begin, end) {}
	Bytes(const_iterator begin, const_iterator end) : std::vector<byte>(begin, end) {}
	Bytes(const std::vector<Bytes> &chunks) { merge(chunks); }

	const Bytes &operator =(const Bytes &rhs)
	{
		this->assign(rhs.begin(), rhs.end());
		return *this;
	}

	const Bytes &operator +=(const Bytes &rhs)
	{
		this->insert(this->end(), rhs.begin(), rhs.end());
		return *this;
	}

	const Bytes &operator ^=(const Bytes &rhs)
	{
		// TODO: see if this can be improved by a wider data type pointer
		assert(rhs.size() == size());
		Bytes::iterator dst = this->begin();
		Bytes::const_iterator src = rhs.begin();
		while (dst != this->end()) { *dst++ ^= *src++;}
		return *this;
	}

	const Bytes &operator &=(const Bytes &rhs)
	{
		// TODO: see if this can be improved by a wider data type pointer
		assert(rhs.size() == size());
		Bytes::iterator dst = this->begin();
		Bytes::const_iterator src = rhs.begin();
		while (dst != this->end()) { *dst++ &= *src++;}
		return *this;
	}

	byte get_ith_bit(uint64_t ix) const
	{
	//	assert(ix < size()*8LL);
		return ((*this)[ix>>3] >> (ix%0x08)) & 0x01;
	}

	void set_ith_bit(uint64_t ix, byte bit)
	{
	//	assert(ix < size()*8LL);
		static const byte INVERSE_MASK[8] =
			{ 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F };

		(*this)[ix>>3] &= INVERSE_MASK[ix%8];
		(*this)[ix>>3] |= (bit&0x01) << (ix%8);
	}

	std::string to_hex() const;
	void from_hex(const std::string &s);

	Bytes hash(size_t bits) const;
	std::vector<Bytes> split(const size_t chunk_len) const;
	void merge(const std::vector<Bytes> &chunks);
	void merge2(const std::vector<Bytes> &chunks);


	inline void setBytes(long l)
	{
	int index=0;


	resize(16,0);

	long x = l;
	while (l>0)
	{
		if(l&1==1)
		{
			set_ith_bit(index,1);
		}

		l=l>>1;
		index++;
	}

	index=0;
	while (x>0)
	{
		if(x&1==1)
		{
			set_ith_bit(index+64,1);
		}

		x=x>>1;
		index++;
	}
	
}
};

// pre-condition: lhs.size() == rhs.size()
inline Bytes operator^ (const Bytes &lhs, const Bytes &rhs)
{
	assert(lhs.size() == rhs.size());
	Bytes ret(lhs);
	ret ^= rhs;
	return ret;
}

inline Bytes operator+ (const Bytes &lhs, const Bytes &rhs)
{
	Bytes ret(lhs);
	ret += rhs;
	return ret;
}

inline bool operator ==(const Bytes &lhs, const Bytes &rhs)
{
	return (lhs.size() != rhs.size())?
		false : std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline Bytes operator&& (const Bytes &lhs, const Bytes &rhs)
{
	assert(lhs.size() == rhs.size());
	Bytes ret(lhs);
	ret &= rhs;
	return ret;
}



#endif /* BYTES_H_ */
