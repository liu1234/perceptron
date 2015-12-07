#include "base/callback.hh"
#include "base/intmath.hh"
#include "cpu/pred/perceptron.hh"
#include "debug/Predictor.hh"
#include "debug/FreeList.hh"
#include "debug/DebugInfo.hh"
#include "base/trace.hh"
#include <cmath>
#include <fstream>

PerceptronBP::PerceptronBP(unsigned _perceptronPredictorSize,
						   unsigned _perceptronHistoryBits,
						   unsigned _instShiftAmt,
						   unsigned long _branchAddr)
: perceptronPredictorSize(_perceptronPredictorSize),
  perceptronHistoryBits(_perceptronHistoryBits),
  instShiftAmt(_instShiftAmt),
  debugAddr(_branchAddr)
{
	perceptronTable.resize(perceptronPredictorSize);

	for(int i = 0; i < perceptronPredictorSize; i++)
		perceptronTable[i].resize(perceptronHistoryBits+1);

	//globalHistory.resize(perceptronHistoryBits-1);

	threshold = 1.93*perceptronHistoryBits+14;

	DPRINTF(Predictor, "Perceptron branch predictor threshold: %i\n", threshold);
	DPRINTF(Predictor, "Tracking branch: %x\n", debugAddr);

	Callback* cb = new MakeCallback<PerceptronBP, &PerceptronBP::writeDebugInfo>(this);
	registerExitCallback(cb);
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
		//DPRINTF(Predictor, "Correlation factor with #%i history %i is: %i\n", histLen-i-1, globalHistory[histLen-i-1], weightVec[i+1]);
	}

	bool predTaken = (result > 0) ? true : false;

	DebugInfo<HistRegister>::Correlation* cptr = new DebugInfo<HistRegister>::Correlation();
	for(int i = 1; i < histLen+1; i++)
	{
		cptr->histVec.emplace_back(debugGlobalHist.historyAddr[histLen-i], weightVec[i], globalHistory[histLen-i]);
	}
	cptr->pattern = globalHistory;
	cptr->result = predTaken;
	record.correlator.push_back(cptr);

//	DPRINTF(Predictor, "Branch bias itself is %i\n", weightVec[0]);
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
{
}

void PerceptronBP::update(Addr& branch_addr, bool taken, void*& bp_history, bool squashed)
{
//	DPRINTF(Predictor, "Perceptron updating %s\n", branch_addr);
	
	if(!bp_history)		return;

	// update debug info
	
	updateDebugInfo(branch_addr, taken, bp_history);

	unsigned int perceptronTableIdx = calHistIdx(branch_addr);

	assert(perceptronTableIdx < perceptronTable.size());
	WeightVector& weightVec = perceptronTable[perceptronTableIdx];

	BPHistory* history = (BPHistory*)bp_history;
	auto& histQueue = history->globalHistory;

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

		auto histLen = globalHistory.size();
		for(int i = 0; i < histLen; i++)
		{
			if(taken == histQueue[histLen-i-1])		weightVec[i+1] += 1;
			else									weightVec[i+1] -= 1;
		}
		
	}

	else
	{
//		DPRINTF(Predictor, "%s looks like stablized\n", branch_addr);
		/* TODO: can still mispredict after stablized */
		//if(!record.stablized)
		//{
			//printoutStats(branch_addr, record);
			//record.stablized = true;
		//}
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
	if(!bp_history)		return;

	DPRINTF(FreeList, "In flight branch is squashed due to last branch misprediction\n");
	BPHistory* history = (BPHistory*)bp_history;
	delete history;

	/* TODO: if speculatively update history, we need to recover history */
}

void PerceptronBP::updateDebugInfo(Addr& addr, bool taken, void*& bp_history)
{
	BPHistory* history = (BPHistory*)bp_history;

	if(debugMap.find(addr) == debugMap.end())
	{
		auto& record = debugMap[addr];
		record.unCondBr = true;
	}
	
	auto& record = debugMap[addr];
	if(record.unCondBr)		record.count++;
	record.histPattern[history->globalHistory]++;
	record.globalCount++;

	if(taken == history->perceptronPredTaken)
	{
		record.hit++;
		if(taken)
		{
			record.hitTaken++;
			record.hitTakenWeight += history->perceptronOutput;
		}
		else
		{
			record.hitNotTaken++;
			record.hitNotTakenWeight += history->perceptronOutput;
		}
	}
	else
	{
		record.miss++;
		if(taken)
		{
			record.missTaken++;
			record.missTakenWeight += history->perceptronOutput;
		}
		else
		{
			record.missNotTaken++;
			record.missNotTakenWeight += history->perceptronOutput;
		}
	}

	if(taken)
	{
		record.taken++;
		record.takenWeight += history->perceptronOutput;
		record.histTaken[history->globalHistory]++;
		
	}
	else
	{
		record.notTaken++;
		record.notTakenWeight += history->perceptronOutput;
	}
}

/*
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
*/

void PerceptronBP::writeDebugInfo()
{
	std::ofstream file_all("/home/min/a/liu1234/gem5/log.txt");
	std::ofstream file_single("/home/min/a/liu1234/gem5/single.txt");
	
	for(auto it = debugMap.begin(); it != debugMap.end(); it++)
	{
		auto& addr = it->first;
		auto& record = it->second;
		if(addr == debugAddr)
		{
			DPRINTF(DebugInfo, "-------Traced One-------\n");
			record.printCorrelationInfo(file_single, addr);
		}
		if(record.unCondBr || record.count < 10 || (double)(record.hit)/(double)(record.count) > 0.8)	continue;
		//if(record.unCondBr || record.count < 10)	continue;
		DPRINTF(DebugInfo, "-------------------new record-----------------\n");
		
		file_all << "Addr: " << addr << " hist pattern" << '\n';
		record.printDebugInfo(file_all, addr);
		
		DPRINTF(DebugInfo, "-------------------record end-----------------\n");
		file_all << '\n';
	}

	file_single.close();
	file_all.close();
}
