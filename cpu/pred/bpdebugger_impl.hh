#include <bitset>
#include <type_traits>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "cpu/pred/bpdebugger.hh"
#include "debug/DebugInfo.hh"
#include "debug/Pattern.hh"
#include "base/trace.hh"

#define LOG 1

template<typename History>
void DebugInfo<History>::printDebugInfo(std::ofstream& file, const Addr& addr)
{
	DPRINTF(DebugInfo, "Branch Addr(%i): \t\t\t\t\t%i\n", unCondBr, addr);
	DPRINTF(DebugInfo, "Execution times: \t\t\t\t\t%i\n", count);
	DPRINTF(DebugInfo, "Branch Missprediction rate: \t\t\t%f\n", (double)miss/(double)count);
	DPRINTF(DebugInfo, "Branch Hit times: \t\t\t\t\t%i\n", hit);
	DPRINTF(DebugInfo, "Correct prediction(taken): \t\t\t%i\n", hitTaken);
	DPRINTF(DebugInfo, "Correct prediction(taken) Avg weight: \t\t%i\n", hitTakenWeight/(int)(hitTaken+1));
	DPRINTF(DebugInfo, "Correct prediction(not taken): \t\t\t%i\n", hitNotTaken);
	DPRINTF(DebugInfo, "Correct prediction(not taken) Avg weight: \t\t%i\n", hitNotTakenWeight/(int)(hitNotTaken+1));

	DPRINTF(DebugInfo, "Branch Miss times: \t\t\t\t%i\n", miss);
	DPRINTF(DebugInfo, "Miss prediction(taken): \t\t\t\t%i\n", missTaken);
	DPRINTF(DebugInfo, "Miss prediction(taken) Avg weight: \t\t%i\n", missTakenWeight/(int)(missTaken+1));
	DPRINTF(DebugInfo, "Miss prediction(not taken): \t\t\t%i\n", missNotTaken);
	DPRINTF(DebugInfo, "Miss prediction(not taken) Avg weight: \t\t%i\n", missNotTakenWeight/(int)(missNotTaken+1));

	DPRINTF(DebugInfo, "Branch Taken times: \t\t\t\t%i\n", taken);
	DPRINTF(DebugInfo, "Branch Taken Avg weight: \t\t\t\t%i\n", takenWeight/(int)(taken+1));
	DPRINTF(DebugInfo, "Branch Not Taken times: \t\t\t\t%i\n", notTaken);
	DPRINTF(DebugInfo, "Branch Not Taken Avg weight: \t\t\t%i\n", notTakenWeight/(int)(notTaken+1));

	DPRINTF(DebugInfo, "Branch history pattern #: \t\t\t\t%i\n", histPattern.size());
	DPRINTF(DebugInfo, "Branch conflicts \t\t\t\t\t%i\n", conflictSet.size());

	DPRINTF(DebugInfo, "Global History Used: \t\t\t\t%i\n", globalCount);
	DPRINTF(DebugInfo, "Local History Used: \t\t\t\t%i\n", localCount);

#ifdef LOG
	if(!histPattern.empty())	printPatternInfo(file, 5);
	if(!conflictSet.empty())	printConflictInfo(file, addr);
#endif
}

template<typename A, typename B>
std::pair<B,A> flipPair(const std::pair<A,B>& ele)
{
	return std::pair<B,A>(ele.second, ele.first);
}

template<typename History>
void DebugInfo<History>::printHistoryPattern(std::ofstream& file, History history)
{
	std::bitset<64> historyBits;
	for(int i = 0; i < history.size(); i++)
	{
		historyBits[i] = history[i];
	}
	file.setf(std::ios::hex, std::ios::basefield);
	file.setf(std::ios::showbase);
	file << historyBits.to_ulong() << ", ";
	file.unsetf(std::ios::hex);
	file.unsetf(std::ios::showbase);
}

template<>
void DebugInfo<unsigned int>::printHistoryPattern(std::ofstream& file, unsigned int history)
{
	file.setf(std::ios::hex, std::ios::basefield);
	file.setf(std::ios::showbase);
	file << history << ", ";
	file.unsetf(std::ios::hex);
	file.unsetf(std::ios::showbase);
}

template<typename History>
void DebugInfo<History>::printPatternInfo(std::ofstream& file, int maxCount)
{
	file << '\n' << "-------------Start of Pattern Info---------------" << '\n';	

	std::multimap<unsigned int, History> reverseMap;

	std::transform(histPattern.begin(), histPattern.end(), std::inserter(reverseMap, reverseMap.begin()),flipPair<History, unsigned int>);

	int i = 0;
	auto rIt = reverseMap.rbegin();
    while(i < maxCount && rIt != reverseMap.rend())
	{
		auto key = rIt->first;
		file << "#" << i+1 << " access pattern(" << key << " times): "<< '\n';
		auto it_pair = reverseMap.equal_range(key);
		for(auto it = it_pair.first; it != it_pair.second; it++, rIt++)
		{
			printHistoryPattern(file, it->second);
			if(histTaken.find(it->second) != histTaken.end())
				file << "this pattern suggests taken: " << histTaken[it->second] << " times\n";
		}
		i++;
  	}

	file << '\n' << "-------------End of Pattern Info---------------" << '\n';	
}

template<typename History>
void DebugInfo<History>::printCorrelationInfo(std::ofstream& file, const Addr& addr)
{
	file << "Addr: " << addr << '\n';
	file << '\n' << "-------------Start of Correlation Info---------------" << '\n';

	for(auto it = correlator.begin(); it != correlator.end(); it++)
	{
		file << "----------New record----------" << '\n';
		auto& histVec = (*it)->histVec;
		for(auto br = histVec.begin(); br != histVec.end(); br++)
		{
			file << br->addr << '\t' << br->weights << '\t' << br->taken << '\n';
		}
		printHistoryPattern(file, (*it)->pattern);
		file << "Pattern suggests " << (*it)->result << '\n';
	}
	for(auto it = correlator.begin(); it != correlator.end(); it++)
	{
		delete *it;
	}
	file << '\n' << "-------------End of Correlation Info---------------" << '\n';
}

template<typename History>
void DebugInfo<History>::printConflictInfo(std::ofstream& file, const Addr& addr)
{
	file << '\n' << "-------------Start of Conflict Info---------------" << '\n';
	file << "conflict addr:" << '\n';
	for(auto it = conflictSet.begin() ; it != conflictSet.end(); it++)
	{
		if(*it != addr)		file << *it << '\n';
	}
	file <<'\n' <<  "-------------End of Conflict Info---------------" << '\n';
}
