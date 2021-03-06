# 基本的な型とクラスの定義
$$
(x_1\lor\lnot x_3)\land(x_2\lor x_3\lor\lnot x_1)
$$
上記のようなSAT問題をプログラムが理解する形に表現する必要があります。`bullsat`では以下のように定義しました。

```cpp
namespace bullsat {
using Var = int; //命題変数: x1
struct Lit;　//リテラル: x1 or !x1
using Clause = std::vector<Lit>; //節: (x1 or !x2 or x3)
}
```
- 命題変数(`Var`) `int`
- リテラル(`Lit`) `struct`
- 節(`Clause`) `std::vector<Lit>`

### リテラルの実装
リテラルは`正`のリテラル\\(x_1\\)と`負`のリテラル\\(\lnot x_1\\)が存在します。  
正のリテラルは`偶数`、負のリテラルは`奇数`として表現します。

- 正のリテラル: \\(x_0\\)は`0`、\\(x_1\\)は`2`、\\(x_2\\)は`4`
- 負のリテラル: \\(\lnot x_0\\)は`1`、\\(\lnot x_1\\)は`3`、\\(\lnot x_2\\)は`5`

少しトリッキーなのですが、正/負のリテラルを正の整数として表現することでSATソルバのデータ構造や前処理で役に立ちます。

以下が`Lit`のコードです。また必要な関数も定義しました。

```cpp
// x is
// even: positive x0 (0 -> x0, 2 -> x1)
// odd: negative !x0 (1 -> !x0, 3 -> !x1)
struct Lit {
  int x;
  Lit() = default;
  Lit(const Lit &lit) = default;
  // 0-index
  // Lit(0, true) means x0
  // Lit(0, false) means !x0
  Lit(Var v, bool positive) {
    assert(v >= 0);
    x = positive ? 2 * v : 2 * v + 1;
  }
  bool operator==(Lit lit) const { return x == lit.x; }
  bool operator!=(Lit lit) const { return x != lit.x; }
  bool operator<(Lit lit) const { return x < lit.x; }
  bool pos() const { return !neg(); }
  bool neg() const { return x & 1; }
  Var var() const { return x >> 1; }
  size_t vidx() const { return static_cast<size_t>(var()); }
  size_t lidx() const { return static_cast<size_t>(x); }
};

// ~x0 = !x0
inline Lit operator~(Lit p) {
  Lit q(p);
  q.x ^= 1;
  return q;
}

std::ostream &operator<<(std::ostream &os, const Lit &lit) {
  os << (lit.neg() ? "!x" : "x") << lit.var();
  return os;
}

```

- `bool pos()`: `true`なら正のリテラル
- `bool neg()`: `true`なら負のリテラル
- `Var var()`: 命題変数を取得
正/負のリテラルから命題変数は現在の値から1ビット右にシフト(1/2)

- `Lit operator~(Lit p)`: リテラルの否定(\\(x_0\\) -> \\(\lnot x_0\\), \\(\lnot x_0\\) -> \\(x_0\\))

以下の関数は`Variable`や`Lit`をindexとした`std::vector`のデータ構造が必要となるため定義した。

- `size_t vidx()`: 命題変数に変換して`size_t`として取得
- `size_t lidx()`: リテラルを`size_t`として取得

### テスト
実装したリテラルに対してテストを`test.cpp`に追加しましょう。

```cpp
void test_lit() {
  test_start(__func__);
  Lit x0 = Lit(0, true); // x0
  Lit nx0 = ~x0;         // !x0
  assert(x0.lidx() == 0);
  assert(nx0.lidx() == 1);

  assert(x0.pos());
  assert(!x0.neg());
  assert(nx0.neg());
  assert(x0.pos());

  assert(x0.var() == nx0.var());

  assert(x0 != nx0);
  assert(~x0 == nx0);
  assert(x0.lidx() == (~nx0).lidx());

  Lit x1 = Lit(1, true); // x1
  assert(x1.lidx() == 2);
  assert(x0 < x1);
  assert(!x1.neg());
}

int main() {
  cerr << "===================== test ===================== " << endl;
  test_lit();
}
```

テストコードのコンパイルと実行をしてテストが通ることを確認しましょう。
```bash
% make test
./test
===================== test =====================
==================== test_lit ====================
```