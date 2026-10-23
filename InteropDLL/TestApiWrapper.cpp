#include "Common.h"
#include "Core/Shared/RecordedRomTest.h"
#include "Core/Shared/Emulator.h"
#include "Core/Shared/EmuSettings.h"
#include "Core/Shared/RewindManager.h"
#include "Core/Shared/NotificationManager.h"
#include "Core/Shared/BaseControlDevice.h"
#include "Core/Shared/Interfaces/IInputProvider.h"
#include "Core/Shared/Interfaces/INotificationListener.h"
#include "Core/Shared/Video/FrameInterpolator.h"
#include "Utilities/FolderUtilities.h"
#include "Utilities/StringUtilities.h"
#include "Utilities/HexUtilities.h"

extern unique_ptr<Emulator> _emu;
shared_ptr<RecordedRomTest> _recordedRomTest;

namespace RewindTest
{
	//Feeds controller 1 with a different, deterministic button combination on every frame
	//While rewinding, the rewind manager must supply the recorded inputs - if it doesn't, an
	//invalid input is fed instead so that any missing/misaligned input makes the test fail
	class InputProvider final : public IInputProvider
	{
		Emulator* _emu;

	public:
		InputProvider(Emulator* emu) : _emu(emu) {}

		bool SetInput(BaseControlDevice* device) override
		{
			if(device->GetPort() != 0) {
				return false;
			}

			uint32_t frame = _emu->GetFrameCount();
			uint8_t buttons = (uint8_t)((frame * 2654435761u) >> 13);
			if(_emu->GetRewindManager()->IsRewinding()) {
				buttons = 0xA5;
			}

			for(int i = 0; i < 8; i++) {
				device->SetBitValue(i, (buttons >> i) & 1);
			}
			return true;
		}
	};

	//Captures the console's RAM the first time the target frame is reached outside of rewind
	class RamCapture final : public INotificationListener
	{
		Emulator* _emu;
		uint32_t _targetFrame;

	public:
		vector<uint8_t> Ram;
		std::atomic<bool> Captured = false;

		RamCapture(Emulator* emu, uint32_t targetFrame) : _emu(emu), _targetFrame(targetFrame) {}

		void ProcessNotification(ConsoleNotificationType type, void* parameter) override
		{
			if(type == ConsoleNotificationType::PpuFrameDone && !Captured && !_emu->GetRewindManager()->IsRewinding() && _emu->GetFrameCount() == _targetFrame) {
				ConsoleMemoryInfo mem = _emu->GetMemory(MemoryType::NesInternalRam);
				Ram = vector<uint8_t>((uint8_t*)mem.Memory, (uint8_t*)mem.Memory + mem.Size);
				Captured = true;
			}
		}
	};

	struct Run
	{
		unique_ptr<Emulator> Emu;
		unique_ptr<InputProvider> Input;
		shared_ptr<RamCapture> Capture;

		bool Start(const char* romFile, uint32_t rewindBufferSize, uint32_t targetFrame)
		{
			Emu.reset(new Emulator());
			Emu->Initialize(false);
			Emu->GetSettings()->GetNesConfig().Port1.Type = ControllerType::NesController;
			Emu->GetSettings()->GetNesConfig().RamPowerOnState = RamState::AllZeros;
			Emu->GetSettings()->GetPreferences().RewindBufferSize = rewindBufferSize;
			Emu->GetSettings()->GetEmulationConfig().EmulationSpeed = 0;
			Emu->GetSettings()->GetEmulationConfig().RewindSpeed = 0;

			Input.reset(new InputProvider(Emu.get()));
			Capture.reset(new RamCapture(Emu.get(), targetFrame));
			Emu->GetNotificationManager()->RegisterNotificationListener(Capture);
			if(!Emu->LoadRom(VirtualFile(string(romFile)), VirtualFile())) {
				return false;
			}
			//Registered after LoadRom so the rewind manager's input provider has priority
			Emu->RegisterInputProvider(Input.get());
			return true;
		}

