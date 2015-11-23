#include "base/callback.hh"
#include "base/intmath.hh"
#include "debug/Predictor.hh"
#include "debug/FreeList.hh"
#include "debug/DebugInfo.hh"
#include "base/trace.hh"
#include "cpu/pred/gshare.hh"

GshareBP::GshareBP(unsigned int _gshareHistoryBits, 
				   unsigned int _gshareHistoryTableSize,
				   unsigned int _gshareGlobalCtrBits)
		:		   gshareHistoryBits(_gshareHistoryBits),
				   gshareHistoryTableSize(_gshareHistoryTableSize),
				   gshareGlobalCtrBits(_gshareGlobalCtrBits)
{
	globalCtrsTable.resize(gshareHistoryTableSize);

	for (int i = 0; i < gshareHistoryTableSize; ++i)
        globalCtrsTable[i].setBits(gshareGlobalCtrBits);

	globalHistMask = gshareHistoryTableSize-1;

	threshold = (1 << (gshareGlobalCtrBits - 1)) - 1;

	Callback* cb = new MakeCallback<GshareBP, &GshareBP::writeDebugInfo>(this);
	registerExitCallback(cb);
}

inline
unsigned int GshareBP::calGlobalHistIdx(Addr& addr)
{
	return ((addr >> instShiftAmt) & globalHistMask) ^ globalHistory;
}

void GshareBP::updateGlobalHistoryTaken()
{
	globalHistory = globalHistory << 1 | 1;
	globalHistory = globalHistory & globalHistMask;
}

void GshareBP::updateGlobalHistoryNotTaken()
{
	globalHistory = globalHistory << 1;
	globalHistory = globalHistory & globalHistMask;
}

void GshareBP::uncondBr(void*& bp_history)
{
	BPHistory* history = new BPHistory();
	history->gshareGlobalHist = globalHistory;
	history->gsharePred = true;
	bp_history = (void*)history;
}

void GshareBP::BTBUpdate(Addr& branch_addr, void*& bp_history)
{
}

bool GshareBP::lookup(Addr& branch_addr, void*& bp_history)
{
	auto& record = debugMap[branch_addr];
	record.count++;

	unsigned int globalCtrsTableIdx = calGlobalHistIdx(branch_addr);
	assert(globalCtrsTableIdx < globalCtrsTable.size());

	idxMap[globalCtrsTableIdx].insert(branch_addr);

	record.conflictSet = idxMap[globalCtrsTableIdx];

	bool gsharePrediction = globalCtrsTable[globalCtrsTableIdx].read() > threshold;

	BPHistory* history = new BPHistory();
	history->gsharePred = gsharePrediction;
	history->gshareGlobalHist = globalHistory;
	bp_history = (void*)history;

	return gsharePrediction;
}

void GshareBP::update(Addr& branch_addr, bool taken, void*& bp_history, bool squashed)
{
	if(!bp_history)		return;

	BPHistory* history = (BPHistory*)bp_history;
	updateDebugInfo(branch_addr, taken, bp_history);
		
	unsigned int globalCtrsTableIdx = calGlobalHistIdx(branch_addr);

	if(taken)
	{
		updateGlobalHistoryTaken();
		globalCtrsTable[globalCtrsTableIdx].increment();
	}
	else
	{
		updateGlobalHistoryNotTaken();
		globalCtrsTable[globalCtrsTableIdx].decrement();
	}

	delete history;
}

void GshareBP::updateDebugInfo(Addr& addr, bool taken, void*& bp_history)
{
	BPHistory* history = (BPHistory*)bp_history;

	if(debugMap.find(addr) == debugMap.end())
	{
		auto& record = debugMap[addr];
		record.unCondBr = true;
	}
	
	auto& record = debugMap[addr];
	if(record.unCondBr)		record.count++;

	if(taken == history->gsharePred)
	{
		record.hit++;
		if(taken)
		{
			record.hitTaken++;
		}
		else
		{
			record.hitNotTaken++;
		}
	}
	else
	{
		record.miss++;
		if(taken)
		{
			record.missTaken++;
		}
		else
		{
			record.missNotTaken++;
		}
	}

	if(taken)
	{
		record.taken++;
	}
	else
	{
		record.notTaken++;
	}	
}

void GshareBP::squash(void*& bp_history)
{
	if(!bp_history)		return;

	DPRINTF(FreeList, "In flight branch is squashed due to last branch misprediction\n");
	BPHistory* history = (BPHistory*)bp_history;
	delete history;

	/* TODO: if speculatively update history, we need to recover history */
}

void GshareBP::writeDebugInfo()
{
	for(auto it = debugMap.begin(); it != debugMap.end(); it++)
	{
		auto& addr = it->first;
		auto& record = it->second;
		if(record.unCondBr || record.count < 10 || (double)(record.hit)/(double)(record.count) > 0.8)	continue;

		DPRINTF(DebugInfo, "-------------------new record-----------------\n");
		
		record.printDebugInfo(addr);

		DPRINTF(DebugInfo, "-------------------record end-----------------\n");
	}
}
