#include "BitPosition.h"
#include "Crypto/sha1.h"
#include "Crypto/PRNG.h"
#include <random>
#include "Common/Log.h"
#include "Common/Log1.h"
#include <numeric>


namespace osuCrypto
{
	int isSet(block & v, int n)
	{
		__m128i chkmask = _mm_slli_epi16(_mm_set1_epi16(1), n & 0xF);
		int     movemask = (1 << (n >> 3));
		int     isSet = (((_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_and_si128(chkmask, v), _mm_setzero_si128())) & movemask) ^ movemask));
		return isSet;
	}
	void setBit(block & v, int pos)
	{
		__m128i shuf = _mm_set_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
		shuf = _mm_add_epi8(shuf, _mm_set1_epi8(16 - (pos >> 3)));
		shuf = _mm_and_si128(shuf, _mm_set1_epi8(0x0F));
		__m128i setmask = _mm_shuffle_epi8(_mm_cvtsi32_si128(1 << (pos & 0x7)), shuf);
		v = _mm_or_si128(v, setmask);
	}
	bool TestBitN(__m128i value, int N)
	{
		__m128i positioned = _mm_slli_epi64(value, 7 - (N & 7));
		return (_mm_movemask_epi8(positioned) & (1 << (N / 8))) != 0;
	}



	BaseOPPRF::BaseOPPRF()
	{
	}

	BaseOPPRF::~BaseOPPRF()
	{
	}

	BitPosition::BitPosition()
	{
	}

	BitPosition::~BitPosition()
	{
	}


	void BitPosition::print() const
	{
		std::cout << "mPos: \n";
		for (auto it = mPos.begin(); it != mPos.end(); ++it)
		{
			std::cout << static_cast<int16_t>(*it) << "  ";
		}
		std::cout << std::endl;
		std::cout << "Masks: ";
		for (auto iter = mMaps.begin(); iter != mMaps.end(); ++iter) {
			std::cout << static_cast<int16_t>((*iter)) << " ";
		}

	}
	void BitPosition::init(/*u64 numRealCodeWord,*/ u64 numMaxBitSize)
	{
		/*mRealBitSize= std::floor(std::log2(numRealCodeWord)) + 1;*/
		mMaxBitSize = numMaxBitSize;

	}

	//#################Table based
	bool BitPosition::getMasks(std::vector<block>& codeword) {

		u8 rs, idx;
		for (int i = 0; i < codeword.size(); i++) {
			rs = 0;
			idx = 1;
			for (auto it = mPos.begin(); it != mPos.end(); ++it)
			{
				if (TestBitN(codeword[i], *it))
				{
					rs = rs^idx;
					//std::cout << static_cast<int16_t>(idx) << std::endl;
				}
				idx = idx << 1;
			}
			if (std::find(mMaps.begin(), mMaps.end(), rs) == mMaps.end())
			{
				mMaps.push_back(rs);
			}
			else
			{
				mMaps.clear();
				return false;
			}
		}
		return true;
	}
	void BitPosition::getMask(block& codeword, u8& mask) {

		u8 rs, idx;
		mask = 0;
		idx = 1;
		for (auto it = mPos.begin(); it != mPos.end(); ++it)
		{
			if (TestBitN(codeword, *it))
			{
				mask = mask^idx;
				//std::cout << static_cast<int16_t>(idx) << std::endl;
			}
			idx = idx << 1;
		}
	}

	int BitPosition::midIdx(std::vector<block>& codewords, int length)
	{
		int temp = 0;
		int idx = 0;
		int mid = 0;
		int cnt = 0;
		//std::cout << "temp ";
		if (codewords.size() == 1) {
			while (true)
			{
				auto rand = std::rand() % 128; //choose randome bit location
				if (std::find(mPos.begin(), mPos.end(), rand) == mPos.end())
				{
					return rand;
				}
			}
		}
		else if (codewords.size() == 2) {
			block diff = codewords[0] ^ codewords[1];
			for (size_t j = 0; j < length; j++)
			{
				if (TestBitN(diff, j))
					if (std::find(mPos.begin(), mPos.end(), j) == mPos.end())
					{
						return j;
					}
			}
		}
		else
			for (size_t j = 0; j < length; j++)
			{
				temp = 0;
				if (std::find(mPos.begin(), mPos.end(), j) == mPos.end())
				{
					cnt++;
					for (size_t i = 0; i < codewords.size(); i++)
					{
						if (TestBitN(codewords[i], j))
							temp++;
					}
					//std::cout << j << "," << temp << " ";
					if (temp < codewords.size() / 2 && mid < temp)
					{
						mid = temp;
						idx = j;
					}
				}
			}
		//	std::cout << idx << " - " << mid << " - " << codewords.size() << " - " << cnt << std::endl;
		return idx;
	}


	std::vector<std::vector<block>> testSet;
	int idxS = -1;
	void BitPosition::getPosHelper(std::vector<block>& codewords, int length)
	{
		idxS++;
		int setSize = codewords.size();
		std::vector<block> testSet1;
		std::vector<block> testSet2;

		if (mPos.size() < mRealBitSize) {
			int idx = midIdx(codewords, length);

			//if (std::find(mPos.begin(), mPos.end(), idx) == mPos.end())
			{
				mPos.push_back(idx);
				std::cout << std::endl;
				for (size_t i = 0; i < setSize; i++)
					if (TestBitN(codewords[i], idx))
					{
						//	std::cout << codewords[i] << " " << idx << " "<< 1 << std::endl;
						testSet1.push_back(codewords[i]);

					}
					else
					{
						//std::cout << codewords[i] << " " << idx << " " << 0 << std::endl;
						//std::cout << "TestBitN=0: " << i << std::endl;
						testSet2.push_back(codewords[i]);
					}

				for (size_t i = 0; i < testSet1.size(); i++)
					std::cout << testSet1[i] << " " << idx << " " << 1 << std::endl;

				std::cout << std::endl;

				for (size_t i = 0; i < testSet2.size(); i++)
					std::cout << testSet2[i] << " " << idx << " " << 0 << std::endl;


				testSet.push_back(testSet1);
				testSet.push_back(testSet2);
				//std::cout <<"testSet1 " <<  testSet1.size() << std::endl;
				//std::cout <<"testSet2 "<< testSet2.size() << std::endl;
				getPos(testSet[idxS], length);
			}
		}
	}

	void BitPosition::getPos1(std::vector<block>& codewords, int length)
	{
		bool isFind = false;

		if (codewords.size() == 1) {
			getRandPos();
			getMasks(codewords);
		}
		if (codewords.size() == 2) {
			block diff = codewords[0] ^ codewords[1];
			while (!isFind)
			{
				u64 rand = std::rand() % length;
				if (TestBitN(diff, rand))
				{
					mPos.push_back(rand);
					isFind = true;
				}
			}
			getRandPos();
			getMasks(codewords);
		}
		if (codewords.size() == 3) {
			block diff = codewords[0] ^ codewords[1];
			while (!isFind)
			{
				u64 rand = std::rand() % length;
				if (TestBitN(diff, rand))
				{
					mPos.push_back(rand);
					isFind = true;
				}
			} //find 1st position

			isFind = false; //start to find 2nd position
			block diff2 = codewords[0] ^ codewords[2];

			while (!isFind)
			{
				u64 rand = std::rand() % length;
				if (TestBitN(diff, rand) == false && TestBitN(diff2, rand) == true)
				{
					if (std::find(mPos.begin(), mPos.end(), rand) == mPos.end())
					{
						mPos.push_back(rand);
						isFind = true;
					}
				}
			} //find 2nd position
			getRandPos();
			getMasks(codewords);
		}
		else
		{
			std::vector<block> diff;

			for (int i = 0; i + 1 < codewords.size(); i += 2) {
				diff.push_back(codewords[i] ^ codewords[i + 1]);
			}
			if (codewords.size() % 2 == 1)
				diff.push_back(codewords[codewords.size() - 1]);

			u64 sizeDiff = diff.size();

			while (!isFind)
			{
				mMaps.clear();
				mPos.clear();
				block m = ZeroBlock;
				while (mPos.size() < mMaxBitSize)
				{
					bool isRand = true;
					while (isRand)
					{
						u64 rIdx = std::rand() % length;
						u64 rDiffIdx = std::rand() % sizeDiff;
						if (TestBitN(diff[rDiffIdx], rIdx))
							if (std::find(mPos.begin(), mPos.end(), rIdx) == mPos.end())
							{
								mPos.push_back(rIdx);
								isRand = false;
								//setBit(m, rand);
							}
					}
				}

				//test mPos
				/*for (size_t i = 0; i < codewords.size(); i++)
				{
				block a = codewords[i] & m;
				Log::out << a << Log::endl;
				checkUnique.push_back(a);
				}*/

				isFind = getMasks(codewords);

				//// using default comparison:
				//std::vector<u8>::iterator it;
				//it = std::unique(mMaps.begin(), mMaps.end());

				////remove duplicate
				//mMaps.resize(std::distance(mMaps.begin(), it));

			}
		}
	}
	void BitPosition::getPos(std::vector<block>& codewords, int length)
	{
		getPosHelper(codewords, length);
		//getRandPos();									 
	}
	void BitPosition::getRandPos()
	{
		while (mPos.size()<mMaxBitSize)
		{
			u64 rand = std::rand() % 128; //choose randome bit location
			if (std::find(mPos.begin(), mPos.end(), rand) == mPos.end())
			{
				mPos.push_back(rand);
			}
		}


	}

	//#################POLYNOMIAL

	void BaseOPPRF::poly_init(u64 numBytes) {
		mGf2x.~GF2X();
		mNumBytes = numBytes;
		NTL::BuildIrred(mGf2x, numBytes * 8);
		NTL::GF2E::init(mGf2x);

	}

	/*void BaseOPPRF::reset() {
	mGf2x.~GF2X();
	}*/

	void BaseOPPRF::GF2EFromBlock(NTL::GF2E &element, block& blk, u64 size) {

		//NTL::GF2X x1;

		//x1=NTL::BuildSparseIrred_GF2X(128);
		//NTL::GF2E::init(x1);
		//convert the Block to GF2X element.
		NTL::GF2XFromBytes(mGf2x, (u8*)&blk, size);

		//TODO("remove this hack, get NTL thread safe");
		element = to_GF2E(mGf2x);
		//NTL::random(element);

	}

	void BaseOPPRF::BlockFromGF2E(block& blk, NTL::GF2E & element, u64 size) {


		//Get the bytes of the random element.
		NTL::GF2X fromEl = NTL::rep(element); //convert the GF2E element to GF2X element.	
											  //the function rep returns the representation of GF2E as the related GF2X, it returns as read only.
		BytesFromGF2X((u8*)&blk, fromEl, size);
	}


	//computes coefficients (in blocks) of f such that f(x[i]) = y[i]
	void BaseOPPRF::getBlkCoefficients(NTL::vec_GF2E& vecX, NTL::vec_GF2E& vecY, std::vector<block>& coeffs)
	{

		NTL::GF2E e;

		//interpolate
		NTL::GF2EX polynomial = NTL::interpolate(vecX, vecY);

		////convert coefficient to vector<block> 
		coeffs.resize(NTL::deg(polynomial) + 1);
		for (int i = 0; i < coeffs.size(); i++) {
			//get the coefficient polynomial
			e = NTL::coeff(polynomial, i);
			BlockFromGF2E(coeffs[i], e, mNumBytes);
		}
	}


	void BaseOPPRF::getBlkCoefficients(u64 degree, std::vector<block>& setX, std::vector<block>& setY, std::vector<block>& coeffs)
	{
		NTL::vec_GF2E x; NTL::vec_GF2E y;
		NTL::GF2E e;

		//Log::out << "-----Poly------" << Log::endl;


		for (u64 i = 0; i < setX.size(); ++i)
		{
			//	Log::out << "setX["<<i<<"]: " << setX[i] << Log::endl;
			//	Log::out << "setY[" << i << "]: " << setX[i] << Log::endl;


			GF2EFromBlock(e, setX[i], mNumBytes);
			x.append(e);

			GF2EFromBlock(e, setY[i], mNumBytes);
			y.append(e);
		}


		NTL::GF2EX polynomial = NTL::interpolate(x, y);

		//indeed, we dont need to pad dummy item to max_bin_size
		//we can compute a polynomial over real items
		//for exaple, there are 3 items in a bin (xi,yi) => interpolate poly p1(x) of a degree 2
		// gererate a root poly pRoot(x) of degree 2 over (xi,0)
		// gererate a dummy poly dummy(x) of degree max_bin_size - degree of p1(x)
		//send coff of poly dummy(x)*pRoot(x)+p1(x)
		//if x*=xi =>pRoot(xi)=0 => get p1(x*)

		NTL::GF2EX root_polynomial;
		NTL::BuildFromRoots(root_polynomial, x);


		NTL::GF2EX dummy_polynomial;
		/*for (u64 i = setX.size(); i < degree-1; ++i)
		{
		NTL::random(e);
		x.append(e);
		}*/

		NTL::random(dummy_polynomial, degree - setX.size());
		NTL::GF2EX d_polynomial;
		//NTL::mul(d_polynomial, dummy_polynomial, root_polynomial);

		//NTL::BuildFromRoots(dummy_polynomial, x);

		//for (u64 i = 0; i < setX.size(); ++i)
		//{
		//	NTL::GF2E e1 = NTL::eval(d_polynomial, x[i]); //get y=f(x) in GF2E
		//	if (e1 != 0)
		//	std::cout << x[i] << std::endl;
		//}




		//NTL::BuildFromRoots(dummy_polynomial, x);

		//NTL::random(dummy_polynomial, degree- setX.size()-1);
		// NTL::add(polynomial, d_polynomial, polynomial);
		polynomial = polynomial + dummy_polynomial*root_polynomial;

		//	dummy_polynomial.~GF2EX();
		//	root_polynomial.~GF2EX();

		//for (u64 i = 0; i < setX.size(); ++i)
		//{
		// NTL::GF2E e1 = NTL::eval(polynomial, x[i]); //get y=f(x) in GF2E
		// if (e1 != y[i])
		//	 throw std::runtime_error(LOCATION);
		//// else
		////	 std::cout << "ok" << std::endl;
		//}


		//std::cout << NTL::deg(polynomial) << std::endl;

		////convert coefficient to vector<block> 
		coeffs.resize(NTL::deg(polynomial) + 1);
		for (int i = 0; i < coeffs.size(); i++) {
			//get the coefficient polynomial
			e = NTL::coeff(polynomial, i);
			BlockFromGF2E(coeffs[i], e, mNumBytes);
		}
	}

	//compute y=f(x) giving coefficients (in block)
	void BaseOPPRF::evalPolynomial(std::vector<block>& coeffs, block& x, block& y)
	{
		NTL::GF2EX res_polynomial;
		NTL::GF2E e;
		//std::cout << coeffs.size() << std::endl;
		for (u64 i = 0; i < coeffs.size(); ++i)
		{
			GF2EFromBlock(e, coeffs[i], mNumBytes);
			NTL::SetCoeff(res_polynomial, i, e); //build res_polynomial
		}

		GF2EFromBlock(e, x, mNumBytes);
		e = NTL::eval(res_polynomial, e); //get y=f(x) in GF2E
		BlockFromGF2E(y, e, mNumBytes); //convert to block 
	}

	NTL::GF2EX BaseOPPRF::buildPolynomial(std::vector<block>& coeffs)
	{
		NTL::GF2EX res_polynomial;
		NTL::GF2E e;
		//std::cout << coeffs.size() << std::endl;
		for (u64 i = 0; i < coeffs.size(); ++i)
		{
			GF2EFromBlock(e, coeffs[i], mNumBytes);
			NTL::SetCoeff(res_polynomial, i, e); //build res_polynomial
		}
		return res_polynomial;
	}
}

