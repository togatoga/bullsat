# 準備
我々が作成するSATソルバは`bullsat`とします。SATソルバの機能としては最小限で(現代SATソルバと比較して)賢くはなく、問題が解けるまでプログラムは停止しません。  
そこでひたすら突っ走る闘牛の`bull`と`sat`で`bullsat`です。いい名前ですね。
それでは必要なファイルを準備しましょう。
```bash
% mkdir bullsat
% cd bullsat
% touch bullsat.hpp test.cpp main.cpp Makefile
```
- `bullsat.hpp` SATソルバのコード
- `test.cpp` SATソルバのテストコード
- `main.cpp` SATソルバのコマンドラインapp
- `Makefile` release/debugビルドやテスト

各章では`bullsat.hpp`にSATソルバのアルゴリズムを実装し、並行して`test.cpp`にテストを書いていきます。SATソルバが完成した後に`main.cpp`にSATソルバをコマンドラインappとして実装します。
テスト自体は書かなくても問題ありませんが、以下の理由でテストを書くことをおすすめします。

1. テストコードを書くことで、実装したコードの理解を深める
2. SATソルバの実装は非常にバグを生み出しやすく、またバグの原因を探るのが非常に困難

任意ですがフォーマットツールを導入するのをオススメします。本記事では`clang-format`を使用します。  
各章では細かくテストコードを動かしたり、都度コンパイルするのでMakefileを用意しました。

```bash
APP=bullsat
CXX := clang++
CXXFLAGS := -std=c++17 -Weverything -Wno-c++98-compat-pedantic -Wno-missing-prototypes -Wno-padded
DEBUGFLAGS := -g -fsanitize=undefined

all: release debug

release: main.cpp bullsat.hpp
    mkdir -p build/release/
    $(CXX) $(CXXFLAGS) -O3 -DNDEBUG -o build/release/$(APP) main.cpp

debug: main.cpp bullsat.hpp
    mkdir -p build/debug/
    $(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o build/debug/$(APP) main.cpp

test: test.cpp bullsat.hpp
    $(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $@ test.cpp 
    ./$@

format:
    clang-format -i *.cpp *.hpp

clean:
    rm -rf build test *.o

.PHONY: all release test format clean
```
- `make release/debug` SATソルバのコマンドラインappをリリース/デバックビルド
- `make test` テストコードをコンパイルして実行
- `make format` ソースコードをフォーマット
- `make clean` 生成したバイナリやゴミを削除

