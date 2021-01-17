# ゼロから作るCDCL SATソルバ(bullsat)

## はじめに
充足可能性問題とは与えられた命題論理式が充足可能かまたは不可能かを判定をする問題です(`SAT`)
`SAT`はNP完全という問題に属し、効率的に解けるアルゴリズムが無いと一般的には考えられています。  
SATを解くソフトウェア`SATソルバ`の多くはConflict-Driven-Clause-Learning(`CDCL`)と呼ばれる、矛盾(`Conflict`)から新たな節(`Clause`)を導出し現問題に追加することにより、高速にSATを解くことができます。

本記事でゼロからCDCL SATソルバを実装しながらSATソルバの理解することを目的としており、SATソルバの必要最低限の機能とアルゴリズムだけを実装します。実装するSATソルバは`minisat`をベースとします。  
言語は`C++17`でコンパイラは`clang++`とします。環境は`Linux`または`macOS`を想定しています。
使用するライブラリは標準ライブラリ(`std`)のみだけとします。

```bash
% cat /etc/os-release
NAME="Ubuntu"
VERSION="20.04.1 LTS (Focal Fossa)"
ID=ubuntu
ID_LIKE=debian
PRETTY_NAME="Ubuntu 20.04.1 LTS"
VERSION_ID="20.04"
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
VERSION_CODENAME=focal
UBUNTU_CODENAME=focal

% clang++ --version
clang version 10.0.0-4ubuntu1 
Target: x86_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/bin
```
`C++17`の機能を使いますが、本記事では言語自体については説明いたしません。

## 準備
我々が作成するSATソルバは`bullsat`とします。SATソルバの機能としては最小限で賢くはなく問題が解けるまでプログラムは停止しません。
なのでひたすら突っ走る闘牛の`bull`と`sat`で`bullsat`です。いい名前ですね。
必要なファイルを準備しましょう。

