#ifndef __CPU_O3_BPDEBUGGER_PRED_HH__
#define __CPU_O3_BPDEBUGGER_PRED_HH__

#include <map>
#include <vector>
#include <set>
#include <deque>
#include <fstream>
#include <memory>

#include "base/types.hh"
//#include "cpu/pred/perceptron.hh"
//#include "cpu/pred/gshare.hh"
//#include "cpu/pred/tournament.hh"

class PerceptronBP;
class GshareBP;
class TournamentBP;

template<typename A, typename B>
std::pair<B,A> flipPair(const std::pair<A,B>& ele);

template<typename History>
class DebugInfo
{
	friend class PerceptronBP;
	friend class GshareBP;
	friend class TournamentBP;

	void printDebugInfo(std::ofstream&, const Addr&);
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

	unsigned int globalCount;
	unsigned int localCount;
	
	std::map<History, unsigned int> histPattern;
	
	// If linear separable
	std::map<History, unsigned int> histTaken;
	
public:
	struct Correlation
	{
		struct RelatedBr
		{
			RelatedBr() {}
			RelatedBr(const Addr& _addr, const int& _weights, const bool& _taken) : addr(_addr), weights(_weights), taken(_taken) {}
			Addr addr;
			int weights;
			bool taken;
		};
		Correlation() {}
		std::vector<RelatedBr> histVec;
		History pattern;
		bool result;
	};
private:
	std::vector<Correlation*> correlator;

	unsigned int coflict;
	std::set<Addr> conflictSet;

	bool unCondBr;

	bool stablized;

	void printPatternInfo(std::ofstream&, int maxCount);
	void printHistoryPattern(std::ofstream&, History);
	void printCorrelationInfo(std::ofstream&, const Addr&);
	void printConflictInfo(std::ofstream&, const Addr&);
};

#endif
