/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */
// 0x1230 in binary is 0001001000110000
#include "bp_api.h"
#include <iostream>
#include <vector>
#include <tuple>

#define LSB_SHARED_STARTING_INDEX 2
#define MID_SHARED_STARTING_INDEX 16
#define VALID_BIT=1
#define TARGET_BITS=30

using namespace std;

enum FSMState{
	StronglyNotTaken = 0,
	WeaklyNotTaken = 1,
	WeaklyTaken = 2,
	StronglyTaken = 3
};

enum SharedOption{
	NotShared = 0,
	LSBShared = 1,
	MidShared = 2
};

class FSMEntry{
	private:
		FSMState fsmState;

	public:
		FSMEntry();
		FSMEntry(unsigned fsmInitialState);
		bool GetPrediction();
		void UpdateFSM(bool taken);
};

class HistoryEntry{
	private:
		unsigned historySize;
		unsigned history;
		unsigned mask;

	public:
		HistoryEntry(unsigned historySize);
		unsigned GetHistory();
		unsigned GetHistorySize();
		void UpdateHistory(bool taken);
};

class BTBEntry{
	public:
		bool occupied;
		unsigned tag;
		unsigned targetPc;
		HistoryEntry *history;
		FSMEntry *fsmTable;

		BTBEntry();
		BTBEntry(unsigned tag, unsigned targetPc, HistoryEntry *history, FSMEntry *fsmTable);
		unsigned GetFSMTableIndex(uint32_t pc, SharedOption sharedOption);
};


class BTB{
protected:
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmInitialState;
	SharedOption sharedOption;
	unsigned flushNumber;
	unsigned branchNumber;
	unsigned allocatedMemory;
	BTBEntry *btbEntries;

public:
	BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int shared);
	virtual bool Predict(uint32_t pc, uint32_t *dst) = 0;
	virtual void Update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst) = 0;
	SIM_stats GetStats();
	virtual ~BTB();
};

class BTB_GlobalHistoryGlobalFSM : public BTB{
private:
	HistoryEntry globalHistoryEntry;
	FSMEntry *globalFSMTable;

public:
	BTB_GlobalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int Shared);
	bool Predict(uint32_t pc, uint32_t *dst);
	void Update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	~BTB_GlobalHistoryGlobalFSM();
};

class BTB_GlobalHistoryLocalFSM : public BTB{
};

class BTB_LocalHistoryGlobalFSM : public BTB{
};

class BTB_LocalHistoryLocalFSM : public BTB{
};

unsigned ParseBinary(unsigned numberToParse, unsigned startingIndex, unsigned numberOfBits); 
unsigned GetFSMTableIndex(unsigned history, unsigned historySize, uint32_t pc, SharedOption sharedOption);
unsigned GetNumberOfBitsFromDividableBy2(unsigned number);

BTB *btb;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	if(isGlobalHist && isGlobalTable)
		btb = new BTB_GlobalHistoryGlobalFSM(btbSize, historySize, tagSize, fsmState, Shared);
	
	// if(isGlobalHist && !isGlobalTable)
	// 	//btb = new BTB_GlobalHistoryLocalFSM(btbSize, historySize, tagSize, fsmState, Shared);
	
	// if(!isGlobalHist && isGlobalTable)
	// 	//btb = new BTB_LocalHistoryGlobalFSM(btbSize, historySize, tagSize, fsmState, Shared);

	// if(!isGlobalHist && !isGlobalTable)
	// 	//btb = new BTB_LocalHistoryLocalFSM(btbSize, historySize, tagSize, fsmState, Shared);
		
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return btb->Predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	btb->Update(pc, targetPc, taken, pred_dst);
	return;
}

void BP_GetStats(SIM_stats *curStats){
	*curStats = btb->GetStats();
	return;
}

BTB::BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int shared){
	this->btbSize = btbSize;
	this->historySize = historySize;
	this->tagSize = tagSize;
	this->fsmInitialState = fsmInitialState;
	switch (shared)
	{
	case 0:
		this->sharedOption = NotShared;
		break;
	
	case 1:
		this->sharedOption = LSBShared;
		break;
	
	case 2:
		this->sharedOption = MidShared;
		break;
	}
	this->flushNumber = 0;
	this->branchNumber = 0;
	this->btbEntries = new BTBEntry[btbSize];
}

BTB::~BTB(){
	delete[] this->btbEntries;
}

SIM_stats BTB::GetStats(){
	SIM_stats stats;
	stats.flush_num = this->flushNumber;
	stats.br_num = this->branchNumber;
	stats.size = this->allocatedMemory;
	return stats;
}



BTB_GlobalHistoryGlobalFSM::BTB_GlobalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int Shared) : BTB(btbSize, historySize, tagSize, fsmInitialState, Shared), globalHistoryEntry(HistoryEntry(historySize)), globalFSMTable(new FSMEntry[1 << historySize]){
	for(int i = 0; i < (1 << historySize); i++)
	{
		globalFSMTable[i] = FSMEntry(fsmInitialState);
	}
	for (int i = 0; i < btbSize; i++)
	{
		btbEntries[i].history = &globalHistoryEntry;
		btbEntries[i].fsmTable = globalFSMTable;
	}
	this->allocatedMemory = this->(tagSize+TARGET_BITS+VALID_BIT)* btbSize + this->historySize + ((1 << historySize) * 2); // TODO - check if we need to add the target pc size
}

