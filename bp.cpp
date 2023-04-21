/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */
// 0x1230 in binary is 0001001000110000
#include "bp_api.h"
#include <iostream>
#include <vector>
#include <tuple>

using namespace std;

BTB *btb;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	if(isGlobalHist && isGlobalTable)
		btb = new BTB_GlobalHistoryGlobalFSM(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	
	if(isGlobalHist && !isGlobalTable)
		btb = new BTB_GlobalHistoryLocalFSM(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	
	if(!isGlobalHist && isGlobalTable)
		btb = new BTB_LocalHistoryGlobalFSM(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);

	if(!isGlobalHist && !isGlobalTable)
		btb = new BTB_LocalHistoryLocalFSM(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	*curStats = btb->GetStats();
	return;
}

class BTB{
protected:
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int Shared;
	unsigned flushNumber;
	unsigned branchNumber;
	unsigned allocatedMemory;
	tuple<unsigned, unsigned> *tagAndTargetList;

public:
	BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	virtual bool Predict(uint32_t pc, uint32_t *dst) = 0;
	virtual void Update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) = 0;
	virtual SIM_stats GetStats() = 0;
	virtual ~BTB();
};

BTB::BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	this->btbSize = btbSize;
	this->historySize = historySize;
	this->tagSize = tagSize;
	this->fsmState = fsmState;
	this->isGlobalHist = isGlobalHist;
	this->isGlobalTable = isGlobalTable;
	this->Shared = Shared;
	this->tagAndTargetList = new tuple<unsigned, unsigned>[btbSize];
}

class BTB_GlobalHistoryGlobalFSM : public BTB{
private:
	HistoryEntry history;
	FSMEntry *fsmTable;

public:
	BTB_GlobalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	bool Predict(uint32_t pc, uint32_t *dst);
	~BTB_GlobalHistoryGlobalFSM();
};

BTB_GlobalHistoryGlobalFSM::BTB_GlobalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState,
			bool isGlobalHist, bool isGlobalTable, int Shared) : BTB(btbSize, historySize, tagSize, fsmInitialState, isGlobalHist, isGlobalTable, Shared){
	history = HistoryEntry(historySize);
	fsmTable = new FSMEntry*[2^historySize];
	for(int i = 0; i < 2^historySize; i++)
		fsmTable[i] = FSMEntry(fsmInitialState);
}

bool BTB_GlobalHistoryGlobalFSM::Predict(uint32_t pc, uint32_t *dst){
	bool excist = this->tagAndTargetList
}

class BTB_GlobalHistoryLocalFSM : public BTB{
};

class BTB_LocalHistoryGlobalFSM : public BTB{
};

class BTB_LocalHistoryLocalFSM : public BTB{
};

class HistoryEntry{
	private:
		unsigned historySize;
		unsigned history;
		unsigned mask;

	public:
		HistoryEntry(unsigned historySize);
		unsigned GetHistory();
		void UpdateHistory(bool taken);
};

HistoryEntry::HistoryEntry(unsigned historySize){
	this->historySize = historySize;
	this->history = 0;
	this->mask = (1 << historySize) - 1;
}

unsigned HistoryEntry::GetHistory(){
	return history;
}

void HistoryEntry::UpdateHistory(bool taken){
	history = (history << 1) | taken;
	history = history & mask;
}

class FSMEntry{
	private:
		unsigned fsmState;

	public:
		FSMEntry(unsigned fsmInitialState);
		bool GetPrediction();
		void UpdateFSM(bool taken);
		~FSMEntry();
};

FSMEntry::FSMEntry(unsigned fsmInitialState){
	this->fsmState = fsmInitialState;
}

bool FSMEntry::GetPrediction(){
	return fsmState > 1;
}

void FSMEntry::UpdateFSM(bool taken){
	if(taken){
		if(fsmState < 3)
			fsmState++;
	}
	else{
		if(fsmState > 0)
			fsmState--;
	}
}