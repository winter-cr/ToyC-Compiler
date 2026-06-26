# ToyC Compiler

ToyC Compiler 鏄竴涓绋嬪疄璺电敤鐨?ToyC 鍒?RV32 姹囩紪缂栬瘧鍣ㄣ€傜紪璇戝櫒浠庢爣鍑嗚緭鍏ヨ鍙?ToyC 婧愮▼搴忥紝鍚戞爣鍑嗚緭鍑哄啓鍑?RISC-V 32 浣嶆眹缂栵紱閿欒淇℃伅杈撳嚭鍒版爣鍑嗛敊璇€?
## 蹇€熸瀯寤?
鎺ㄨ崘浣跨敤 CMake锛?
```bash
cmake -S . -B build
cmake --build build
```

涔熸彁渚涙牴鐩綍 `Makefile`锛屾柟渚胯瘎娴嬬郴缁熺洿鎺ユ墽琛?`make`锛?
```bash
make
```

鐢熸垚鐨勭紪璇戝櫒鍙墽琛屾枃浠跺悕锛?
- CMake: `build/toyc`
- Makefile: `./toyc`

## 浣跨敤鏂瑰紡

```bash
./build/toyc < tests/performance/loop_sum.tc > output.s
./build/toyc -opt < tests/performance/loop_sum.tc > output.s
```

浣跨敤 Makefile 鏋勫缓鍚庯細

```bash
./toyc -opt < tests/performance/fib_heavy.tc > output.s
```

`-opt` 浼氬紑鍚?IR 浼樺寲銆佽烦杞竻鐞嗗拰姹囩紪绾х瀛斾紭鍖栥€?
## 椤圭洰缁撴瀯

```text
.
鈹溾攢鈹€ CMakeLists.txt          # CMake 鏋勫缓鍏ュ彛锛岄粯璁?Release
鈹溾攢鈹€ Makefile                # 閫氱敤 make 鏋勫缓鍏ュ彛锛岀敓鎴?./toyc
鈹溾攢鈹€ README.md               # 椤圭洰璇存槑
鈹溾攢鈹€ docs/                   # 瀹炶返鎶ュ憡鍜屾枃妗?鈹溾攢鈹€ scripts/                # 杈呭姪娴嬭瘯鑴氭湰
鈹溾攢鈹€ src/
鈹?  鈹溾攢鈹€ main.cpp            # 缂栬瘧鍣ㄥ懡浠よ鍏ュ彛
鈹?  鈹溾攢鈹€ lexer/              # 璇嶆硶鍒嗘瀽
鈹?  鈹溾攢鈹€ parser/             # 璇硶鍒嗘瀽鍜?AST 鏋勫缓
鈹?  鈹溾攢鈹€ ast/                # AST 鑺傜偣涓庢墦鍗?鈹?  鈹溾攢鈹€ semantic/           # 璇箟鍒嗘瀽銆佺鍙疯〃銆佸父閲忚〃杈惧紡姹傚€?鈹?  鈹溾攢鈹€ codegen/            # AST 鍒板悗绔?IR 闄嶄綆
鈹?  鈹斺攢鈹€ optimizer/          # 鍓嶇闆嗘垚灞備紭鍖?鈹溾攢鈹€ backend/
鈹?  鈹溾攢鈹€ include/toyc/backend/ # 鍚庣 IR銆佷紭鍖栧櫒銆丷V32 浠ｇ爜鐢熸垚鎺ュ彛
鈹?  鈹溾攢鈹€ src/                  # 鍚庣瀹炵幇
鈹?  鈹溾攢鈹€ tests/                # 鍚庣鍗曞厓娴嬭瘯
鈹?  鈹斺攢鈹€ examples/             # 鍚庣绀轰緥
鈹斺攢鈹€ tests/                  # 鍩虹銆佽涔夈€佹帶鍒舵祦鍜屾€ц兘鏍蜂緥
```

## 缂栬瘧娴佺▼

1. `Lexer` 灏?ToyC 婧愮爜杞崲涓?token 搴忓垪銆?2. `Parser` 鏋勫缓 AST銆?3. `SemanticAnalyzer` 妫€鏌ヤ綔鐢ㄥ煙銆佸父閲忋€佸嚱鏁拌皟鐢ㄣ€佽繑鍥炶矾寰勫拰鎺у埗娴佸悎娉曟€с€?4. `AstToIr` 灏?AST 闄嶄綆鍒颁笁鍦板潃 IR銆?5. `backend::optimize` 鍜?`DOptimizer` 鍦?`-opt` 涓嬫墽琛屽父閲忔姌鍙犮€佹涓存椂鍊煎垹闄ゃ€佽烦杞竻鐞嗙瓑浼樺寲銆?6. `RiscV32CodeGenerator` 鐢熸垚 RV32 姹囩紪銆?
## 褰撳墠浼樺寲

- 淇 `x-y` 杩欑被琛ㄨ揪寮忕殑璇嶆硶鍒嗚瘝锛岄伩鍏嶆妸鍑忓彿璇苟鍏ユ爣璇嗙銆?- 杩斿洖璺緞鍒嗘瀽鏀寔宓屽鍧椼€佸畬鏁?`if/else` 鍜屽父閲忕湡寰幆銆?- IR 姝讳复鏃跺€煎垹闄ゆ敼涓哄弽鍚戞椿璺冩€ф壂鎻忥紝閬垮厤閲嶅鍏ㄩ噺鎵弿瀵艰嚧缂栬瘧鏃堕棿澧為暱銆?- RV32 鍚庣灏嗗父鐢ㄥ眬閮ㄥ彉閲忋€佸弬鏁板拰涓存椂鍊煎垎閰嶅埌 `s1..s11`锛屽噺灏戝惊鐜拰閫掑綊涓殑鏍堣闂€?- 澶嶅埗銆佽繑鍥炲拰鏉′欢鍒嗘敮瀵瑰瘎瀛樺櫒鍊艰蛋蹇€熻矾寰勶紝鍑忓皯涓嶅繀瑕佺殑 `mv` 鍜?`lw/sw`銆?
## 璇勬祴鎻愪氦寤鸿

鎻愪氦浠撳簱鏃惰鍖呭惈浠ヤ笅鍐呭锛?
- `CMakeLists.txt` 鍜?`Makefile`
- `src/`銆乣backend/`銆乣tests/`銆乣scripts/`銆乣docs/`
- `README.md`
- `docs/瀹炶返鎶ュ憡.md`

涓嶈鎻愪氦 `build/`銆佹湰鍦板彲鎵ц鏂囦欢銆乼oken銆乣.env` 鎴?IDE 缂撳瓨銆

## Optimization Notes

- Tail recursion calls of the current function are lowered to a local jump when the argument list is small enough.
- Binary operations use immediate and direct-register RV32 forms when possible.
- Compare-plus-branch patterns are emitted as direct RV32 conditional branches.
