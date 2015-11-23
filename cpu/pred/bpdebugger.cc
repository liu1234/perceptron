#include "cpu/pred/bpdebugger.hh"
#include "debug/DebugInfo.hh"
#include "base/trace.hh"

void DebugInfo::printDebugInfo(const Addr& addr)
{
	DPRINTF(DebugInfo, "Branch Addr(%i): \t\t\t\t\t%s\n", unCondBr, addr);
	DPRINTF(DebugInfo, "Execution times: \t\t\t\t\t%i\n", count);
	DPRINTF(DebugInfo, "Branch Missprediction rate: \t\t\t%f\n", (double)miss/(double)count);
	DPRINTF(DebugInfo, "Branch Hit times: \t\t\t\t\t%i\n", hit);
	DPRINTF(DebugInfo, "Correct prediction(taken): \t\t\t\t%i\n", hitTaken);
	DPRINTF(DebugInfo, "Correct prediction(taken) Avg weight: \t\t%i\n", hitTakenWeight/(int)(hitTaken+1));
	DPRINTF(DebugInfo, "Correct prediction(not taken): \t\t\t%i\n", hitNotTaken);
	DPRINTF(DebugInfo, "Correct prediction(not taken) Avg weight: \t\t%i\n", hitNotTakenWeight/(int)(hitNotTaken+1));

	DPRINTF(DebugInfo, "Branch Miss times: \t\t\t\t\t%i\n", miss);
	DPRINTF(DebugInfo, "Miss prediction(taken): \t\t\t\t%i\n", missTaken);
	DPRINTF(DebugInfo, "Miss prediction(taken) Avg weight: \t\t\t%i\n", missTakenWeight/(int)(missTaken+1));
	DPRINTF(DebugInfo, "Miss prediction(not taken): \t\t\t%i\n", missNotTaken);
	DPRINTF(DebugInfo, "Miss prediction(not taken) Avg weight: \t\t%i\n", missNotTakenWeight/(int)(missNotTaken+1));

	DPRINTF(DebugInfo, "Branch Taken times: \t\t\t\t%i\n", taken);
	DPRINTF(DebugInfo, "Branch Taken Avg weight: \t\t\t\t%i\n", takenWeight/(int)(taken+1));
	DPRINTF(DebugInfo, "Branch Not Taken times: \t\t\t\t%i\n", notTaken);
	DPRINTF(DebugInfo, "Branch Not Taken Avg weight: \t\t\t%i\n", notTakenWeight/(int)(notTaken+1));

	DPRINTF(DebugInfo, "Branch history pattern #: \t\t\t\t%i\n", histPattern.size());
	DPRINTF(DebugInfo, "Branch conflicts \t\t\t\t\t%i\n", conflictSet.size());
}
