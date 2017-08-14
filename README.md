# Programmable Oblivious PRF & multi-party PSI
This is the implementation of our [CCS 2017](http://dl.acm.org/xxx)  paper: **Practical Multi-party Private Set Intersection from Symmetric-Key Techniques**[[ePrint](https://eprint.iacr.org/2017/xxx)]. 

Evaluating on a single Intel Xeon server (`2 36-cores Intel Xeon CPU E5-2699 v3 @ 2.30GHz and 256GB of RAM`), ours protocol requires only `71` seconds to securely compute the intersection of `5` parties, each has `2^20`-size sets, regardless of the bit length of the items.

For programmable OPRF, this code implements:
* Table-based OPPRF
* Polynomial-based  OPPRF
* BloomFilter-based OPPRF

For PSI, we implement 2-party PSI (2PSI) and multi-party PSI (nPSI) in augmented-semihonest model and standard semihonest model.

## Installations

### Required libraries
 C++ compiler with C++14 support. There are several library dependencies including [`Boost`](https://sourceforge.net/projects/boost/), [`Crypto++`](http://www.cryptopp.com/), [`Miracl`](https://github.com/miracl/MIRACL), [`Mpir`](http://mpir.org/), [`NTL`](http://www.shoup.net/ntl/) , and [`libOTe`](https://github.com/osu-crypto/libOTe). For [`libOTe`], it requires CPU supporting `PCLMUL`, `AES-NI`, and `SSE4.1`. Optional: `nasm` for improved SHA1 performance.   Our code has been tested on both Windows (Microsoft Visual Studio) and Linux. To install the required libraries: 
  * windows: open PowerShell,  `cd ./thirdparty`, and `.\all_win.ps1` 
  * linux: `cd ./thirdparty`, and `bash .\all_linux.get`.   
  
  
### Building the Project
After cloning project from git,
##### Windows:
1. build cryptoTools,libOTe, and libOPRF projects in order.
2. add argument for bOPRFmain project (for example: -t)
3. run bOPRFmain project
 
##### Linux:
1. make (requirements: `CMake`, `Make`, `g++` or similar)
2. for test:
	./Release/bOPRFmain.exe -t


## Running the code
The database is generated randomly. The outputs include the average online/offline/total runtime that displayed on the screen and output.txt. 
#### Flags:
    -u      unit test which computes PSI of 5 paries, each contains a set of size 2^8 in semihonest setting
	-n		number of parties
	-t      number of corrupted parties
	-m 		set size
	-a      run in augmented semihonest model. Table-based OPPRF is by default.
	-o      indicates which OPPRF protocol chosen. Requires -a be set. Table-based OPPRF is by default. 
	        0: Table-based; 1: POLY-seperated; 2-POLY-combined; 3-BloomFilter
#### Examples: 
##### 1. Unit test:
	./bOPRFmain.exe -u
	
##### 2. two-party PSI:
Compute PSI of 2 parties, each holds 2^8 items

	./bOPRFmain.exe -n 2 -m 8
	
##### 3. nPSI:
Compute PSI of 5 parties, 2 dishonestly colluding, each with set size 2^8 in semihonest setting

	./bOPRFmain.exe -n 5 -t 2 -n 8 
	
Compute PSI of 5 parties, 2 dishonestly colluding, each with set size 2^8 in augmented semihonest setting with Bloom filter based OPPRF

	./bOPRFmain.exe -n 5 -t 2 -n 8 -a 1 -o 3

	
## Help
For any questions on building or running the library, please contact [`Ni Trieu`](http://people.oregonstate.edu/~trieun/) at trieun at oregonstate dot edu
