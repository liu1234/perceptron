#include "base/intmath.hh"
#include "cpu/pred/perceptron.hh"
#include "debug/Predictor.hh"
#include "base/trace.hh"
#include <cmath>

PerceptronBP::PerceptronBP(unsigned _perceptronPredictorSize,
						   unsigned _perceptronHistoryBits)
: perceptronPredictorSize(_perceptronPredictorSize),
  perceptronHistoryBits(_perceptronHistoryBits)
{
	perceptronTable.resize(perceptronPredictorSize);

	for(int i = 0; i < perceptronPredictorSize; i++)
		perceptronTable[i].resize(perceptronHistoryBits+1);

	//globalHistory.resize(perceptronHistoryBits-1);

	threshold = 1.93*perceptronHistoryBits+14;
}

inline
unsigned int PerceptronBP::calHistIdx(Addr& addr)
{
	return (addr >> instShiftAmt) & (perceptronPredictorSize-1);
}

inline
void PerceptronBP::updateGlobalHistoryTaken()
{
	PerceptronUnit temp;
	temp.push_back(1);
	temp.insert(temp.begin()+1, globalHistory.begin(), globalHistory.end());
	if(temp.size() > perceptronHistoryBits)		temp.pop_back();
	globalHistory = temp;
}

inline
void PerceptronBP::updateGlobalHistoryNotTaken()
{
	PerceptronUnit temp;
	temp.push_back(0);
	temp.insert(temp.begin()+1, globalHistory.begin(), globalHistory.end());
	if(temp.size() > perceptronHistoryBits)		temp.pop_back();
	globalHistory = temp;
}

bool PerceptronBP::lookup(Addr& branch_addr, void*& bp_history)
{
	DPRINTF(Predictor, "PerceptronBP looking up addr %s\n", branch_addr);

	bool addrConflict = false;
	auto addrIt = addrMap.find(branch_addr);
	if(addrIt != addrMap.end())
	{
		DPRINTF(Predictor, "Addr has already been looked up for %i times\n", addrIt->second);
		addrConflict = true;
	}
	else
	{
		DPRINTF(Predictor, "This is a new address\n");
		addrMap[branch_addr]++;
	}

	unsigned int perceptronTableIdx = calHistIdx(branch_addr);

	auto idxIt = idxMap.find(perceptronTableIdx);
	if(idxIt != idxMap.end() && !addrConflict)
	{
		DPRINTF(Predictor, "Oops.. New addr map to the same perceptron unit, WATCH OUT\n");
		DPRINTF(Predictor, "Conflict addr has %i: %s\n", (idxIt->second).size(), *((idxIt->second).begin()));
	}
	else if(idxIt == idxMap.end())
	{
		idxMap[perceptronTableIdx].insert(branch_addr);
	}

	assert(perceptronTableIdx < perceptronTable.size());
	WeightVector& weightVec = perceptronTable[perceptronTableIdx];

	//unsigned int maxWeightIdx = 1;
	//DPRINTF(Predictor, "Global history 1 length: %i\n", globalHistory.size());
	int result = weightVec[0];
	for(int i = 0; i < globalHistory.size(); i++)
	{
		if(globalHistory[i])
		{
			result += weightVec[i+1];
		}
		else
		{
			result -= weightVec[i+1];
		}
		DPRINTF(Predictor, "Correlation factor with #%i history %i is: %i\n", i, globalHistory[i], weightVec[i+1]);
	}

	DPRINTF(Predictor, "Branch bias itself is %i\n", weightVec[0]);
	bool predTaken = (result > 0) ? true : false;

	DPRINTF(Predictor, "PerceptronBP result: %i, predict: %i\n", result, predTaken);
	BPHistory* history = new BPHistory();
	history->globalHistory = globalHistory;
	history->perceptronOutput = result;
	history->perceptronPredTaken = predTaken;
	bp_history = (void*)history;

	return predTaken;
}

void PerceptronBP::uncondBr(void*& bp_history)
{
	BPHistory* history = new BPHistory();
	history->globalHistory = globalHistory;
	history->perceptronOutput = threshold+1;
	history->perceptronPredTaken = true;
	bp_history = (void*)history;
}

void PerceptronBP::BTBUpdate(Addr& branch_addr, void*& bp_history)
{}

void PerceptronBP::update(Addr& branch_addr, bool taken, void*& bp_history, bool squashed)
{
	DPRINTF(Predictor, "Perceptron updating %s\n", branch_addr);

	unsigned int perceptronTableIdx = calHistIdx(branch_addr);

	assert(perceptronTableIdx < perceptronTable.size());
	WeightVector& weightVec = perceptronTable[perceptronTableIdx];

	if(!bp_history)		return;

	BPHistory* history = (BPHistory*)bp_history;

	if(taken != history->perceptronPredTaken || abs(history->perceptronOutput) <= threshold)
	{
		if(taken != history->perceptronPredTaken)
		{
			DPRINTF(Predictor, "update due to misprediction\n");
		}
		else
		{
			DPRINTF(Predictor, "update due to threshold\n");
		}
		if(taken)		weightVec[0] += 1;
		else			weightVec[0] -= 1;

		for(int i = 0; i < globalHistory.size(); i++)
		{
			if(taken == globalHistory[i])		weightVec[i+1] += 1;
			else								weightVec[i+1] -= 1;
		}
		
	}
	
	if(taken)
	{
		updateGlobalHistoryTaken();
	}
	else
	{
		updateGlobalHistoryNotTaken();
	}

	DPRINTF(Predictor, "Update global history %s to %i\n", branch_addr, taken);
	//DPRINTF(Predictor, "Global history 2 length: %i\n", globalHistory.size());
	
//	globalHistAddr.pop_front();
//	globalHistAddr.push_back(branch_addr);

	delete history;

}

void PerceptronBP::squash(void*& bp_history)
{
	BPHistory* history = (BPHistory*)bp_history;
	delete history;
}

