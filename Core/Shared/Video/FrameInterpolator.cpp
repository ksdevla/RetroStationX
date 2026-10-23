#include "pch.h"
#include "Shared/Video/FrameInterpolator.h"
#include <chrono>

double FrameInterpolator::GetTime()
{
	using namespace std::chrono;
	return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

void FrameInterpolator::Reset()
{
	auto lock = _lock.AcquireSafe();
	_receivedCount = 0;
	_frameInterval = 0;
	_frameId++;
}

void FrameInterpolator::AddFrame(RenderedFrame& frame)
{
	double now = GetTime();
	auto lock = _lock.AcquireSafe();

	if(frame.Width != _receivedWidth || frame.Height != _receivedHeight) {
		//Size changed (e.g video filter change), can't interpolate between frames of different sizes
		_receivedWidth = frame.Width;
		_receivedHeight = frame.Height;
		_receivedCount = 0;
	}

	uint32_t* buffer = (uint32_t*)frame.FrameBuffer;
	int next = _receivedIndex ^ 1;
	_receivedFrames[next].assign(buffer, buffer + frame.Width * frame.Height);
	_receivedIndex = next;
	_receivedFrameNumber = frame.FrameNumber;
	_receivedCount = std::min<uint32_t>(_receivedCount + 1, 2);
	_frameId++;

	UpdateTimeline(now);
}

void FrameInterpolator::UpdateTimeline(double now)
{
	//Frames are received with some jitter (they go through the emulation and video decoder threads),
	//so keep a smoothed timeline instead of using the raw arrival times, to keep the motion even
	double delta = now - _lastArrivalTime;
	_lastArrivalTime = now;

	if(_receivedCount < 2 || _frameInterval <= 0 || delta < MinFrameInterval / 2 || delta > MaxFrameInterval * 2) {
		_frameInterval = _receivedCount < 2 ? 0 : delta;
		_frameTime = now;
		return;
	}

	_frameInterval += (delta - _frameInterval) * 0.05;

	double expected = _frameTime + _frameInterval;
	double error = now - expected;
	if(std::abs(error) > _frameInterval / 2) {
		//Too far off, resync
		_frameTime = now;
	} else {
		_frameTime = expected + error * 0.1;
	}
}

bool FrameInterpolator::GetFrame(RenderedFrame& output)
{
	double now = GetTime();
	double t = 1.0;
	bool newFrame = false;
	{
		auto lock = _lock.AcquireSafe();
		if(_receivedCount == 0) {
			return false;
		}

		if(_frameId != _analyzedFrameId) {
			_analyzedFrameId = _frameId;
			_width = _receivedWidth;
			_height = _receivedHeight;
			_frameNumber = _receivedFrameNumber;
			_cur = _receivedFrames[_receivedIndex];
			_hasPair = _receivedCount >= 2;
			if(_hasPair) {
				_prev = _receivedFrames[_receivedIndex ^ 1];
			}
			newFrame = true;
		}

		if(_frameInterval >= MinFrameInterval && _frameInterval <= MaxFrameInterval) {
			//The previous frame is displayed when the current frame is received, and the current frame
			//is reached one frame interval later - the frames in between are generated
			t = (now - _frameTime) / _frameInterval;
		}
	}

	if(newFrame && _hasPair) {
		PrepareAnalysis();
		EstimateMotion();
	}

	GenerateFrame(t, output);
	return true;
}

void FrameInterpolator::GenerateFrame(double t, RenderedFrame& output)
{
	t = std::clamp(t, 0.0, 1.0);
	if(!_hasPair || t >= 1.0) {
		output = RenderedFrame(_cur.data(), _width, _height, 1.0, _frameNumber);
		return;
	}

	Synthesize(t);
	output = RenderedFrame(_output.data(), _width, _height, 1.0, _frameNumber);
}

bool FrameInterpolator::GenerateFrameForTest(double t, RenderedFrame& output)
{
	RenderedFrame unused;
	if(!GetFrame(unused)) {
		return false;
	}
	GenerateFrame(t, output);
	return true;
}

void FrameInterpolator::PrepareAnalysis()
{
	//Use bigger blocks for frames that were upscaled by a video filter
	_blockSize = _width <= 320 ? 8 : (_width <= 640 ? 16 : 32);
	_searchRange = 16 * (int32_t)(_blockSize / 8);

	uint32_t blocksX = (_width + _blockSize - 1) / _blockSize;
	uint32_t blocksY = (_height + _blockSize - 1) / _blockSize;
	if(blocksX != _blocksX || blocksY != _blocksY) {
		_blocksX = blocksX;
		_blocksY = blocksY;
		_prevVectors.assign(_blocksX * _blocksY, {});
	}
	_vectors.assign(_blocksX * _blocksY, {});
	_output.resize(_width * _height);

	ComputeLuma(_prev, _prevLuma);
	ComputeLuma(_cur, _curLuma);
}

void FrameInterpolator::ComputeLuma(vector<uint32_t>& frame, vector<uint8_t>& luma)
{
	luma.resize(frame.size());
	for(size_t i = 0, len = frame.size(); i < len; i++) {
		uint32_t c = frame[i];
		luma[i] = (uint8_t)((((c >> 16) & 0xFF) * 77 + ((c >> 8) & 0xFF) * 150 + (c & 0xFF) * 29) >> 8);
	}
}

FrameInterpolator::MotionVector FrameInterpolator::FindGlobalVector()
{
	//Compare row/column sums between both frames to find the dominant scrolling (cheap, and gives a good starting candidate)
	MotionVector result = {};
	int32_t w = (int32_t)_width;
	int32_t h = (int32_t)_height;

	vector<uint32_t> colCur(w), colPrev(w), rowCur(h), rowPrev(h);
	for(int32_t y = 0; y < h; y++) {
		for(int32_t x = 0; x < w; x++) {
			colCur[x] += _curLuma[y * w + x];
			colPrev[x] += _prevLuma[y * w + x];
			rowCur[y] += _curLuma[y * w + x];
			rowPrev[y] += _prevLuma[y * w + x];
		}
	}

	auto findShift = [this](vector<uint32_t>& cur, vector<uint32_t>& prev) -> int16_t {
		int32_t len = (int32_t)cur.size();
		double bestCost = -1;
		int32_t bestShift = 0;
		for(int32_t shift = -_searchRange; shift <= _searchRange; shift++) {
			uint64_t cost = 0;
			int32_t count = 0;
			for(int32_t i = std::max(0, shift); i < std::min(len, len + shift); i++) {
				cost += (uint64_t)std::abs((int64_t)cur[i] - (int64_t)prev[i - shift]);
				count++;
			}
			double avgCost = count ? (double)cost / count : 0;
			if(bestCost < 0 || avgCost < bestCost || (avgCost == bestCost && std::abs(shift) < std::abs(bestShift))) {
				bestCost = avgCost;
				bestShift = shift;
			}
		}
		return (int16_t)bestShift;
	};

	result.X = findShift(colCur, colPrev);
	result.Y = findShift(rowCur, rowPrev);
	return result;
}

uint32_t FrameInterpolator::GetBlockCost(uint32_t bx, uint32_t by, MotionVector v, uint32_t maxCost)
{
	//Sum of absolute differences between the block in the current frame and the same block in the previous frame, offset by -v
	int32_t w = (int32_t)_width;
	int32_t h = (int32_t)_height;
	int32_t x0 = bx * _blockSize;
	int32_t y0 = by * _blockSize;
	int32_t x1 = std::min<int32_t>(x0 + _blockSize, w);
	int32_t y1 = std::min<int32_t>(y0 + _blockSize, h);

	bool insideX = x0 - v.X >= 0 && x1 - v.X <= w;

	uint32_t cost = 0;
	for(int32_t y = y0; y < y1; y++) {
		int32_t py = std::clamp(y - v.Y, 0, h - 1);
		uint8_t* curRow = &_curLuma[y * w];
		uint8_t* prevRow = &_prevLuma[py * w];
		if(insideX) {
			uint8_t* shiftedPrevRow = prevRow - v.X;
			for(int32_t x = x0; x < x1; x++) {
				cost += (uint32_t)std::abs((int32_t)curRow[x] - (int32_t)shiftedPrevRow[x]);
			}
		} else {
			for(int32_t x = x0; x < x1; x++) {
				int32_t px = std::clamp(x - v.X, 0, w - 1);
				cost += (uint32_t)std::abs((int32_t)curRow[x] - (int32_t)prevRow[px]);
			}
		}
		if(cost >= maxCost) {
			return cost;
		}
	}
	return cost;
}

bool FrameInterpolator::IsFlatBlock(uint32_t bx, uint32_t by)
{
	int32_t w = (int32_t)_width;
	int32_t x0 = bx * _blockSize;
	int32_t y0 = by * _blockSize;
	int32_t x1 = std::min<int32_t>(x0 + _blockSize, w);
	int32_t y1 = std::min<int32_t>(y0 + _blockSize, (int32_t)_height);

	uint32_t color = _cur[y0 * w + x0];
	for(int32_t y = y0; y < y1; y++) {
		for(int32_t x = x0; x < x1; x++) {
			if(_cur[y * w + x] != color) {
				return false;
			}
		}
	}
	return true;
}

void FrameInterpolator::EstimateMotion()
{
	_globalVector = FindGlobalVector();

	//Small penalty for vectors that differ from the neighboring blocks, keeps the field smooth in flat areas
	uint32_t penaltyPerPixel = _blockSize * _blockSize / 16;

	for(uint32_t by = 0; by < _blocksY; by++) {
		for(uint32_t bx = 0; bx < _blocksX; bx++) {
			uint32_t index = by * _blocksX + bx;
			if(IsFlatBlock(bx, by)) {
				//Motion can't be measured in a single-color area (any vector matches), and picking one at random can erase
				//objects that disappear (e.g flickering sprites). Objects moving into this area use their own block's vector.
				_vectors[index] = {};
				continue;
			}

			MotionVector left = bx > 0 ? _vectors[index - 1] : MotionVector {};
			MotionVector top = by > 0 ? _vectors[index - _blocksX] : MotionVector {};
			MotionVector topRight = (by > 0 && bx + 1 < _blocksX) ? _vectors[index - _blocksX + 1] : MotionVector {};

			//Median of the neighbors is the predicted vector
			MotionVector pred;
			pred.X = (int16_t)std::max(std::min(left.X, top.X), std::min(std::max(left.X, top.X), topRight.X));
			pred.Y = (int16_t)std::max(std::min(left.Y, top.Y), std::min(std::max(left.Y, top.Y), topRight.Y));

			uint32_t bestCost = UINT32_MAX;
			MotionVector best = {};
			auto evaluate = [&](MotionVector v) {
				if(std::abs(v.X) > _searchRange || std::abs(v.Y) > _searchRange) {
					return;
				}
				uint32_t penalty = penaltyPerPixel * (uint32_t)(std::abs(v.X - pred.X) + std::abs(v.Y - pred.Y));
				if(penalty >= bestCost) {
					return;
				}
				uint32_t cost = GetBlockCost(bx, by, v, bestCost - penalty) + penalty;
				if(cost < bestCost) {
					bestCost = cost;
					best = v;
				}
			};

			evaluate(pred);
			evaluate({});
			evaluate(_globalVector);
			evaluate(_prevVectors[index]);
			evaluate(left);
			evaluate(top);
			evaluate(topRight);

			//Refine around the best candidate
			for(int i = 0; i < 8 && bestCost > 0; i++) {
				MotionVector center = best;
				evaluate({ (int16_t)(center.X - 1), center.Y });
				evaluate({ (int16_t)(center.X + 1), center.Y });
				evaluate({ center.X, (int16_t)(center.Y - 1) });
				evaluate({ center.X, (int16_t)(center.Y + 1) });
				if(best == center) {
					break;
				}
			}

			_vectors[index] = best;
		}
	}

	_prevVectors = _vectors;
}

void FrameInterpolator::Synthesize(double t)
{
	int32_t w = (int32_t)_width;
	int32_t h = (int32_t)_height;
	int32_t bs = (int32_t)_blockSize;
	int32_t blocksX = (int32_t)_blocksX;
	int32_t blocksY = (int32_t)_blocksY;
	vector<uint32_t>& nearest = t < 0.5 ? _prev : _cur;

	auto colorsMatch = [](uint32_t a, uint32_t b) {
		if(a == b) {
			return true;
		}
		int32_t diff = std::abs((int32_t)((a >> 16) & 0xFF) - (int32_t)((b >> 16) & 0xFF)) + std::abs((int32_t)((a >> 8) & 0xFF) - (int32_t)((b >> 8) & 0xFF)) + std::abs((int32_t)(a & 0xFF) - (int32_t)(b & 0xFF));
		return diff <= 24;
	};

	struct Candidate
	{
		MotionVector Vector;
		int32_t ShiftX;
		int32_t ShiftY;
	};

	//A pixel at position p at time t comes from p - t*v in the previous frame and ends up at p + (1-t)*v in the current frame.
	//The vector is only used when both frames agree on the color - otherwise the pixel from the nearest frame is used.
	//This avoids blending/ghosting (pixel art stays sharp), at the cost of some motion not being interpolated.
	auto makeCandidate = [t](MotionVector v) {
		return Candidate { v, (int32_t)std::floor(t * v.X + 0.5), (int32_t)std::floor(t * v.Y + 0.5) };
	};

	auto tryCandidate = [&](int32_t x, int32_t y, Candidate& c, uint32_t& result) {
		int32_t ax = x - c.ShiftX;
		int32_t ay = y - c.ShiftY;
		int32_t bx = ax + c.Vector.X;
		int32_t by = ay + c.Vector.Y;
		if(ax < 0 || ay < 0 || ax >= w || ay >= h || bx < 0 || by < 0 || bx >= w || by >= h) {
			return false;
		}
		uint32_t a = _prev[ay * w + ax];
		if(colorsMatch(a, _cur[by * w + bx])) {
			result = a;
			return true;
		}
		return false;
	};

	for(int32_t blockY = 0; blockY < blocksY; blockY++) {
		for(int32_t blockX = 0; blockX < blocksX; blockX++) {
			Candidate own = makeCandidate(_vectors[blockY * blocksX + blockX]);

			//When the block's own vector doesn't work for a pixel, it may belong to an object moving in from a neighboring block
			Candidate others[5];
			int otherCount = 0;
			auto addCandidate = [&](MotionVector v) {
				if(v == own.Vector) {
					return;
				}
				for(int i = 0; i < otherCount; i++) {
					if(others[i].Vector == v) {
						return;
					}
				}
				others[otherCount++] = makeCandidate(v);
			};
			if(blockX > 0) {
				addCandidate(_vectors[blockY * blocksX + blockX - 1]);
			}
			if(blockX + 1 < blocksX) {
				addCandidate(_vectors[blockY * blocksX + blockX + 1]);
			}
			if(blockY > 0) {
				addCandidate(_vectors[(blockY - 1) * blocksX + blockX]);
			}
			if(blockY + 1 < blocksY) {
				addCandidate(_vectors[(blockY + 1) * blocksX + blockX]);
			}
			addCandidate({});

			int32_t x0 = blockX * bs;
			int32_t y0 = blockY * bs;
			int32_t x1 = std::min(x0 + bs, w);
			int32_t y1 = std::min(y0 + bs, h);
			for(int32_t y = y0; y < y1; y++) {
				for(int32_t x = x0; x < x1; x++) {
					uint32_t& out = _output[y * w + x];
					if(tryCandidate(x, y, own, out)) {
						continue;
					}

					bool found = false;
					for(int i = 0; i < otherCount; i++) {
						if(tryCandidate(x, y, others[i], out)) {
							found = true;
							break;
						}
					}

					if(!found) {
						out = nearest[y * w + x];
					}
				}
			}
		}
	}
}
