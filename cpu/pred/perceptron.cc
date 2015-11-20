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

	DPRINTF(Predictor, "Perceptron branch predictor threshold: %i\n", threshold);
}

inline
unsigned int PerceptronBP::calHistIdx(Addr& addr)
{
	return (addr >> instShiftAmt) & (perceptronPredictorSize-1);
}

inline
void PerceptronBP::updateGlobalHistoryTaken(Addr& addr)
{	
	globalHistory.push_back(1);
	if(globalHistory.size() > perceptronHistoryBits)		globalHistory.pop_front();

	// This is for debug
	debugGlobalHist.historyAddr.push_back(addr);
	if(debugGlobalHist.historyAddr.size() > perceptronHistoryBits)		debugGlobalHist.historyAddr.pop_front();
}

inline
void PerceptronBP::updateGlobalHistoryNotTaken(Addr& addr)
{
	globalHistory.push_back(0);
	if(globalHistory.size() > perceptronHistoryBits)		globalHistory.pop_front();

	// This is for debug
	debugGlobalHist.historyAddr.push_back(addr);
	if(debugGlobalHist.historyAddr.size() > perceptronHistoryBits)		debugGlobalHist.historyAddr.pop_front();
}

bool PerceptronBP::lookup(Addr& branch_addr, void*& bp_history)
{
//	DPRINTF(Predictor, "PerceptronBP looking up addr %s\n", branch_addr);

	auto& record = debugMap[branch_addr];
	record.count++;
	
	unsigned int perceptronTableIdx = calHistIdx(branch_addr);
	assert(perceptronTableIdx < perceptronTable.size());	

	idxMap[perceptronTableIdx].insert(branch_addr);

	record.conflictSet = idxMap[perceptronTableIdx];

	if(record.conflictSet.size() > 1)
	{
//		DPRINTF(Predictor, "This address conflicts with other %i addresses\n", conflictSet.size()-1);
	}

	WeightVector& weightVec = perceptronTable[perceptronTableIdx];

	//unsigned int maxWeightIdx = 1;
	//DPRINTF(Predictor, "Global history 1 length: %i\n", globalHistory.size());
	auto histLen = globalHistory.size();
	int result = weightVec[0];
	for(int i = 0; i < histLen; i++)
	{
		if(globalHistory[histLen-i-1])
		{
			result += weightVec[i+1];
		}
		else
		{
			result -= weightVec[i+1];
		}

		/*if(conflictSet.size() > 1 && (conflictSet.find(debugGlobalHist.historyAddr[histLen-i-1]) != conflictSet.end()) && debugGlobalHist.historyAddr[histLen-i-1] != branch_addr)*/
		//{
			//DPRINTF(Predictor, "Conflict Addr %s also in the history, bad things can happen\n", debugGlobalHist.historyAddr[histLen-i-1]);
		//}
		/*DPRINTF(Predictor, "Correlation factor with #%i history %i is: %i\n", histLen-i-1, globalHistory[histLen-i-1], weightVec[i+1]);*/
	}

//	DPRINTF(Predictor, "Branch bias itself is %i\n", weightVec[0]);
	bool predTaken = (result > 0) ? true : false;

	if(predTaken)	
	{
		record.taken++;
		record.takenWeight += result;
	}
	else			
	{
		record.notTaken++;
		record.notTakenWeight += result;
	}

//	DPRINTF(Predictor, "PerceptronBP result: %i, predict: %i\n", result, predTaken);
	BPHistory* history = new BPHistory();
	history->globalHistory = globalHistory;
	history->perceptronOutput = result;
	history->perceptronPredTaken = predTaken;
	bp_history = (void*)history;

	/* TODO: speculatively update history to support more in-flight branches */

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
//	DPRINTF(Predictor, "Perceptron updating %s\n", branch_addr);
	
	if(!bp_history)		return;

	if(debugMap.find(branch_addr) == debugMap.end())
	{
		auto& record = debugMap[branch_addr];
		record.count = 1;
		record.unCondBr = true;
		//panic("non-existing address needs update\n");
		//return;
	}
	
	auto& record = debugMap[branch_addr];

	unsigned int perceptronTableIdx = calHistIdx(branch_addr);

	assert(perceptronTableIdx < perceptronTable.size());
	WeightVector& weightVec = perceptronTable[perceptronTableIdx];

	BPHistory* history = (BPHistory*)bp_history;

	if(taken != history->perceptronPredTaken || abs(history->perceptronOutput) <= threshold)
	{
		bool mispredict = false;
		if(taken != history->perceptronPredTaken)
		{
//			DPRINTF(Predictor, "update due to misprediction\n");
			record.miss++;
			mispredict = true;
			if(record.stablized)
			{
				DPRINTF(Predictor, "%s is an unstable branch\n", branch_addr);
			}
		}
		else
		{
			record.hit++;
//			DPRINTF(Predictor, "update due to threshold\n");
		}
		if(taken)		
		{
			weightVec[0] += 1;
			if(mispredict)
			{
				record.missTaken++;
				record.missTakenWeight += history->perceptronOutput;
			}
			else
			{
				record.hitTaken++;
				record.hitTakenWeight += history->perceptronOutput;
			}
		}
		else			
		{
			weightVec[0] -= 1;
			if(mispredict)
			{
				record.missNotTaken++;
				record.missNotTakenWeight += history->perceptronOutput;
			}
			else
			{
				record.hitNotTaken++;
				record.hitNotTakenWeight += history->perceptronOutput;
			}
		}
		auto histLen = globalHistory.size();
		for(int i = 0; i < histLen; i++)
		{
			if(taken == globalHistory[histLen-i-1])		weightVec[i+1] += 1;
			else										weightVec[i+1] -= 1;
		}
		
	}

	else
	{
//		DPRINTF(Predictor, "%s looks like stablized\n", branch_addr);
		/* TODO: can still mispredict after stablized */
		if(!record.stablized)
		{
			printoutStats(branch_addr, record);
			record.stablized = true;
		}
	}
	/* TODO: this can be done speculatively when looking up */

	if(taken)
	{
		updateGlobalHistoryTaken(branch_addr);

	}
	else
	{
		updateGlobalHistoryNotTaken(branch_addr);
	}

//	DPRINTF(Predictor, "Update global history %s to %i\n", branch_addr, taken);
	//DPRINTF(Predictor, "Global history 2 length: %i\n", globalHistory.size());
	
	delete history;

}

