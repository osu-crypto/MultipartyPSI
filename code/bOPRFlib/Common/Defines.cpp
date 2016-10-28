#include "Common/Defines.h"
#include "Crypto/Commit.h"
#include "Common/BitVector.h"
//#include "Common/Timer.h"

namespace bOPRF {

	Timer gTimer;
	const block ZeroBlock = _mm_set_epi64x(0, 0);
	const block OneBlock = _mm_set_epi64x(0, 1);
	const block AllOneBlock = _mm_set_epi64x(u64(-1), u64(-1));
	const block CCBlock = ([]() {block cc; memset(&cc, 0xcc, sizeof(block)); return cc; })();

	std::ostream& operator<<(std::ostream& out, const block& blk)
	{
		out << std::hex;
		u64* data = (u64*)&blk;

		out << std::setw(16) << std::setfill('0') << data[0]
			<< std::setw(16) << std::setfill('0') << data[1];

		out << std::dec << std::setw(0);

		return out;
	}

	std::ostream& operator<<(std::ostream& out, const blockBop& blk)
	{
		out << std::hex;
		u64* data = (u64*)&blk;

		out << std::setw(16) << std::setfill('0') << data[0] << "..."
			//<< std::setw(16) << std::setfill('0') << data[1]
			//<< std::setw(16) << std::setfill('0') << data[2]
			//<< std::setw(16) << std::setfill('0') << data[3]
			//<< std::setw(16) << std::setfill('0') << data[4]
			//<< std::setw(16) << std::setfill('0') << data[5]
			<< std::setw(16) << std::setfill('0') << data[6];

		out << std::dec << std::setw(0);

		return out;
	}


	std::ostream* gOut = &std::cout;



	std::ostream& operator<<(std::ostream& out, const Commit& comm)
	{
		out << std::hex;

		u32* data = (u32*)comm.data();

		out << std::setw(8) << std::setfill('0') << data[0]
			<< std::setw(8) << std::setfill('0') << data[1]
			<< std::setw(8) << std::setfill('0') << data[2]
			<< std::setw(8) << std::setfill('0') << data[3]
			<< std::setw(8) << std::setfill('0') << data[4];

		out << std::dec << std::setw(0);

		return out;
	}
	block PRF(const block& b, u64 i)
	{
		//TODO("REMOVE THIS!!");
		//return b;





		block ret, tweak = _mm_set1_epi64x(i), enc;

		ret = b ^ tweak;

		mAesFixedKey.ecbEncBlock(ret, enc);

		ret = ret ^ enc; // H( a0 ) 

		return ret;
	}

	void split(const std::string &s, char delim, std::vector<std::string> &elems) {
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			elems.push_back(item);
		}
	}

	std::vector<std::string> split(const std::string &s, char delim) {
		std::vector<std::string> elems;
		split(s, delim, elems);
		return elems;
	}

	const int tab64[64] = {
		63,  0, 58,  1, 59, 47, 53,  2,
		60, 39, 48, 27, 54, 33, 42,  3,
		61, 51, 37, 40, 49, 18, 28, 20,
		55, 30, 34, 11, 43, 14, 22,  4,
		62, 57, 46, 52, 38, 26, 32, 41,
		50, 36, 17, 19, 29, 10, 13, 21,
		56, 45, 25, 31, 35, 16,  9, 12,
		44, 24, 15,  8, 23,  7,  6,  5 };


	u64 log2floor(u64 value)
	{
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;
		value |= value >> 32;
		return tab64[((uint64_t)((value - (value >> 1)) * 0x07EDD5E59A4E28C2)) >> 58];
	}

	u64 log2ceil(u64 value)
	{
		return u64(std::ceil(std::log2(value)));
	}

	u64 get_stash_size(u64 neles) {
		if (neles >= (1 << 24))
			return 2;
		if (neles >= (1 << 20))
			return 3;
		if (neles >= (1 << 16))
			return 4;
		if (neles >= (1 << 12))
			return 6;
		if (neles >= (1 << 8))
			return 12;
		
		return 12; //other
		
		throw std::runtime_error("get_stash_size: rt error at " LOCATION);

	}
	u64 get_bin_size(u64 neles) {
		if (neles >= (1 << 24))
			return 28;
		if (neles >= (1 << 20))
			return 27;
		if (neles >= (1 << 16))
			return 26;
		if (neles >= (1 << 12))
			return 25;
		if (neles >= (1 << 8))
			return 24;
	
		return 24;  //other

		//throw std::runtime_error("get_bin_size: rt error at " LOCATION);
	}

	u64 get_codeword_size(u64 neles) {
		if (neles >= (1 << 24))
			return 448/8; // in byte
		if (neles >= (1 << 20))
			return 448/8;
		if (neles >= (1 << 16))
			return 440/8;
		if (neles >= (1 << 12))
			return 432/8;
		if (neles >= (1 << 8))
			return 424/8;

		return 424 /8;

		//throw std::runtime_error("get_codeword_size: rt error at " LOCATION);
	}
	u64 get_mask_size(u64 neles) {
		if (neles >= (1 << 24))
			return 88/8;  // in byte
		if (neles >= (1 << 20))
			return 80 / 8;
		if (neles >= (1 << 16))
			return 72 / 8;
		if (neles >= (1 << 12))
			return 64 / 8;
		if (neles >= (1 << 8))
			return 56 / 8;

		return 56 /8;
		//return (40 + 2 * log(neles)) / 8;

	//	throw std::runtime_error("get_codeword_size: rt error at " LOCATION);
	}
}


