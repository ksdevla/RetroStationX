#include "pch.h"
#include "Shared/RewindData.h"
#include "Shared/Emulator.h"
#include "Shared/SaveStateManager.h"

namespace
{
	//Read-only stream over existing memory - avoids copying the whole state into a stringstream on every load
	class MemoryStreamBuf : public std::streambuf
	{
	public:
		MemoryStreamBuf(const vector<uint8_t>& data)
		{
			char* start = (char*)data.data();
			setg(start, start, start + data.size());
		}

	protected:
		pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override
		{
			char* target;
			switch(dir) {
				case std::ios_base::beg: target = eback() + off; break;
				case std::ios_base::cur: target = gptr() + off; break;
				default: target = egptr() + off; break;
			}

			if(target < eback() || target > egptr()) {
				return pos_type(off_type(-1));
			}
			setg(eback(), target, egptr());
			return pos_type(target - eback());
		}

		pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
		{
			return seekoff(off_type(pos), std::ios_base::beg, which);
		}
	};
}

void RewindData::GetStateData(stringstream& stateData)
{
	if(_saveStateData) {
		stateData.write((char*)_saveStateData->data(), _saveStateData->size());
	}
}

void RewindData::LoadState(Emulator* emu, bool sendNotification)
{
	if(!_saveStateData || _saveStateData->empty()) {
		return;
	}

	MemoryStreamBuf buffer(*_saveStateData);
	std::istream stream(&buffer);
	emu->Deserialize(stream, SaveStateManager::FileFormatVersion, true, std::nullopt, sendNotification);
}

void RewindData::SaveState(Emulator* emu)
{
	std::stringstream state;
	emu->Serialize(state, true, 0);

	string data = state.str();
	_saveStateData = std::make_shared<const vector<uint8_t>>(data.begin(), data.end());
	FrameCount = 0;
}