void PerceptronBP::squash(void*& bp_history)
{
	BPHistory* history = (BPHistory*)bp_history;
	delete history;

	/* TODO: if speculatively update history, we need to recover history */
}

void PerceptronBP::printoutStats(Addr& addr, DebugInfo& record)
{
	if(record.unCondBr)
	{
		DPRINTF(Predictor, "This is an unconditional branch, should be always correct, ignore it\n");
		return;
	}
	DPRINTF(Predictor, "-------------------new record-----------------\n");
	DPRINTF(Predictor, "%s has been executed for %i times and stablized\n", addr, record.count);
	//DPRINTF(Predictor, "After %i times, it stablized(seemingly)\n", record.threshold);
	DPRINTF(Predictor, "Branch Hit times: %i; Miss times: %i\n", record.hit, record.miss);

	DPRINTF(Predictor, "In branch correct prediction, %i is taken\n", record.hitTaken);
	DPRINTF(Predictor, "Avg weight for this prediction is %i\n", record.hitTakenWeight/(int)(record.hitTaken+1));
	DPRINTF(Predictor, "In branch correct prediction, %i is not taken\n", record.hitNotTaken);
	DPRINTF(Predictor, "Avg weight for this prediction is %i\n", record.hitNotTakenWeight/(int)(record.hitNotTaken+1));

	if(record.miss > 0)
	{
		DPRINTF(Predictor, "In branch missprediction, %i should be taken but predicted as not taken\n",record.missTaken);
		DPRINTF(Predictor, "Avg weight for this missprediction is %i\n",record.missTakenWeight/(int)(record.missTaken+1));
		DPRINTF(Predictor, "In branch missprediction, %i should be not taken but predicted as taken\n",record.missNotTaken);
		DPRINTF(Predictor, "Avg weight for this missprediction is %i\n",record.missNotTakenWeight/(int)(record.missNotTaken+1));
	}

	DPRINTF(Predictor, "Branch taken %i times avg weight: %i; not taken %i times avg weight: %i\n", record.taken, record.takenWeight/(int)(record.taken+1), record.notTaken, record.notTakenWeight/(int)(record.notTaken+1));

	DPRINTF(Predictor, "-------------------record end-----------------\n");
}
