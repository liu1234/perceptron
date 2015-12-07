#ifndef __CPU_O3_PERCEPTRON_PRED_HH__
#define __CPU_O3_PERCEPTRON_PRED_HH__

#include <vector>
#include <deque>
#include <map>
#include <set>

#include "base/types.hh"
#include "cpu/pred/bpdebugger.hh"

/*
class PerceptronBPBase
{
	PerceptronBP(unsigned perceptronPredictorSize,
				 unsigned perceptronHistoryBits) {}
public:
	bool lookup(Addr& branch_addr, void*& bp_history) = 0;
	void update(Addr& branch_addr, bool taken, void*& bp_history, bool squashed) = 0;
	void squash(Addr& branch_addr, void*& bp_history) = 0;

};
*/

class PerceptronBP {

public:
	PerceptronBP(unsigned _perceptronPredictorSize,
				 unsigned _perceptronHistoryBits,
				 unsigned _instShiftAmt,
				 unsigned long _branchAddr);

	bool lookup(Addr& branch_addr, void*& bp_history);
	void update(Addr& branch_addr, bool taken, void*& bp_history, bool squashed);
	void squash(void*& bp_history);
	void BTBUpdate(Addr& branch_addr, void*& bp_history);
	void uncondBr(void*& bp_history);
	

protected:
	unsigned int calHistIdx(Addr& addr);
	void updateGlobalHistoryTaken(Addr&);
	void updateGlobalHistoryNotTaken(Addr&);
	
	typedef std::deque<bool> HistRegister;
	typedef std::vector<int> WeightVector;

	struct BPHistory {
		HistRegister globalHistory;
		int perceptronOutput;
		bool perceptronPredTaken;
	};

	HistRegister globalHistory;
	std::vector<WeightVector> perceptronTable;

	unsigned perceptronPredictorSize;
	unsigned perceptronHistoryBits;

	unsigned int threshold;
	unsigned int instShiftAmt;

	std::map<Addr, unsigned int> addrMap;
	std::map<unsigned int, std::set<Addr>> idxMap;

	// This is for debugging
/*	typedef struct
	{
		unsigned int count;

		unsigned int threshold;

		unsigned int squashed;
		
		unsigned int hit;
		// total weight when branch predicts correct, this value should be high
		unsigned int hitTaken;
		int hitTakenWeight;
		unsigned int hitNotTaken;
		int hitNotTakenWeight;

		// These three value could potentially reveal the reason for the misprediction
		unsigned int miss;
		// total weight when branch predicts wrong
		unsigned int missTaken;
		// total weight when branch predicts wrong(should be taken) 
		int missTakenWeight;
		unsigned int missNotTaken;
		// total weight when branch predicts wrong(should be not taken) 		
		int missNotTakenWeight;

		unsigned int taken;
		int takenWeight;
		unsigned int notTaken;
		int notTakenWeight;

		std::map<std::deque<bool>, int> histPattern;
		unsigned int coflict;
		std::set<Addr> conflictSet;

		bool unCondBr;

		bool stablized;

	}DebugInfo;
*/

	Addr debugAddr;
	std::map<Addr, DebugInfo<HistRegister>> debugMap;

	void updateDebugInfo(Addr&, bool, void*&);
//	void printoutStats(Addr&, DebugInfo<HistRegister>&);
	void writeDebugInfo();

	typedef std::deque<Addr> HistAddress;
	typedef struct
	{
		HistRegister globalHistory;
		HistAddress historyAddr;

//		unsigned int size() {return globalHistory.size()};
//		bool operator[](int idx) {return globalHistory[idx];}
	}DebugGlobalHist;

	DebugGlobalHist debugGlobalHist;
//	unsigned int weightBits;

};

#endif
