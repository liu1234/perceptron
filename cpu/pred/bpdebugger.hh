#ifndef __CPU_O3_BPDEBUGGER_PRED_HH__
#define __CPU_O3_BPDEBUGGER_PRED_HH__

#include <map>
#include <vector>
#include <set>

#include "base/types.hh"
#include "cpu/pred/perceptron.hh"
#include "cpu/pred/gshare.hh"


class DebugInfo
{
	friend class PerceptronBP;
	friend class GshareBP;

	void printDebugInfo(const Addr&);
private:
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

	std::map<std::deque<bool>, unsigned int> histPattern;

	unsigned int coflict;
	std::set<Addr> conflictSet;

	bool unCondBr;

	bool stablized;

};

#endif