BTB_GlobalHistoryGlobalFSM::~BTB_GlobalHistoryGlobalFSM(){
	delete[] this->globalFSMTable;
}

bool BTB_GlobalHistoryGlobalFSM::Predict(uint32_t pc, uint32_t *dst){
	unsigned btbSizeBits = GetNumberOfBitsFromDividableBy2(btbSize);
	unsigned indexToSearchIn = ParseBinary(pc, 2, btbSizeBits);
	unsigned tagToSearchFor = ParseBinary(pc, 2 + btbSizeBits, tagSize);
	BTBEntry *btbEntry = &(this->btbEntries[indexToSearchIn]);
	if (btbEntry->occupied && btbEntry->tag == tagToSearchFor && btbEntry->fsmTable[btbEntry->GetFSMTableIndex(pc, this->sharedOption)].GetPrediction())
	{
		*dst = btbEntry->targetPc;
		return true;
	}
	*dst = pc + 4;
	return false;
}

void BTB_GlobalHistoryGlobalFSM::Update(uint32_t pc, uint32_t targetPc, bool takenOrNotTaken, uint32_t pred_dst){
	unsigned btbSizeBits = GetNumberOfBitsFromDividableBy2(btbSize);
	unsigned indexToSearchIn = ParseBinary(pc, 2, btbSizeBits);
	unsigned tagToSearchFor = ParseBinary(pc, 2 + btbSizeBits, tagSize);
	BTBEntry *btbEntry = &(this->btbEntries[indexToSearchIn]);
	this->globalHistoryEntry.UpdateHistory(takenOrNotTaken);
	if (btbEntry->occupied && btbEntry->tag == tagToSearchFor)
	{
		btbEntry->targetPc = targetPc;
	}
	else
	{
		this->btbEntries[indexToSearchIn] = BTBEntry(tagToSearchFor, targetPc, &(this->globalHistoryEntry), this->globalFSMTable);
		this->globalFSMTable[btbEntry->GetFSMTableIndex(pc, this->sharedOption)] = FSMEntry(this->fsmInitialState); // TODO - check if we need to update the prediction with default value
	}
	
	if(!(this->globalFSMTable[btbEntry->GetFSMTableIndex(pc, this->sharedOption)].GetPrediction() == takenOrNotTaken && targetPc == pred_dst))
	{
		this->flushNumber++;
	}
	this->globalFSMTable[btbEntry->GetFSMTableIndex(pc, this->sharedOption)].UpdateFSM(takenOrNotTaken);
	this->branchNumber++;
}



HistoryEntry::HistoryEntry(unsigned historySize){
	this->historySize = historySize;
	this->history = 0;
	this->mask = (1 << historySize) - 1;
}

unsigned HistoryEntry::GetHistory(){
	return history;
}

unsigned HistoryEntry::GetHistorySize(){
	return historySize;
}

void HistoryEntry::UpdateHistory(bool taken){
	history = (history << 1) | taken;
	history = history & mask;
}



FSMEntry::FSMEntry(){
	this->fsmState = WeaklyNotTaken;
}

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
	return this->fsmState == WeaklyTaken || this->fsmState == StronglyTaken;
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



BTBEntry::BTBEntry(){
	this->occupied = false;
}

BTBEntry::BTBEntry(unsigned tag, unsigned targetPc, HistoryEntry *history, FSMEntry *fsmTable){
	this->occupied = true;
	this->tag = tag;
	this->targetPc = targetPc;
	this->history = history;
	this->fsmTable = fsmTable;
}

unsigned BTBEntry::GetFSMTableIndex(uint32_t pc, SharedOption sharedOption){
	switch (sharedOption)
	{
	case NotShared:
		return this->history->GetHistory();
		break;

	case LSBShared:
		return this->history->GetHistory() ^ ParseBinary(pc, LSB_SHARED_STARTING_INDEX, this->history->GetHistorySize());
		break;

	case MidShared:
		return this->history->GetHistory() ^ ParseBinary(pc, MID_SHARED_STARTING_INDEX, this->history->GetHistorySize());
		break;
	}
}


/// @brief parses a binary number from startingIndex to startingIndex + numberOfBits
/// @param numberToParse the unsigned number to parse
/// @param startingIndex the starting index to parse from (starting from index 0) (includes the starting index)
/// @param numberOfBits the number of bits to parse
/// @return the parsed number
unsigned ParseBinary(unsigned numberToParse, unsigned startingIndex, unsigned numberOfBits){
	unsigned mask = (1 << numberOfBits) - 1;
	return (numberToParse >> startingIndex) & mask;
}

unsigned GetNumberOfBitsFromDividableBy2(unsigned number){
	unsigned log = 0;
	while (number >>= 1)
		log++;
	return log;
	
}