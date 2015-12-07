#ifndef _CPU_O3_GSHARE_PRED_HH
#define _CPU_O3_GSHARE_PRED_HH

#include <map>
#include <vector>
#include <set>

#include "base/types.hh"
#include "cpu/o3/sat_counter.hh"
#include "cpu/pred/bpdebugger.hh"

class GshareBP
{
public:
	GshareBP(unsigned int _gshareHistoryBits, unsigned int _gshareHistoryTableSize, unsigned int _gshareGlobalCtrBits, unsigned int _instShiftAmt);

	bool lookup(Addr& branch_addr, void*& bp_history);
	void update(Addr& branch_addr, bool taken, void*& bp_history, bool squashed);
	void squash(void*& bp_history);
	void BTBUpdate(Addr& branch_addr, void*& bp_history);
	void uncondBr(void*& bp_history);

protected:
	unsigned int calGlobalHistIdx(Addr& addr);
	void updateGlobalHistoryTaken();
	void updateGlobalHistoryNotTaken();

	struct BPHistory
	{
		bool gsharePred;
		unsigned int gshareGlobalHist;
	};

	unsigned int gshareHistoryBits;
	unsigned int gshareHistoryTableSize;
	unsigned int gshareGlobalCtrBits;

	unsigned int globalHistory;
	unsigned int globalHistMask;

	unsigned int threshold;
	int instShiftAmt;

    std::vector<SatCounter> globalCtrsTable;

	std::map<unsigned int, std::set<Addr>> idxMap;
	
	std::map<Addr, DebugInfo<unsigned int>> debugMap;
	
	void updateDebugInfo(Addr&, bool, void*&);
	void writeDebugInfo();
	
};

#endif
