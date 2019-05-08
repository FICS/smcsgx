# Hybrid Secure Function Evaluation (SFE) using Garbled Circuits and SGX

### Developed by researchers from the Florida Institute for Cybersecurity Research (FICS Research)

The goal of secure function evaluation (SFE), sometimes referred to as secure multiparty computation (SMC), is to enable multiple parties to compute and receive the output of a function without having each party's private input exposed to any other party.

We propose a hybrid two-party SFE (2P-SFE) scheme where the function is partitioned into a sequence of round functions. Some of these rounds functions are evaluated within an SGX enclave while others are evaluated using a garbled circuit.

Our protocols involve two parties: the sender (Alice) and the evaluator (Bob).

*  Evaluator implementation is comprised of: State Machine + Socket + Enclave + SGX Remote Attestation + YAO (garbled circuits)
*  Sender implementation is comprised of: State Machine + Socket + Enclave + YAO

#####Setup Notes
1. Use "#if defined(\_MSC\_VER)" for Windows compilation
2. Use "#elif defined(\_\_GNUC\_\_)" for Linux gcc/make compilation
3. All code was developed for Linux
4. Make sure to disable UT mode in both app files for real testing
5. Make sure to disable simulation mode in the Makefile for hardware run
6. Use "\_\_INTEL\_COMPILER" for Intel compilers
7. Make sure YAO is built
8. Make sure CCS.params is present in the top-level directory
9. Private key files (.pem) in enclave subdirectories were pulled from the Intel SGX SDK sample code and left for completeness
10. Run ``make`` within the sample\_libcrypto subdirectory at first to generate **libsample\_libcrypto.so**

#####Repository Contents
Sub-directories:

* **common**: code common to both evaluator's and sender's application
* eval\_app and eval\_enclave: application and enclave code for Bob
* **include**: various header files to be included
* **input**: contains input for the test cases used in our evaluation
* **log**: example logs leftover from testing
* **nonsfe**: early artifacts (naive); C++ versions of the case study applications, later integrated into application/enclave code
* **nonsfe_v2**: same as above, but with changes made to fit the hybrid protocol
* **perf\_eval**: contains template benchmarking scripts used by deploy.sh
* **sample\_libcrypto**: contains sample crypto library APIs; pulled from Intel SGX SDK <https://github.com/intel/linux-sgx/tree/master/sdk/sample_libcrypto> (Linux 2.5 Open Source Gold Release)
* **send\_app** and **send\_enclave**: application and enclave code for Alice
* **sfe**: early artifacts (with side-channel mitigations); C++ versions of the case study applications, later integrated into application/enclave code
* **sfe_v2**: same as above, but with changes made to fit the hybrid protocol
* **yao**: source files used to build Yao objects for garbled circuit evaluation rounds

Top-level files:

* **build_runs.sh**: script for generating runs of all or specific test programs (millionaires, Dijkstra, or database); will run `make clean` and `make`
* **CCS.params**: needed in top-level directory for compilation of YAO
* **deploy.sh**: deploy script for making static builds of run configurations; `make` target and also used by build\_runs.sh.
* **HOWTO.txt**: commands and parameters for running specific applications
* **inp.txt**: likely an artifact of early testing of GC implementation
* **Makefile**
* **README.md**

#####Building/Running the Code
Build using the build\_runs.sh script, which will call `make`. Once built, refer to the included __HOWTO.txt__ file for invocation instructions.

#####Frigate Dependencies
We adapt Frigate (garbled circuit compiler and execution environment) for evaluation of round functions within garbled circuits. Successful compilation of the Frigate component (YAO) may be best achieved on a system with Bison version 2.7.1 and Flex version 2.5.37. Frigate is available at <https://bitbucket.org/bmood/frigaterelease>.

#####AsiaCCS'19 paper citation
This repository contains the implementation of the hybrid protocols presented in our AsiaCCS'19 paper. The full BibTeX citation for our paper is as follows:

```
@inproceedings{smcsgx19,
    author = {Choi, Joseph I. and Tian, Dave (Jing) and Hernandez, Grant and Patton, Christopher and Mood, Benjamin and Shrimpton, Thomas and Butler, Kevin R. B. and Traynor, Patrick},
    title = {{A Hybrid Approach to Secure Function Evaluation using SGX}},
    booktitle = {Proceedings of the ACM Asia Conference on Computer and Communications Security (AsiaCCS'19)},
    year = 2019}
```

Our full version, with proofs, is available on arXiv: <https://arxiv.org/abs/1905.01233>.