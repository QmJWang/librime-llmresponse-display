# librime-llmresponse-display

一个 [RIME](https://github.com/rime/librime) / [Weasel](https://github.com/rime/weasel) 输入法插件：把预先生成好的「LLM 回复建议」以候选词的形式接入输入法，让你用打字/翻页候选的方式直接选出一条回复，一键上屏。

> **这个插件本身不调用任何 LLM，也不会读取微信 / IM 消息。**
> 它只是一个"候选展示层"：负责监听一个约定好的目录，把里面的 JSON 文件渲染成输入法候选并管理翻页/归档。生成这些 JSON 文件（比如接一个聊天软件的消息监听 + LLM API）是使用者自己的事，不在本仓库范围内。

## 效果

在任意 RIME/Weasel 方案里配置好触发键（默认 `Ctrl+R`）后：

1. 按下触发键：输入法扫描候选目录，如果有待处理的消息，弹出一组候选（最多 8 条），每条是一句完整的回复文本。
2. `Tab` / `Shift+Tab`：在"当前正在等你回复的联系人"之间切换（比如同时有 3 个人发了消息在排队）。
3. 用数字键或方向键选中某一条候选并上屏：这条消息被标记为"已处理"，对应的 JSON 文件会被移到 history 目录归档（每个联系人最多保留 20 条历史，超出自动清理最老的）。

## 它是怎么工作的

```
                       ┌─────────────────────────────┐
你自己写的脚本/服务      │  任意来源: 聊天消息监听         │
（不在本仓库内）          │  + 调用任意 LLM API 生成回复    │
                       └───────────────┬─────────────┘
                                       │ 写 JSON 文件
                                       ▼
                    <output_dir>/candidate/<联系人>/<时间戳>_<uuid>.json
                                       │
                                       │ Ctrl+R 触发扫描
                                       ▼
                       ┌─────────────────────────────┐
本插件（librime 插件）   │  Segmentor 识别触发哨兵串       │
                       │  Translator 把 JSON 内容渲染成   │
                       │  候选词，Tab 切换联系人           │
                       └───────────────┬─────────────┘
                                       │ 选中候选并上屏
                                       ▼
                    <output_dir>/history/<联系人>/<时间戳>_<uuid>.json
                                    （归档，每人最多 20 条）
```

插件由四个 librime 组件构成，各自职责很薄，状态全部集中在共享的 `LLMResponseEngine` 里：

| 组件 | 文件 | 作用 |
| --- | --- | --- |
| Processor | `src/llmresponse_processor.*` | 监听触发键 / `Tab` 切换 / 监听提交事件（选中后归档） |
| Segmentor | `src/llmresponse_segmentor.*` | 识别 Processor 塞入输入串的哨兵字符串，划出专属 segment |
| Translator | `src/llmresponse_translator.*` | 只在上述 segment 上生效，把候选数据转成 `Candidate` 列表 |
| Engine | `src/llmresponse_engine.*` | 共享状态：扫描候选目录、维护当前联系人指针、选中后移动文件归档 |
| Db | `src/llmresponse_db.*` | 按 `mtime` 惰性解析单个 JSON 文件，避免重复 IO |

三个组件（Processor / Segmentor / Translator）按 schema_id 共用同一个 `LLMResponseEngine` 实例，保证 `Tab` 翻页和候选展示的状态是一致的。

## 数据格式

### 目录结构

```
<output_dir>/
  candidate/
    <联系人 ID>/
      <毫秒时间戳>_<uuid8>.json   # 待处理消息，文件名决定排序
      ...
    <另一个联系人 ID>/
      ...
  history/
    <联系人 ID>/
      <毫秒时间戳>_<uuid8>.json   # 已处理，自动归档，每人最多保留 20 条
```

- `<联系人 ID>` 只是一个目录名，用来把同一个人/同一个会话的多条待处理消息分组；具体填什么（微信 wxid、手机号、随便一个昵称）完全由生成数据的一方决定，插件本身不关心内容。
- 文件名建议用 `<毫秒时间戳>_<8位随机串>.json`，插件按文件名字典序排序（约等于时间顺序），不依赖文件系统的创建时间。
- 一个 JSON 文件 = 一条待回复的消息 + 一组回复候选。

### JSON 字段

```json
{
  "title": "周末一起吃饭吗？",
  "is_question": true,
  "responses": [
    { "text": "好啊，周末有空，你想吃什么？", "label": "接受-随口问", "angle": "accept" },
    { "text": "这周末不太方便，下周可以吗？", "label": "婉拒-改约", "angle": "decline_reschedule" },
    { "text": "好呀，老地方见？", "label": "接受-延续习惯", "angle": "accept_default" }
  ]
}
```

| 字段 | 类型 | 必填 | 说明 |
| --- | --- | --- | --- |
| `title` | string | 否 | 对方发来的原始消息，仅作为候选的"提示/comment"展示，不参与匹配逻辑 |
| `is_question` | bool | 否 | 预留字段，当前插件只读取不使用，可用于自己的上游逻辑区分是否需要回复 |
| `responses` | array | 是 | 回复候选列表，**最多渲染前 8 条**（对应 schema 里 `menu/page_size: 8`，建议数据源本身也只生成 ≤8 条） |
| `responses[].text` | string | 是 | 完整回复文本，选中后原样上屏 |
| `responses[].label` | string | 否 | 候选的简短标签，会作为 `Candidate` 的 `comment` 展示在候选列表里 |
| `responses[].angle` | string | 否 | 预留字段，标记这条回复的"角度"（接受/婉拒/追问……），当前插件不使用，方便自己在生成数据时做分类调试 |

完整示例见 [`example/candidate/alice/1731400000000_a1b2c3d4.json`](example/candidate/alice/1731400000000_a1b2c3d4.json)。

如果 `title` 和 `responses` 同时为空，或者文件解析失败（比如生成脚本正在写入、文件不完整），插件会当作"暂时不可用"处理，不会展示半份数据，等下一次扫描（下一次按触发键）再重新读取。

## 安装

1. 克隆本仓库到你的 librime 源码的 `plugins/` 目录下，目录名会成为插件名的一部分，建议直接用：

   ```sh
   cd librime/plugins
   git clone <本仓库地址> librime-llmresponse
   ```

2. 按 librime / Weasel 常规流程重新编译（插件目录一般会被构建脚本自动发现，具体取决于你使用的是 [librime](https://github.com/rime/librime) 还是 [Weasel](https://github.com/rime/weasel) 的构建方式，参考各自仓库的插件安装说明，通常是 `plugins.list` 或直接放入 `plugins/` 目录即可）。

3. 编译产物会注册一个 `llmresponse` module，导出 `llmresponse_segmentor` / `llmresponse_processor` / `llmresponse_translator` 三个组件，供 schema 的 `patch` 配置引用（见下）。

> Windows/MSVC 用户注意：源码里的中文注释没有 BOM，`CMakeLists.txt` 已经显式加了 `/utf-8` 编译选项，正常 `cmake --build` 即可，不需要额外处理。

## 配置（schema yaml）

在你的输入方案的 `.custom.yaml` 里（参考 [`data/luna_pinyin.custom.yaml`](data/luna_pinyin.custom.yaml)）加上：

```yaml
patch:
  # processor 建议放在 ascii_composer 之后，避免触发键被别的按键绑定抢先处理
  'engine/processors/@after 0': llmresponse_processor
  # segmentor 必须排在最前面（abc_segmentor 之前），否则触发哨兵串尾部的
  # 合法拼音字母会被 abc_segmentor 先抢走，凑不出完整哨兵串
  'engine/segmentors/@before 0': llmresponse_segmentor
  'engine/translators/@before 0': llmresponse_translator

  # 一次最多展示 8 条候选（对应数据里 responses 建议 ≤ 8 条）
  menu:
    page_size: 8

  # 三个组件共用的配置，必须放在 llmresponse 这个 key 下面（不要挪到某个
  # 具体组件的注册名下），因为三边通过固定 key 找到同一份配置
  llmresponse:
    output_dir: "C:/rime/llm_outputs"   # 换成你自己的候选数据根目录，建议绝对路径

  # 触发键，可选，默认就是 Control+r
  llmresponse_processor:
    trigger_key: "Control+r"
```

配置项说明：

- `llmresponse/output_dir`：候选数据根目录（对应上面「数据格式」章节的 `<output_dir>`）。插件内部会拼接 `output_dir/candidate/...` 和 `output_dir/history/...`，这里只填根目录本身。
- `llmresponse_processor/trigger_key`：触发扫描并弹出候选的按键，语法与 RIME 按键绑定一致（例如 `Control+r`、`Alt+grave` 等）。
- 如果你的方案本身已经占用了 `Ctrl+R`，改这个键即可，不影响插件其他行为。

保存后按 Weasel 的部署方式重新部署（或者 `librime` 场景下重新加载配置）生效。

## 使用方法

1. 正常打字，光标在空白输入状态时按下触发键（默认 `Ctrl+R`）。
2. 有待处理消息时会弹出候选：第一行/第一条通常配合 `title` 作为提示，后面跟着最多 8 条回复候选，每条候选后面的括号内容是 `label`。
3. `Tab` 切换到下一个待处理联系人，`Shift+Tab` 切换到上一个。
4. 数字键 / 方向键 + 回车选中某条候选并上屏：这次消息处理完毕，对应文件被移到 `history` 目录归档，不会再出现在候选列表里。
5. 如果目录里没有任何待处理文件，触发键不会有任何反应（直接放行给正常打字流程）。

## 常见问题

**为什么候选没反应 / 按了 Ctrl+R 没弹出来？**
按触发键的瞬间才会重新扫描目录，请确认：`output_dir` 路径正确、目录结构是 `output_dir/candidate/<联系人>/*.json`（不是直接把 json 扔进 `output_dir` 根目录）、JSON 内容合法且 `responses` 非空。

**候选内容为什么没刷新？**
`Db` 是按文件 `mtime` 判断要不要重新解析的，同一个文件只要 `mtime` 没变就不会重新读；如果你在外部脚本里更新了同名文件的内容，确认文件系统真的更新了 `mtime`（多数写入方式会自动更新，除非手动改了时间戳）。目录本身的增减需要重新按一次触发键才会重新扫描（`ScanPending()` 只在触发键按下、或者候选被选中后自动执行）。

**能不能一次展示超过 8 条？**
把 schema 里的 `menu/page_size` 调大即可，插件本身没有硬编码上限，只是渲染 `responses` 里的全部内容；`page_size: 8` 只是配套的默认展示建议。

**我要怎么生成 `candidate/` 目录里的 JSON？**
这部分完全由你自己实现，可以是任何语言写的脚本：监听某个聊天工具的新消息，调用任意 LLM（本地模型 / OpenAI / Claude API 等都行）生成几种角度的回复，按上面的 JSON 格式写文件即可。写文件时建议先写到临时文件名再 rename 到最终文件名，避免插件在写入过程中读到不完整的 JSON（虽然插件对空内容有基本的兜底，但不保证能识别所有"写了一半"的情况）。

## 目录结构

```
.
├── CMakeLists.txt              # librime 插件构建入口
├── data/
│   └── luna_pinyin.custom.yaml # 配置示例（针对 luna_pinyin 方案，可套用到任意方案）
├── example/
│   └── candidate/alice/...     # 一个候选 JSON 文件的完整示例
└── src/
    ├── llmresponse_constants.h    # 触发哨兵串常量
    ├── llmresponse_db.*           # 单个 JSON 文件的懒加载解析
    ├── llmresponse_engine.*       # 共享状态：扫描/翻页/归档
    ├── llmresponse_module.cc      # librime module 注册入口
    ├── llmresponse_processor.*    # 触发键 / Tab 切换 / 选中后归档
    ├── llmresponse_segmentor.*    # 识别触发哨兵串，划分 segment
    └── llmresponse_translator.*   # 把候选数据渲染成 Candidate
```

## License

[MIT](LICENSE)


