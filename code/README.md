# Batched Oblivious PRF
This is the implementation of our CCS 2016 paper: **Efficient Batched Oblivious PRF with Applications to Private Set Intersection**[[ePrint](https://...)]. Our code utilizes some parts of the [`libPSI`] (https://github.com/osu-crypto/libPSI) framework (OT extension) provided by [Peter Rindal](http://web.engr.oregonstate.edu/~rindalp/). We would like to thank Peter Rindal for contributing libraries and helpful suggestions to our protocol implementation. For any questions related to the implementation, please contact Ni Trieu at trieun at oregonstate dot edu

## Abstract
---
We describe a lightweight protocol for oblivious evaluation of a pseudorandom function (OPRF) in the presence of semi-honest adversaries. In an OPRF protocol a receiver has an input $r$; the sender gets output $s$ and the receiver gets output $F(s,r)$, where $F$ is a pseudorandom function and $s$ is a random seed. Our protocol uses a novel adaptation of 1-out-of-2 OT-extension protocols, and is particularly efficient when used to generate a large batch of OPRF instances. The cost to realize $m$ OPRF instances is roughly the cost to realize $3.5 m$ instances of standard 1-out-of-2 OTs (using state-of-the-art OT extension).

We explore in detail our protocol's application to semi-honest secure private set intersection (PSI). The fastest state-of-the-art PSI protocol (Pinkas et al., Usenix 2015) is based on efficient OT extension. We observe that our OPRF can be used to remove their PSI protocol's dependence on the bit-length of the parties' items. We implemented both PSI protocol variants and found ours to be 3.0--3.2x faster than Pinkas et al.\ for PSI of 128-bit strings and sufficiently large sets. Concretely, ours requires only 4.6 seconds to securely compute the intersection of $2^{20}$-size sets, regardless of the bitlength of the items. For very large sets, our protocol is only 5.2x slower than the {\em insecure} naive hashing approach for PSI.

## Installations
---
###Required libraries
  * [`Boost`](https://sourceforge.net/projects/boost/)
  * [`Crypto++`](http://www.cryptopp.com/)
  * [`Miracl`](https://github.com/miracl/MIRACL)
  * [`Mpir`](http://mpir.org/)
  
Please read [`libPSI`](https://github.com/osu-crypto/libPSI) for more detail about how to install the required libraries
### Building the Project
After cloning project from git,
##### Windows:
1. build pOPRFlib project
2. add argument for bOPRFmain project (for example: -t)
3. run bOPRFmain project
 
##### Linux:
1. make
2. for test:
	./Release/bOPRFmain.exe -t
	
### Test
Our database is generated randomly. We have 2 functions: 
#### 1. Unit Test: 
test PSI result for a small number of inputs (2^12), shows whether the program computes a right PSI. This test runs on one terminal:

	./bOPRFmain.exe -t
	
#### 2. Simulation: 
Using two terminals, compute PSI in 6 cases with the number of input (2^8, 2^12, 2^16, 2^18, 2^20, 2^24). For each case, we run the code 10 times to compute PSI. The outputs include the average online/offline/total runtime (displayed on the screen) and the output.txt file. Note that these parameters can be customized in the code.
On the Sender's terminal, run:

	./bOPRFmain.exe -r 0
	
On the Receiver's terminal, run:
	
	./bOPRFmain.exe -r 1
### NOTE
Currently, we implement bOPRF and PSI in the same module. We plan to separate the code for each problem shortly. We will also consider multi-threaded implementation in our future work.
## References
---
[1] 
[2]