		bool WaitFor(std::function<bool()> condition, int timeoutMs = 30000)
		{
			auto start = std::chrono::steady_clock::now();
			while(!condition()) {
				if(std::chrono::steady_clock::now() - start > std::chrono::milliseconds(timeoutMs)) {
					return false;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			return true;
		}

		bool WaitForFrame(uint32_t frame)
		{
			return WaitFor([&]() { return Emu->GetFrameCount() >= frame; });
		}
		bool WaitForRewindTo(uint32_t frame)
		{
			return WaitFor([&]() { return Emu->GetFrameCount() <= frame || !Emu->GetRewindManager()->IsRewinding(); });
		}
		bool WaitForRewindEnd()
		{
			return WaitFor([&]() { return !Emu->GetRewindManager()->IsRewinding(); });
		}

		void End()
		{
			Emu->UnregisterInputProvider(Input.get());
			Emu->Stop(false);
			Emu->Release();
			Emu.reset();
		}
	};
}

extern "C"
{
	DllExport RomTestResult __stdcall RunRecordedTest(const char* filename, bool inBackground)
	{
		if(inBackground) {
			unique_ptr<Emulator> emu(new Emulator());
			emu->Initialize(false);
			emu->GetSettings()->SetFlag(EmulationFlags::TestMode);
			shared_ptr<RecordedRomTest> romTest(new RecordedRomTest(emu.get(), true));
			RomTestResult result = romTest->Run(filename);
			emu->Release();
			return result;
		} else {
			shared_ptr<RecordedRomTest> romTest(new RecordedRomTest(_emu.get(), false));
			return romTest->Run(filename);
		}
	}

	DllExport uint64_t __stdcall RunTest(char* filename, uint32_t address, MemoryType memType)
	{
		unique_ptr<Emulator> emu(new Emulator());
		emu->Initialize();
		emu->GetSettings()->SetFlag(EmulationFlags::TestMode);
		emu->GetSettings()->GetGameboyConfig().Model = GameboyModel::Gameboy;
		emu->GetSettings()->GetGameboyConfig().RamPowerOnState = RamState::AllZeros;
		emu->LoadRom((VirtualFile)filename, VirtualFile());
		emu->GetSettings()->SetFlag(EmulationFlags::MaximumSpeed);

		while(emu->GetFrameCount() < 500) {
			std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(10));
		}

		ConsoleMemoryInfo memInfo = emu->GetMemory(memType);
		uint8_t* memBuffer = (uint8_t*)memInfo.Memory;
		uint64_t result = memBuffer[address];
		for(int i = 1; i < 8; i++) {
			if(address + i < memInfo.Size) {
				result |= ((uint64_t)memBuffer[address + i] << (8 * i));
			} else {
				break;
			}
		}

		emu->Stop(false);
		emu->Release();

		return result;
	}

	DllExport void __stdcall RomTestRecord(char* filename, bool reset)
	{
		_recordedRomTest.reset(new RecordedRomTest(_emu.get(), false));
		_recordedRomTest->Record(filename, reset);
	}

	DllExport void __stdcall RomTestStop()
	{
		if(_recordedRomTest) {
			_recordedRomTest->Stop();
			_recordedRomTest.reset();
		}
	}

	DllExport bool __stdcall RomTestRecording()
	{
		return _recordedRomTest != nullptr;
	}

