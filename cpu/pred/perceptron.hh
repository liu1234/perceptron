#ifndef __CPU_O3_PERCEPTRON_PRED_HH__
#define __CPU_O3_PERCEPTRON_PRED_HH__

#include <vector>
#include <deque>
#include <map>
#include <set>

#include "base/types.hh"
#include "cpu/o3/sat_counter.hh"

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
	PerceptronBP(unsigned perceptronPredictorSize,
				 unsigned perceptronHistoryBits);

	bool lookup(Addr& branch_addr, void*& bp_history);
	void update(Addr& branch_addr, bool taken, void*& bp_history, bool squashed);
	void squash(void*& bp_history);
	void BTBUpdate(Addr& branch_addr, void*& bp_history);
	void uncondBr(void*& bp_history);
	

protected:
	unsigned int calHistIdx(Addr& addr);
	void updateGlobalHistoryTaken();
	void updateGlobalHistoryNotTaken();
	
	typedef std::vector<bool> PerceptronUnit;
	typedef std::vector<int> WeightVector;

	struct BPHistory {
		PerceptronUnit globalHistory;
		int perceptronOutput;
		bool perceptronPredTaken;
	};
	
	PerceptronUnit globalHistory;
	PerceptronUnit globalHistoryMask;

	std::vector<WeightVector> perceptronTable;
	unsigned perceptronPredictorSize;
	unsigned perceptronHistoryBits;

	unsigned int threshold;
	unsigned int instShiftAmt;

	std::deque<Addr> globalHistAddr;

	std::map<Addr, unsigned int> addrMap;
	std::map<unsigned int, std::set<Addr>> idxMap;
//	unsigned int weightBits;

};

#endif
