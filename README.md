# glyphlings（字母小兽）

给 6、7 岁孩子的打字游戏。要打的是一个小写英文字母，字母的形状是工作区里 `ink-*` 掩码投出来的。打对了，这只小兽就从蛋里长出来，并且写进正在运行的 Aura 工作区。打错了，源码一个字节都不变，屏幕多一行「再试一次」。

开头几只是一个字母。之后如果 `~/code/keys/deepseek` 里有钥匙，`make run` 会让 Aura 自己用 `http-post` 去问 DeepSeek V4.1 Flash（模型名 `deepseek-flash`）。模型只回一行 `单词|中文名|表情`。验过、并且不和已经出现过的词重复，才用 `mutate:replace-value` 写进正在跑的那条 `live`。没有钥匙，或者这一次没要到合格的词，就继续在猫、狗、蜜蜂、太阳、狐狸、猪、鸡、蚂蚁这 8 只里循环。按错的那个字母，下一只会换成含有它的单词。动物园只留最近的 8 只，这一局不收场。Ctrl+C 离开。

动物园、下一只小兽、身体、字母掩码，都是同一份 FlatAST。没有另一张场景表。Python、Rust、C 可以重画这些字，也可以自己去调这个接口。它们做不到的是：每按对一次，就地增量替换工作区里那一条 `live` 字面量（编译纪元跟着动），孵出来的时候再用 `hot-strategy:swap!` 把 `next-spawn` 换成已经在这份源码里的 lambda。新词走的是其中预先写好的 `body-coach-word`，单词本身留在 `live` 里，不写进 lambda。

## 玩

```bash
make run
```

屏幕最上面写着要按的那个字母，比如 `请按这个键  d`，下面是一块深色底上的小写字母。用英文输入，按键盘上的这一个键，大小写都可以。窗口至少 24 行，字母才放得下。按对了，工作区里那一条 `live` 字面量会增量替换，字母关大约两三百毫秒画出小兽。按下去会先闪一下「按到啦，等一等」。退格键收回刚刚长出来的一块。可以一直玩。Ctrl+C 离开。

进度在 `runtime/`。删掉这个目录就从头再来。`make test` 不会写这个目录。

`make line` 是给调试的：每敲一个字母再按回车。孩子玩用 `make run`。

## 启动契约

必须用本机已经编好的 Aura，本仓库不带编译器。

| 变量 | 值 |
|---|---|
| `AURA_BIN` | 默认 `/home/dev/code/grok-dev/aura-grok/build/aura` |
| `AURA_PATH` | 默认 `/home/dev/code/grok-dev/aura-grok/lib` |
| `AURA_SANDBOX` | `off`。默认 Restricted 在 mutation id 为 0 时会拒绝改写 |
| `AURA_PIPELINE_STRICT` | `force-soa`。生产默认会拒绝 `set-code` 的求值 |
| `LLM_API_KEY` | 只在 `make run` / `make line` 里、变量还空着时，从 `/home/dev/code/keys/deepseek` 读入。不写进仓库，不写进 `live` |
| `LLM_BASE_URL` | 默认 `https://api.deepseek.com` |
| `LLM_MODEL` | 玩的时候请求体里固定是 `deepseek-flash`，思考关掉 |
| `GLYPHLINGS_COACH` | `make run` / `make line` 默认 `1`。`make test` 固定 `0`，测试不联网 |

`make doctor` 在二进制不存在时用中文退出 127。

## 和 aura-typeplay 不是同一个东西

typeplay 的画面和词表在 Python 里，Aura 只改场景属性。这里的画面是 `(current-source :workspace)` 的投影。C 程序只把按键编成一行，不认识单词。

设计记录在 `docs/design.md`。