	DllExport bool __stdcall RunCiTests(string testFolder, bool enableDebugger)
	{
		std::replace(testFolder.begin(), testFolder.end(), '\\', '/');
		if(!StringUtilities::EndsWith(testFolder, "/")) {
			testFolder += "/";
		}

		//GBA tests can't run without the GBA BIOS, skip all of them
		vector<string> foldersToSkip = { "GBA/" };

		unordered_set<string> testsToSkip = {
			//These GB tests fail because the test runner can't use the official GB boot rom
			"GB/GBEmulatorShootout/hacktix/bully_gbc.mtp",
			"GB/GBEmulatorShootout/hacktix/bully.mtp",
			"GB/GBEmulatorShootout/mooneye/acceptance/boot_div-dmgABCmgb.mtp",
			"GB/GBEmulatorShootout/mooneye/acceptance/boot_hwio-dmgABCmgb.mtp",
			"GB/GBEmulatorShootout/mooneye/acceptance/serial/boot_sclk_align-dmgABCmgb.mtp",
			"GB/GBEmulatorShootout/mooneye/misc/boot_div-cgbABCDE.mtp",

			//SGB tests can't run properly without the SGB ROM
			"SNES/sgb_packet_test.mtp",
			"SNES/sgb-ext-test.mtp",
			"SNES/sgb-mlt-test.mtp"
		};

		vector<string> tests = FolderUtilities::GetFilesInFolder(testFolder, { ".mtp" }, true, 5);

		vector<string> testsToRun;

		for(string& test : tests) {
			string testPath = test.substr(testFolder.size());
			std::replace(testPath.begin(), testPath.end(), '\\', '/');

			bool include = true;
			for(string& folderToSkip : foldersToSkip) {
				if(StringUtilities::StartsWith(testPath, folderToSkip.c_str())) {
					include = false;
					break;
				}
			}

			if(testsToSkip.find(testPath) != testsToSkip.end()) {
				include = false;
			}

			if(include) {
				testsToRun.push_back(test);
			}
		}

		FolderUtilities::SetHomeFolder(FolderUtilities::CombinePath(testFolder, "MesenHomeFolder"));

		std::cout << "== CI test mode ==\n";
		int threadCount = std::thread::hardware_concurrency();
		int skipCount = (int)(tests.size() - testsToRun.size());
		std::cout << "Running on " << threadCount << " threads\n";
		std::cout << "Test count: " << tests.size() << "\n";
		std::cout << "Tests to skip: " << skipCount << "\n\n";

		std::atomic<int> testNumber = 0;
		std::atomic<int> failCount = 0;
		std::atomic<int> passCount = 0;
		std::atomic<int> progress = 0;

		vector<thread> threads(threadCount);
		for(int i = 0; i < threadCount; i++) {
			threads[i] = std::thread([&]() {
				while(true) {
					int nextTest = testNumber++;
					if(nextTest >= testsToRun.size()) {
						break;
					}

					RomTestResult result = RunRecordedTest(testsToRun[nextTest].c_str(), true);
					if(result.State == RomTestState::Failed) {
						failCount++;
						string testPath = testsToRun[nextTest].substr(testFolder.size());
						std::cout << ("\n== FAILED: " + testPath + "\n");
					} else {
						passCount++;
					}

					int newProgress = (failCount + passCount) * 100 / (uint32_t)testsToRun.size();
					if(newProgress > progress) {
						std::cout << ("\rRunning... (" + std::to_string(newProgress) + "%)");
						progress = newProgress;
					}
				}
			});
		}

		for(int i = 0; i < threadCount; i++) {
			threads[i].join();
		}

		std::cout << "\n\n===================\n";
		std::cout << "Passed: " << passCount << "\n";
		std::cout << "Failed: " << failCount << "\n";
		std::cout << "Skipped: " << skipCount << "\n";
		std::cout << "===================\n";

		return failCount == 0;
	}

