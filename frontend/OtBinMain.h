#pragma once





//void OPPRFRecv();
//void OPPRFSend();
//void BarkOPRFRecv();
//void BarkOPRSend();
void Channel_test();
void OPPRF3_EmptrySet_Test_Main();
void OPPRFn_EmptrySet_Test_Main();
void OPPRF2_EmptrySet_Test_Main();
void Bit_Position_Random_Test();
void OPPRFnt_EmptrySet_Test_Main();
void party3(u64 myIdx, u64 setSize, u64 nTrials);
void party(u64 myIdx, u64 nParties, u64 setSize, std::vector<block>& mSet);
void tparty(u64 myIdx, u64 nParties, u64 tParties, u64 setSize, u64 nTrials);
//void OPPRFn_EmptrySet_Test();