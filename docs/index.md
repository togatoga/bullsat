# ゼロから作るCDCL SATソルバ(bullsat)

## はじめに
充足可能性問題(satisfiability problem)とは与えられた二値(true/false)の命題論理式が充足可能(`SAT`)な変数の真偽値の割り当てが存在するか、またはそういった割り当てが存在しない充足不能(`UNSAT`)かを判定する問題です。  

- 命題変数(`Variable`) true/falseの二値変数
- リテラル(`Literal`) 命題変数またはその否定
- 節(`Clause`) 複数のリテラルをorで結んだ式
- 命題論理式 複数の節をandで結んだ式

以下の命題論理式を例に上げて考えてみましょう。
$$
(x_1\lor\lnot x_3)\land(x_2\lor x_3\lor\lnot x_1)
$$


SAT問題はNP完全という問題に属し、効率的に解けるアルゴリズムが無いと一般的には考えられています。  
SAT問題を解くソフトウェアを`SATソルバ`と呼ばれます。SATソルバの大きな性能向上として、Conflict-Driven-Clause-Learning(`CDCL`)が挙げられます。  
CDCLとは矛盾(`Conflict`)から矛盾の原因となった変数の割り当てを防ぐ新たな節(`Clause`)を導出して、探索の枝刈りを行うことで高速にSATを解くことができます。CDCLアルゴリズムによって扱える変数の数が増え、現実問題から派生した数十万~数百万の変数のSAT問題を解けるようになりました。

本記事でゼロからCDCL SATソルバを実装しながらSATソルバを理解することを目的としており、SATソルバの必要最低限の機能とアルゴリズムだけを実装します。
!!! Note
    本記事とSATソルバのコードは[GitHub](https://github.com/togatoga/bullsat)上で管理されています。pull requestは大歓迎です。

言語は`C++17`でコンパイラは`clang++`とします。環境は`Linux`または`macOS`を想定しています。
使用するライブラリは標準ライブラリ(`std`)のみだけとします。


以下は筆者の環境です。
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
それでは始めていきましょう。