	//Verifies that rewinding never changes the outcome of the emulation: the RAM at the target frame
	//must be identical with and without rewind operations (see the RewindTest classes above)
	//Each scenario runs in its own emulator instance so a failure points to a specific operation
	DllExport bool __stdcall RunRewindTest(const char* romFile, const char* logFile)
	{
		using namespace RewindTest;
		ofstream log(logFile, ios::out | ios::trunc);
		constexpr uint32_t targetFrame = 3000;

		Run reference;
		if(!reference.Start(romFile, 4096, targetFrame)) {
			log << "Reference run: could not load ROM\n";
			return false;
		}
		if(!reference.WaitFor([&]() { return (bool)reference.Capture->Captured; })) {
			log << "Reference run: target frame not captured (reached frame " << reference.Emu->GetFrameCount() << ")\n";
			reference.End();
			return false;
		}
		vector<uint8_t> expectedRam = reference.Capture->Ram;
		reference.End();

		bool allPassed = true;
		auto scenario = [&](string name, std::function<void(Run&)> actions) {
			Run run;
			if(!run.Start(romFile, 4096, targetFrame)) {
				log << name << ": could not load ROM\n";
				allPassed = false;
				return;
			}
			run.Emu->GetSettings()->GetEmulationConfig().EmulationSpeed = 500;
			actions(run);
			uint32_t lastOpFrame = run.Emu->GetFrameCount();
			bool captured = run.WaitFor([&]() { return (bool)run.Capture->Captured; });
			string result;
			if(lastOpFrame >= targetFrame) {
				result = "INCONCLUSIVE (operations ended after the target frame)";
			} else if(!captured) {
				result = "FAIL (target frame not reached)";
			} else if(run.Capture->Ram != expectedRam) {
				int diffCount = 0;
				int firstDiff = -1;
				for(size_t i = 0; i < expectedRam.size(); i++) {
					if(run.Capture->Ram[i] != expectedRam[i]) {
						diffCount++;
						if(firstDiff < 0) {
							firstDiff = (int)i;
						}
					}
				}
				result = "FAIL (" + std::to_string(diffCount) + " RAM bytes differ, first at $" + HexUtilities::ToHex((uint16_t)firstDiff) + ")";
			} else {
				result = "PASS";
			}
			if(result != "PASS") {
				allPassed = false;
			}
			//Check that the history is consistent: replaying an entry's frames must lead exactly to the next entry's
			//frame (within a segment). Overlapping/duplicated frames would desync whenever rewind stops on them.
			{
				auto lock = run.Emu->AcquireLock();
				deque<RewindData> history = run.Emu->GetRewindManager()->GetHistory();
				int badEntries = 0;
				string firstBad;
				for(size_t i = 0; i + 1 < history.size(); i++) {
					if(history[i].EndOfSegment) {
						continue;
					}
					history[i].LoadState(run.Emu.get(), false);
					uint32_t start = run.Emu->GetFrameCount();
					history[i + 1].LoadState(run.Emu.get(), false);
					uint32_t next = run.Emu->GetFrameCount();
					if(start + history[i].FrameCount != next) {
						if(badEntries++ == 0) {
							firstBad = "entry at frame " + std::to_string(start) + " has " + std::to_string(history[i].FrameCount) + " frames but the next entry is at frame " + std::to_string(next);
						}
					}
				}
				if(badEntries) {
					result = (result == "PASS" ? "FAIL" : result) + " - history inconsistent: " + std::to_string(badEntries) + " bad entries, first: " + firstBad;
					allPassed = false;
				}
			}

			log << name << ": " << result << "\n";
			run.End();
		};

		auto hold = [&](Run& run, uint32_t startFrame, uint32_t rewindToFrame) {
			run.WaitForFrame(startFrame);
			RewindManager* rewind = run.Emu->GetRewindManager();
			uint32_t from = run.Emu->GetFrameCount();
			rewind->StartRewinding();
			run.WaitForRewindTo(rewindToFrame);
			uint32_t to = run.Emu->GetFrameCount();
			rewind->StopRewinding();
			run.WaitForRewindEnd();
			log << "  hold: frame " << from << " -> " << to << ", resumed at " << run.Emu->GetFrameCount() << "\n";
		};

		auto taps = [&](Run& run, int count) {
			RewindManager* rewind = run.Emu->GetRewindManager();
			for(int i = 0; i < count; i++) {
				run.WaitForFrame(run.Emu->GetFrameCount() + 3 + (i % 5));
				rewind->StartRewinding();
				if(i % 2) {
					std::this_thread::sleep_for(std::chrono::milliseconds(i % 7));
				}
				rewind->StopRewinding();
				run.WaitForRewindEnd();
			}
			log << "  " << count << " taps, ended at frame " << run.Emu->GetFrameCount() << "\n";
		};

		scenario("No rewind", [&](Run& run) { run.WaitForFrame(500); });
		scenario("Hold once", [&](Run& run) { hold(run, 400, 300); });
		scenario("Hold twice (crossing first resume point)", [&](Run& run) {
			hold(run, 400, 300);
			hold(run, 500, 250);
		});
		scenario("Quick taps", [&](Run& run) {
			run.WaitForFrame(300);
			taps(run, 25);
		});
		scenario("Quick taps, then hold across them", [&](Run& run) {
			run.WaitForFrame(300);
			taps(run, 25);
			uint32_t frame = run.Emu->GetFrameCount();
			hold(run, frame + 30, 280);
		});
		scenario("RewindSeconds(3)", [&](Run& run) {
			run.WaitForFrame(500);
			uint32_t before = run.Emu->GetFrameCount();
			run.Emu->GetRewindManager()->RewindSeconds(3);
			log << "  frame " << before << " -> " << run.Emu->GetFrameCount() << " (expected ~" << (uint32_t)(3 * run.Emu->GetFps()) << " frames back)\n";
		});
		scenario("RewindSeconds(3), then hold across the jump", [&](Run& run) {
			run.WaitForFrame(500);
			run.Emu->GetRewindManager()->RewindSeconds(3);
			uint32_t frame = run.Emu->GetFrameCount();
			hold(run, frame + 60, frame - 60);
		});

		//Performance: frames per second at unlimited speed, with and without rewind
		for(uint32_t bufferSize : { 0u, 4096u }) {
			Run perf;
			perf.Start(romFile, bufferSize, 0xFFFFFFFF);
			perf.WaitForFrame(100);
			auto start = std::chrono::steady_clock::now();
			perf.WaitForFrame(5100);
			double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
			RewindStats stats = perf.Emu->GetRewindManager()->GetStats();
			log << "Speed with rewind " << (bufferSize ? "enabled" : "disabled") << ": " << (int)(5000 / seconds) << " fps";
			if(stats.HistorySize) {
				log << " (" << stats.MemoryUsage / stats.HistorySize << " bytes per frame)";
			}
			log << "\n";
			perf.End();
		}

		log << (allPassed ? "PASS" : "FAIL") << "\n";
		return allPassed;
	}

