#pragma once
#include "pch.h"
#include "Shared/RenderedFrame.h"
#include "Utilities/SimpleLock.h"

//Frame generation: creates in-between frames so that games running at ~60 fps look smoother on high refresh rate monitors.
//-AddFrame() is called for every frame produced by the emulator (video decoder thread)
//-GetFrame() is called by the render thread on every screen refresh, and returns the frame to display at that point in time,
// interpolated between the last 2 emulator frames using block-based motion estimation.
//This adds ~1 emulated frame of display latency (the next frame must be known before in-between frames can be generated).
class FrameInterpolator
{
private:
	struct MotionVector
	{
		int16_t X = 0;
		int16_t Y = 0;

		bool operator==(const MotionVector& other) const { return X == other.X && Y == other.Y; }
	};

	//Data written by the video decoder thread (protected by _lock)
	SimpleLock _lock;
	vector<uint32_t> _receivedFrames[2];
	int _receivedIndex = 0;
	uint32_t _receivedWidth = 0;
	uint32_t _receivedHeight = 0;
	uint32_t _receivedFrameNumber = 0;
	uint32_t _receivedCount = 0;
	uint64_t _frameId = 0;
	double _lastArrivalTime = 0;
	double _frameTime = 0; //Smoothed timeline position of the most recent frame (ms)
	double _frameInterval = 0; //Smoothed time between frames (ms)

	//Data used by the render thread
	uint64_t _analyzedFrameId = 0;
	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t _frameNumber = 0;
	bool _hasPair = false;
	vector<uint32_t> _prev;
	vector<uint32_t> _cur;
	vector<uint8_t> _prevLuma;
	vector<uint8_t> _curLuma;
	vector<uint32_t> _output;

	uint32_t _blockSize = 8;
	uint32_t _blocksX = 0;
	uint32_t _blocksY = 0;
	int32_t _searchRange = 16;
	vector<MotionVector> _vectors;
	vector<MotionVector> _prevVectors;
	MotionVector _globalVector = {};

	static double GetTime();
	void UpdateTimeline(double now);

	void PrepareAnalysis();
	void ComputeLuma(vector<uint32_t>& frame, vector<uint8_t>& luma);
	MotionVector FindGlobalVector();
	uint32_t GetBlockCost(uint32_t bx, uint32_t by, MotionVector v, uint32_t maxCost);
	bool IsFlatBlock(uint32_t bx, uint32_t by);
	void EstimateMotion();
	void Synthesize(double t);
	void GenerateFrame(double t, RenderedFrame& output);

public:
	//Source frame rates outside of this range (fast forward, slow motion, etc.) are displayed without interpolation
	static constexpr double MinFrameInterval = 1000.0 / 125;
	static constexpr double MaxFrameInterval = 1000.0 / 30;

	void AddFrame(RenderedFrame& frame);
	bool GetFrame(RenderedFrame& output);
	void Reset();

	//Analyzes the last 2 frames received (if not done yet) and generates the frame at position t (0 = previous frame, 1 = current frame)
	bool GenerateFrameForTest(double t, RenderedFrame& output);
};
