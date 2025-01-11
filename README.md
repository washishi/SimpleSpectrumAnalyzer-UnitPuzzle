# Simple spectrum analyzer For UnitPuzzle
M5Atomシリーズ及びUnitPuzzleを利用した簡易的なスペクトラムアナライザーです  
Atomのスイッチをクリックするとバーの伸びる方向の上下が変わります  
また長押しするとバーの低音～高音の順番が逆になります

# 対応デバイス
- M5Atomシリーズ すべて (2025/01現在)  
S3R系で利用する場合はM5AtomS3でビルドしてください  
※Aボタンが無いExtやCAM系でも動きますがボタンによる表示変更はできません  
※AtomU系以外のマイクが無い物は下記のいずれかを併用します  
・[AtomMIC](github.com/washishi/atom_mic)  
・M5Stack の [PDMマイクユニット](https://docs.m5stack.com/ja/unit/pdm)  
・M5Stack の [Atmic Echo Base](https://docs.m5stack.com/ja/atom/Atomic%20Echo%20Base)  

# ビルド環境
- VSCode + PlatformIO  
- デフォルトではUnitPuzzleを4つ連結して使う設定になっています  
変更したい場合は [src/main.cpp](src/main.cpp) の21行目  
#define NUM_PUZZLE_UNIT 4 // Puzzle unit の数 1～16   
を変更してビルドしてください

# 使用ライブラリ及びバージョン
platformio.iniを参照してください

# LICENSE 
[MIT](https://github.com/washishi/SimpleSpectrumAnalyzer-UnitPuzzle/LICENSE)

# Author
[washishi](https://github.com/washishi)