//void BaseOPPRF::findPos(std::vector<block>& codewords) {
//	bool isDone=false;
//	mMasks.clear();
//	std::set<u8>::iterator it;
//	std::pair<std::set<u8>::iterator, bool> ret;
//	std::pair<std::set<u8>::iterator, bool> retMask;

//	mNumTrial = 0;
//	while (!isDone) {
//		mPos.clear();
//		mMasks.clear();
//		for (u8 i = 0; i < mSize; i++)
//		{
//			u64 rand = std::rand() % 128; //choose randome bit location
//			ret = mPos.insert(rand);
//			if (ret.second == false) 
//				it = ret.first;
//		}
//	//	std::cout << "\n\n ";
//		//print();
//		isDone = true;	
//		//std::set<u8> masks1;

//		
//		for (u8 i = 0; i < codewords.size(); i++)
//		{
//			auto m = map(codewords[i]);
//			std::cout << static_cast<int16_t>(m) << " ";
//			retMask = mMasks.insert(m);
//			if (retMask.second == false) {
//				//std::cout << mNumTrial << std::endl;
//				isDone = false;
//				mNumTrial++;
//				break;
//			}
//		}
//		std::cout <<"\n-------\n";
//	}
//	//std::cout << static_cast<int16_t>(masks[0]) << " ";
//}

//u8 BaseOPPRF::map(block& codeword) {
//	u8 rs = 0;
//	u8 idx = 0;
//	for (auto it = mPos.begin(); it != mPos.end(); ++it)
//	{
//		//	int i = *it / 8; //index of a block of 8 bits
//		//	int r = *it % 8;		//get bit location in i
//
//		////	std::cout << " " << i << ":" << r << std::endl;
//		////	__int8 c = codeword.m128i_i8[i];  //a block of 8 bits
//
//		//	//std::cout << static_cast<int16_t>(c) << std::endl;
//		//	u8 cq = ((codeword.m128i_i8[i] << (7 - r))); //shift to rightmost and left most to get only the single bit
//		//	cq = (cq >> 7) << idx; //then shift to location r
//		//	//std::cout << static_cast<int16_t>(cq) << std::endl;
//		//	rs = rs ^ cq; 
//		//	idx++;
//		//std::cout << std::endl;
//	}
//	//	std::cout << "rs: " << static_cast<int16_t>(rs) << std::endl;
//	return rs;
//}
