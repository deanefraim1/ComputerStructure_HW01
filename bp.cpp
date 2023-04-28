/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */
// 0x1230 in binary is 0001001000110000
#include "bp_api.h"
#include <iostream>
#include <vector>
#include <tuple>

#define LSB_SHARED_STARTING_INDEX 2
#define MID_SHARED_STARTING_INDEX 16
#define TARGET_BITS 30
#define VALID_BIT 1

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
		void ResetFSMEntry(FSMState fsmInitialState);
};

class HistoryEntry{
	private:
		unsigned historySize;
		unsigned history;
		unsigned mask;

	public:
		HistoryEntry();
		HistoryEntry(unsigned historySize);
		unsigned GetHistory();
		unsigned GetHistorySize();
		void UpdateHistory(bool taken);
		void ResetHistory();
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
		void UpdateBTBEntry(unsigned tag, unsigned targetPc, bool isLocalHistory, bool isLocalFSMTable, FSMState fsmInitialState);
};

class BTB{
protected:
	unsigned btbSize;
	unsigned btbSizeBits;
	unsigned historySize;
	unsigned tagSize;
	FSMState fsmInitialState;
	SharedOption sharedOption;
	unsigned flushNumber;
	unsigned branchNumber;
	unsigned allocatedMemory;
	BTBEntry *btbEntries;
	bool isLocalHistory;
	bool isLocalFSMTable;

public:
	BTB(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int shared);
	bool Predict(uint32_t pc, uint32_t *dst);
	void Update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	SIM_stats GetStats();
	BTBEntry *GetBTBEntry(uint32_t pc);
	virtual ~BTB();
};

class BTB_GlobalHistoryGlobalFSM : public BTB{
private:
	HistoryEntry globalHistoryEntry;
	FSMEntry *globalFSMTable;

public:
	BTB_GlobalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int Shared);
	~BTB_GlobalHistoryGlobalFSM();
};

class BTB_GlobalHistoryLocalFSM : public BTB{
private:
	HistoryEntry globalHistoryEntry;
	FSMEntry **localFSMTables;

public:
	BTB_GlobalHistoryLocalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int Shared);
	~BTB_GlobalHistoryLocalFSM();
};

class BTB_LocalHistoryGlobalFSM : public BTB{
private:
	HistoryEntry *localHistoryEntries;
	FSMEntry *globalFSMTable;

public:
	BTB_LocalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int Shared);
	~BTB_LocalHistoryGlobalFSM();
};

class BTB_LocalHistoryLocalFSM : public BTB{
};

unsigned ParseBinary(unsigned numberToParse, unsigned startingIndex, unsigned numberOfBits); 
unsigned GetFSMTableIndex(unsigned history, unsigned historySize, uint32_t pc, SharedOption sharedOption);
unsigned GetNumberOfBitsFromDividableBy2(unsigned number);
bool GetPredictionFromFSMState(FSMState state);
FSMState ParseNumberAsFSMState(unsigned number);

BTB *btb;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	if(isGlobalHist && isGlobalTable)
		btb = new BTB_GlobalHistoryGlobalFSM(btbSize, historySize, tagSize, fsmState, Shared);
	
	if(isGlobalHist && !isGlobalTable)
		btb = new BTB_GlobalHistoryLocalFSM(btbSize, historySize, tagSize, fsmState, Shared);
	
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
	this->fsmInitialState = ParseNumberAsFSMState(fsmInitialState);
	this->btbSizeBits = GetNumberOfBitsFromDividableBy2(btbSize);
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

bool BTB::Predict(uint32_t pc, uint32_t *dst){
	unsigned tagToSearchFor = ParseBinary(pc, 2 + this->btbSizeBits, tagSize);
	BTBEntry *btbEntry = this->GetBTBEntry(pc);
	if (btbEntry->occupied && btbEntry->tag == tagToSearchFor && btbEntry->fsmTable[btbEntry->GetFSMTableIndex(pc, this->sharedOption)].GetPrediction())
	{
		*dst = btbEntry->targetPc;
		return true;
	}
	*dst = pc + 4;
	return false;
}

void BTB::Update(uint32_t pc, uint32_t targetPc, bool actualTakenOrNotTaken, uint32_t btbPredictionDestination){
	unsigned tagToSearchFor = ParseBinary(pc, 2 + this->btbSizeBits, tagSize);
	BTBEntry *btbEntry = GetBTBEntry(pc);
	int fsmTableIndex = btbEntry->GetFSMTableIndex(pc, this->sharedOption);
	bool btbPrediction = btbEntry->fsmTable[fsmTableIndex].GetPrediction();
	// If we found the entry in the BTB
	if (btbEntry->occupied && btbEntry->tag == tagToSearchFor)
	{
		btbEntry->targetPc = targetPc;
		if(btbPrediction != actualTakenOrNotTaken || (btbPrediction == true && actualTakenOrNotTaken == true && btbPredictionDestination != targetPc))
			this->flushNumber++;
	}
	// Else we need to add the entry to the BTB
	else
	{
		btbEntry->UpdateBTBEntry(tagToSearchFor, targetPc, this->isLocalHistory, this->isLocalFSMTable, this->fsmInitialState);

		// if the actual prediction was Taken, we need to flush because we assume NotTaken
		if(actualTakenOrNotTaken == true) // TODO - check if we need to flush if the actual targetPC is PC + 4
			this->flushNumber++;
	}

	// Update the FSM and the history
	btbEntry->fsmTable[fsmTableIndex].UpdateFSM(actualTakenOrNotTaken);
	btbEntry->history->UpdateHistory(actualTakenOrNotTaken);
	this->branchNumber++;
}

