# Usage: python -I run_test.py <MesenCore.dll> <rom.nes> <log.txt> <homeFolder>
import ctypes, sys, os

dll_path, rom, log, home = (os.path.abspath(p) for p in sys.argv[1:5])
os.makedirs(home, exist_ok=True)
os.add_dll_directory(os.path.dirname(dll_path))
core = ctypes.CDLL(dll_path)
core.InitDll()
core.InitializeEmu.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_void_p] + [ctypes.c_bool] * 4
core.InitializeEmu(home.encode(), None, None, True, True, True, True)
core.RunRewindTest.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
core.RunRewindTest.restype = ctypes.c_bool

ok = core.RunRewindTest(rom.encode(), log.encode())
print(open(log).read())
sys.exit(0 if ok else 1)
