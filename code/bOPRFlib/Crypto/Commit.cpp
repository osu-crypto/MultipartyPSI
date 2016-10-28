
#include "Commit.h"
#include "PRNG.h"

namespace bOPRF {

	//void CommitComm(ByteStream& comm, ByteStream& open, const ByteStream& data, PRNG& prng)
	//{
	//	open = data;
	//	open.append(prng.get_block());

	//	Commit commit(open.data(), open.size());
	//	comm.append(commit.data(), commit.size());
	//}

	//void CommitComm(ByteStream& comm, const block& rand, const block& data)
	//{
	//	comm.setp(0);
	//	comm.append(ByteArray(data), sizeof(block));
	//	comm.append(ByteArray(rand), sizeof(block));
	//	comm = comm.hash();
	//}

	//bool CommitOpen(ByteStream& data, const ByteStream& comm, const ByteStream& open)
	//{
	//	ByteStream h = open.hash();
	//	if (!h.equals(comm)) { return false; }
	//	data = open;
	//	data.setp(data.tellp() - SEED_SIZE);
	//	return true;
	//}

	//bool Commitopen(const ByteStream& comm, const block& rand, const block& data)
	//{
	//	ByteStream h;// = open.hash();
	//	h.setp(0);
	//	h.append(ByteArray(data), sizeof(block));
	//	h.append(ByteArray(rand), sizeof(block));
	//	h = h.hash();

	//	if (!h.equals(comm)) { return false; }
	//	return true;
	//}

}