BTB_GlobalHistoryGlobalFSM::BTB_GlobalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int Shared) : BTB(btbSize, historySize, tagSize, fsmInitialState, Shared), globalHistoryEntry(HistoryEntry(historySize)){
	this->globalFSMTable = new FSMEntry[1 << historySize];
	for (int i = 0; i < (1 << historySize); i++)
	{
		globalFSMTable[i] = FSMEntry(fsmInitialState);
	}
	for (int i = 0; i < btbSize; i++)
	{
		btbEntries[i].history = &globalHistoryEntry;
		btbEntries[i].fsmTable = globalFSMTable;
	}
	this->allocatedMemory = (this->tagSize+TARGET_BITS+VALID_BIT) * btbSize + this->historySize + ((1 << historySize) * 2);
	this->isLocalHistory = false;
	this->isLocalFSMTable = false;
}

BTB_GlobalHistoryGlobalFSM::~BTB_GlobalHistoryGlobalFSM(){
	delete[] this->globalFSMTable;
}

BTB_GlobalHistoryLocalFSM::BTB_GlobalHistoryLocalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int Shared) : BTB(btbSize, historySize, tagSize, fsmInitialState, Shared), globalHistoryEntry(HistoryEntry(historySize)){
	this->localFSMTables = new FSMEntry*[btbSize];
	for(int i = 0; i < btbSize; i++)
	{
		localFSMTables[i] = new FSMEntry[1 << historySize];
		for (int j = 0; j < (1 << historySize); j++)
		{
			localFSMTables[i][j] = FSMEntry(fsmInitialState);
		}
		btbEntries[i].history = &globalHistoryEntry;
		btbEntries[i].fsmTable = localFSMTables[i];
	}
	this->allocatedMemory = (this->tagSize+TARGET_BITS+VALID_BIT) * btbSize + this->historySize + ((1 << historySize) * btbSize * 2);
	this->isLocalHistory = false;
	this->isLocalFSMTable = true;
}

BTB_GlobalHistoryLocalFSM::~BTB_GlobalHistoryLocalFSM(){
	for(int i = 0; i < btbSize; i++)
	{
		delete[] localFSMTables[i];
	}
	delete[] localFSMTables;
}

BTB_LocalHistoryGlobalFSM::BTB_LocalHistoryGlobalFSM(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmInitialState, int Shared) : BTB(btbSize, historySize, tagSize, fsmInitialState, Shared){
	this->localHistoryEntries = new HistoryEntry[btbSize];
	for (int i = 0; i < btbSize; i++)
	{
		localHistoryEntries[i] = HistoryEntry(historySize);
		btbEntries[i].history = &localHistoryEntries[i];
		btbEntries[i].fsmTable = globalFSMTable;
	}
	this->allocatedMemory = (this->tagSize+TARGET_BITS+VALID_BIT) * btbSize + this->historySize * btbSize + ((1 << historySize) * 2);
	this->isLocalHistory = true;
	this->isLocalFSMTable = false;
}

BTB_LocalHistoryGlobalFSM::~BTB_LocalHistoryGlobalFSM(){
	delete[] this->localHistoryEntries;
}

HistoryEntry::HistoryEntry(){
	this->historySize = 0;
	this->history = 0;
	this->mask = 0;
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

void HistoryEntry::ResetHistory(){
	this->history = 0;
}

FSMEntry::FSMEntry(){
	this->fsmState = WeaklyNotTaken;
}

FSMEntry::FSMEntry(unsigned fsmInitialState){
	this->fsmState = ParseNumberAsFSMState(fsmInitialState);
}

bool FSMEntry::GetPrediction(){
	return GetPredictionFromFSMState(this->fsmState);
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

void FSMEntry::ResetFSMEntry(FSMState fsmInitialState){
	this->fsmState = fsmInitialState;
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

void BTBEntry::UpdateBTBEntry(unsigned tag, unsigned targetPc, bool isLocalHistory, bool isLocalFSMTable, FSMState fsmInitialState){
	this->occupied = true;
	this->tag = tag;
	this->targetPc = targetPc;
	if(isLocalHistory)
		this->history->ResetHistory();
	if(isLocalFSMTable)
		for(int i = 0; i < (1 << this->history->GetHistorySize()); i++)
			this->fsmTable[i].ResetFSMEntry(fsmInitialState);
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

BTBEntry *BTB::GetBTBEntry(uint32_t pc){
	unsigned btbSizeBits = GetNumberOfBitsFromDividableBy2(btbSize);
	unsigned indexToSearchIn = ParseBinary(pc, 2, btbSizeBits);
	return &(this->btbEntries[indexToSearchIn]);
}

bool GetPredictionFromFSMState(FSMState state)
{
	return state == WeaklyTaken || state == StronglyTaken;
}

FSMState ParseNumberAsFSMState(unsigned number)
{
	switch (number)
	{
	case 0:
		return StronglyNotTaken;

	case 1:
		return WeaklyNotTaken;

	case 2:
		return WeaklyTaken;
	
	case 3:
		return StronglyTaken;

	default:
		return WeaklyNotTaken;
	}
}