# Programmable Oblivious PRF & multi-party PSI
This is the implementation of our [CCS 2017](http://dl.acm.org/xxx)  paper: **Practical Multi-party Private Set Intersection from Symmetric-Key Techniques**[[ePrint](https://eprint.iacr.org/2017/xxx)]. 

Evaluating on a single Intel Xeon server (`2 36-cores Intel Xeon CPU E5-2699 v3 @ 2.30GHz and 256GB of RAM`), ours protocol requires only `71` seconds to securely compute the intersection of `5` parties, each has `2^20`-size sets, regardless of the bit length of the items.

For programmable OPRF, this code implements:
* Table-based OPPRF
* Polynomial-based  OPPRF
* BloomFilter-based OPPRF

For PSI, we implement multi-party PSI (nPSI) in augmented-semihonest model and standard semihonest model.

## Installations

### Required libraries
 C++ compiler with C++14 support. There are several library dependencies including [`Boost`](https://sourceforge.net/projects/boost/), [`Miracl`](https://github.com/miracl/MIRACL), [`NTL`](http://www.shoup.net/ntl/) , and [`libOTe`](https://github.com/osu-crypto/libOTe). For `libOTe`, it requires CPU supporting `PCLMUL`, `AES-NI`, and `SSE4.1`. Optional: `nasm` for improved SHA1 performance.   Our code has been tested on both Windows (Microsoft Visual Studio) and Linux. To install the required libraries: 
  * windows: open PowerShell,  `cd ./thirdparty`, and `.\all_win.ps1` (the script works with Visual Studio 2015. For other version, you should modify [`MSBuild`](https://github.com/osu-crypto/MultipartyPSI/blob/implement/thirdparty/win/getNTL.ps1#L3) at several places in the script.)
  * linux: `cd ./thirdparty`, and `bash .\all_linux.get`.   

NOTE: If you meet problem with `all_win.ps1` or `all_linux.get` which builds boost, miracl and libOTe, please follow the more manual instructions at [`libOTe`](https://github.com/osu-crypto/libOTe) 

### Building the Project
After cloning project from git,
##### Windows:
1. build cryptoTools,libOTe, and libOPRF projects in order.
2. add argument for bOPRFmain project (for example: -u)
3. run bOPRFmain project
 
##### Linux:
1. make (requirements: `CMake`, `Make`, `g++` or similar)
2. for test:
	./bin/frontend.exe -u


## Running the code
The database is generated randomly. The outputs include the average online/offline/total runtime that displayed on the screen and output.txt. 
#### Flags:
    -u		unit test which computes PSI of 5 paries, 2 dishonestly colluding, each with set size 2^12 in semihonest setting
	-n		number of parties
	-p		party ID
	-m		set size
	-t		number of corrupted parties (in semihonest setting)
	-a		run in augmented semihonest model. Table-based OPPRF is by default.
				0: Table-based; 1: POLY-seperated; 2-POLY-combined; 3-BloomFilter
	-r		optimized 3PSI when r = 1			
#### Examples: 
##### 1. Unit test:
	./bin/frontend.exe -u
	
##### 2. nPSI:
Compute PSI of 5 parties, 2 dishonestly colluding, each with set size 2^12 in semihonest setting

	./bin/frontend.exe -n 5 -t 2 -m 12 -p 0 
	& ./bin/frontend.exe -n 5 -t 2 -m 12 -p 1
	& ./bin/frontend.exe -n 5 -t 2 -m 12 -p 2
	& ./bin/frontend.exe -n 5 -t 2 -m 12 -p 3
	& ./bin/frontend.exe -n 5 -t 2 -m 12 -p 4
	
Compute PSI of 5 parties, each with set size 2^12 in augmented semihonest setting with Bloom filter based OPPRF. Note that, the augmented SH protocol protects from a collusion of n-1 parties

	./bin/frontend.exe -n 5 -a 3 -m 12 -p 0 
	& ./bin/frontend.exe -n 5 -a 3  -m 12 -p 1
    & ./bin/frontend.exe -n 5 -a 3  -m 12 -p 2
    & ./bin/frontend.exe -n 5 -a 3  -m 12 -p 3
    & ./bin/frontend.exe -n 5 -a 3  -m 12 -p 4
	
## Summary

      1. git clone https://github.com/osu-crypto/MultipartyPSI.git  
      2. cd thirdparty/
      3. bash all_linux.get 
      4. cd ..
      5. cmake .
      6.  make -j
      7. ./bin/frontend.exe -n 5 -t 2 -m 12 -p 0 & ./bin/frontend.exe -n 5 -t 2 -m 12 -p 1  & ./bin/frontend.exe -n 5 -t 2 -m 12 -p 2 & ./bin/frontend.exe -n 5 -t 2 -m 12 -p 3 & ./bin/frontend.exe -n 5 -t 2 -m 12 -p 4
 	
	
## Help
For any questions on building or running the library, please contact [`Ni Trieu`](http://people.oregonstate.edu/~trieun/) at trieun at oregonstate dot edu
