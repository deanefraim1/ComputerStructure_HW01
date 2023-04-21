/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */
// 0x1230 in binary is 0001001000110000
#include "bp_api.h"
#include <iostream>
#include <vector>
#include <tuple>

using namespace std;

enum FSMState{
	StronglyNotTaken = 0,
	WeaklyNotTaken = 1,
	WeaklyTaken = 2,
	StronglyTaken = 3
};

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
	BTBEntry *tagTakenTargetList;

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
	this->tagTakenTargetList = new BTBEntry[btbSize];
}

class BTB_GlobalHistoryGlobalFSM : public BTB{
private:
	HistoryEntry history;
	FSMEntry *fsmTable;

public:
	BTB_GlobalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	bool Predict(uint32_t pc, uint32_t *dst);
	void Update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	SIM_stats GetStats();
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
	unsigned indexToSearchIn = parseBinary(pc, 3, btbSize);
	unsigned tagToSearchFor = parseBinary(pc, 3 + btbSize, tagSize);
	BTBEntry *btbEntry = &(this->tagTakenTargetList[indexToSearchIn]);
	if (btbEntry->occupied && btbEntry->tag == tagToSearchFor)
	{
		*dst = btbEntry->targetPc;
		return btbEntry->takenOrNotTaken;
	}
	*dst = pc + 4;
	return false;
}

void BTB_GlobalHistoryGlobalFSM::Update(uint32_t pc, uint32_t targetPc, bool takenOrNotTaken, uint32_t pred_dst){
	unsigned indexToSearchIn = parseBinary(pc, 3, btbSize);
	unsigned tagToSearchFor = parseBinary(pc, 3 + btbSize, tagSize);
	BTBEntry* btbEntry = &(this->tagTakenTargetList[indexToSearchIn]);
	if (btbEntry->occupied && btbEntry->tag == tagToSearchFor)
	{
		btbEntry->targetPc = targetPc;
		btbEntry->takenOrNotTaken = takenOrNotTaken;
	}
	else
	{
		this->tagTakenTargetList[indexToSearchIn] = BTBEntry(tagToSearchFor, targetPc, takenOrNotTaken);
	}
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
		FSMState fsmState;

	public:
		FSMEntry(unsigned fsmInitialState);
		bool GetPrediction();
		void UpdateFSM(bool taken);
		~FSMEntry();
};

FSMEntry::FSMEntry(unsigned fsmInitialState){
	switch (fsmInitialState)
	{
	case 0:
		this->fsmState = StronglyNotTaken;
		break;

	case 1:
		this->fsmState = WeaklyNotTaken;
		break;

	case 2:
		this->fsmState = WeaklyTaken;
		break;
	
	case 3:
		this->fsmState = StronglyTaken;
		break;
	}
}

bool FSMEntry::GetPrediction(){
	return fsmState == WeaklyTaken || fsmState == StronglyTaken;
}

void FSMEntry::UpdateFSM(bool taken){
	switch (fsmState)
	{
	case StronglyNotTaken:
		if(taken)
			fsmState = WeaklyNotTaken;
		break;

	case WeaklyNotTaken:
		if(taken)
			fsmState = WeaklyTaken;
		else
			fsmState = StronglyNotTaken;
		break;

	case WeaklyTaken:
		if(taken)
			fsmState = StronglyTaken;
		else
			fsmState = WeaklyNotTaken;
		break;
	
	case StronglyTaken:
		if(!taken)
			fsmState = WeaklyTaken;
		break;
	}
}

class BTBEntry{
	public:
		bool occupied;
		unsigned tag;
		bool takenOrNotTaken;
		unsigned targetPc;

		BTBEntry();
		BTBEntry(unsigned tag, bool taken, unsigned targetPc);
		~BTBEntry();
};

BTBEntry::BTBEntry(){
	this->occupied = false;
}

BTBEntry::BTBEntry(unsigned tag, bool taken, unsigned targetPc){
	this->occupied = true;
	this->tag = tag;
	this->takenOrNotTaken = taken;
	this->targetPc = targetPc;
}


/// @brief parses a binary number from startingIndex to startingIndex + numberOfBits
/// @param numberToParse the unsigned number to parse
/// @param startingIndex the starting index to parse from (starting from index 0) (includes the starting index)
/// @param numberOfBits the number of bits to parse
/// @return the parsed number
unsigned parseBinary(unsigned numberToParse, unsigned startingIndex, unsigned numberOfBits){
	unsigned mask = (1 << numberOfBits) - 1;
	return (numberToParse >> startingIndex) & mask;
}