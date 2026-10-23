#pragma once
#include "pch.h"
#include <deque>
#include "Shared/BaseControlDevice.h"

class Emulator;

class RewindData
{
private:
	//Uncompressed save state, taken at the start of this entry's frame(s)
	//The data is never modified once saved, so copies of this entry (e.g for the history viewer) share it
	shared_ptr<const vector<uint8_t>> _saveStateData;

public:
	std::deque<ControlDeviceState> InputLogs[BaseControlDevice::PortCount];
	int32_t FrameCount = 0;
	bool EndOfSegment = false;

	void GetStateData(stringstream& stateData);
	uint64_t GetStateSize() { return _saveStateData ? _saveStateData->size() : 0; }

	void LoadState(Emulator* emu, bool sendNotification = true);
	void SaveState(Emulator* emu);
};