	//Checks the frame generation (FrameInterpolator) on synthetic pixel art frames with known motion:
	//the generated in-between frame is compared with the expected frame, pixel by pixel
	DllExport bool __stdcall RunFrameInterpolatorTest(const char* logFile)
	{
		ofstream log(logFile, ios::out | ios::trunc);
		constexpr int w = 256;
		constexpr int h = 240;
		const uint32_t palette[6] = { 0xFF5C94FC, 0xFF00A800, 0xFFC84C0C, 0xFFFCBCB0, 0xFF000000, 0xFFF8F8F8 };

		//Tiled pixel-art background (like a NES nametable), with some detail inside each tile
		auto background = [&](int x, int y) -> uint32_t {
			int tx = (x >> 3) & 0xFF;
			int ty = (y >> 3) & 0xFF;
			uint32_t hash = (uint32_t)(tx * 73856093) ^ (uint32_t)(ty * 19349663);
			uint32_t color = palette[hash % 4];
			if(((x + y) & 7) == 0 || ((x & 7) == 3 && (hash & 8))) {
				color = palette[4];
			}
			return color;
		};
		auto sprite = [&](int sx, int sy) -> uint32_t {
			return ((sx / 2 + sy / 2) & 1) ? palette[2] : palette[5];
		};

		struct Frame
		{
			vector<uint32_t> Pixels = vector<uint32_t>(256 * 240);
			uint32_t& At(int x, int y) { return Pixels[y * 256 + x]; }
		};

		//scrollX: camera position, spriteX: sprite position (-1 = no sprite), hud: static status bar at the top
		auto makeFrame = [&](double scrollX, int spriteX, bool hud) {
			Frame f;
			for(int y = 0; y < h; y++) {
				for(int x = 0; x < w; x++) {
					if(hud && y < 32) {
						f.At(x, y) = ((x / 8 + y / 8) % 3 == 0) ? palette[5] : palette[4];
					} else if(scrollX < 0) {
						f.At(x, y) = palette[0]; //flat sky
					} else {
						f.At(x, y) = background(x + (int)scrollX, y);
					}
				}
			}
			if(spriteX >= 0) {
				for(int y = 0; y < 16; y++) {
					for(int x = 0; x < 16; x++) {
						f.At(spriteX + x, 100 + y) = sprite(x, y);
					}
				}
			}
			return f;
		};

		bool allPassed = true;
		auto check = [&](string name, Frame prev, Frame cur, double t, Frame expected, int marginX, double minMatchPercent) {
			FrameInterpolator interpolator;
			RenderedFrame prevFrame(prev.Pixels.data(), w, h);
			RenderedFrame curFrame(cur.Pixels.data(), w, h);
			interpolator.AddFrame(prevFrame);
			interpolator.AddFrame(curFrame);

			RenderedFrame output;
			if(!interpolator.GenerateFrameForTest(t, output)) {
				log << name << ": FAIL (no frame generated)\n";
				allPassed = false;
				return;
			}

			uint32_t* pixels = (uint32_t*)output.FrameBuffer;
			int total = 0;
			int matches = 0;
			for(int y = 0; y < h; y++) {
				for(int x = marginX; x < w - marginX; x++) {
					total++;
					if(pixels[y * w + x] == expected.At(x, y)) {
						matches++;
					}
				}
			}
			double percent = 100.0 * matches / total;
			bool pass = percent >= minMatchPercent;
			allPassed &= pass;
			log << name << ": " << (pass ? "PASS" : "FAIL") << " (" << StringUtilities::ToString(percent, 2) << "% of pixels match, need " << minMatchPercent << "%)\n";
		};

		//Camera scrolls 4 pixels per frame - halfway, the screen must be scrolled by 2 pixels
		check("Scrolling, t=0.5", makeFrame(40, -1, false), makeFrame(44, -1, false), 0.5, makeFrame(42, -1, false), 8, 99);
		check("Scrolling, t=0.25", makeFrame(40, -1, false), makeFrame(44, -1, false), 0.25, makeFrame(41, -1, false), 8, 99);
		check("Scrolling, fast (8px/frame), t=0.5", makeFrame(40, -1, false), makeFrame(48, -1, false), 0.5, makeFrame(44, -1, false), 16, 98);

		//Sprite moves 6 pixels to the right over a flat background
		check("Sprite moving over flat background, t=0.5", makeFrame(-1, 100, false), makeFrame(-1, 106, false), 0.5, makeFrame(-1, 103, false), 0, 99.5);

		//Scrolling playfield with a static status bar at the top
		check("Scrolling with static status bar, t=0.5", makeFrame(40, -1, true), makeFrame(44, -1, true), 0.5, makeFrame(42, -1, true), 8, 98.5);

		//Camera scrolls while a sprite moves the other way
		check("Scrolling + moving sprite, t=0.5", makeFrame(40, 120, false), makeFrame(44, 112, false), 0.5, makeFrame(42, 116, false), 8, 98);

		//No motion: the output must be identical
		check("Static screen", makeFrame(40, 100, true), makeFrame(40, 100, true), 0.5, makeFrame(40, 100, true), 0, 100);

		//Flickering sprite (visible on one frame only): no ghost, the nearest real frame is shown
		check("Flickering sprite, t=0.25", makeFrame(-1, 100, false), makeFrame(-1, -1, false), 0.25, makeFrame(-1, 100, false), 0, 100);
		check("Flickering sprite, t=0.75", makeFrame(-1, 100, false), makeFrame(-1, -1, false), 0.75, makeFrame(-1, -1, false), 0, 100);

		//Speed: motion analysis (once per emulated frame) + generating 2.4 frames (144 Hz / 60 fps)
		{
			FrameInterpolator interpolator;
			vector<Frame> frames;
			for(int i = 0; i < 8; i++) {
				frames.push_back(makeFrame(40 + i * 3, 60 + i * 5, true));
			}
			auto start = std::chrono::steady_clock::now();
			int iterations = 300;
			for(int i = 0; i < iterations; i++) {
				RenderedFrame frame(frames[i % 8].Pixels.data(), w, h);
				interpolator.AddFrame(frame);
				RenderedFrame output;
				interpolator.GenerateFrameForTest(0.33, output);
				interpolator.GenerateFrameForTest(0.66, output);
				interpolator.GenerateFrameForTest(0.99, output);
			}
			double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / iterations;
			log << "Cost per emulated frame (analysis + 3 generated frames): " << StringUtilities::ToString(ms, 3) << " ms (" << StringUtilities::ToString(ms * 60 / 10, 1) << "% of one CPU core at 60 fps)\n";
		}

		log << (allPassed ? "PASS" : "FAIL") << "\n";
		return allPassed;
	}
}
