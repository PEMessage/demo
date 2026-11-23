# 从 byte[N] 到 MIFARE Classic

## 一张存储卡的第一性原理

> 这篇文章整理自一条真实的问答链：从「MIFARE Classic 是不是 CPU 卡」开始，一路问到「为什么它不是简单的 `byte[N]` 大数组」，最后落成一个可运行的 Python 模型。
>
> 目标不是背产品参数，而是建立一套能解释大多数非接触式卡片的建模语言。
>
> 配套代码：[`mifare_classic.py`](./mifare_classic.py)

---

## 目录

- [0. 先回答三个最容易混淆的问题](#0-先回答三个最容易混淆的问题)
- [1. 非接触卡简史](#1-非接触卡简史)
- [2. 为什么需要第一性原理建模](#2-为什么需要第一性原理建模)
- [3. 八级阶梯：从 byte\[N\] 到 Token](#3-八级阶梯从-byten-到-token)
- [4. 把 MIFARE Classic 拼起来](#4-把-mifare-classic-拼起来)
- [5. 模型批判：四个容易犯的错](#5-模型批判四个容易犯的错)
- [6. Python 实现](#6-python-实现)
- [7. 访问位的真实编码](#7-访问位的真实编码)
- [8. 三条设计定律](#8-三条设计定律)
- [9. 结语：卡片史是一部授权下沉史](#9-结语卡片史是一部授权下沉史)
- [附录 A：时间线](#附录-a时间线)
- [附录 B：术语表](#附录-b术语表)
- [附录 C：参考资料](#附录-c参考资料)

---

## 0. 先回答三个最容易混淆的问题

### 0.1 MIFARE Classic 是 CPU 卡吗？

**不是。** 它是一张**逻辑加密存储卡**。

它的芯片里没有 CPU、没有操作系统、没有可执行程序。它的全部能力是：

- 一块按扇区切分的 EEPROM；
- 每扇区一组密钥（KeyA / KeyB）；
- 一组访问位，决定「哪种钥匙能做哪些操作」；
- 一个简单的认证状态机。

所以它更像「带密码锁的存储柜」，而不是「一台微型计算机」。

判断一张卡是不是 CPU 卡，不要看它「有没有芯片」，而要看：

> **它能不能执行程序、做动态加密运算、运行文件系统或应用。**

按这个标准：

| 类别 | 代表 |
|---|---|
| 存储卡 / 逻辑加密卡 | MIFARE Classic、MIFARE Ultralight |
| CPU 卡 | MIFARE DESFire、MIFARE Plus、MIFARE Pro/Pro X、FeliCa |

所以「只有 MIFARE DESFire 是 CPU 卡」这个说法**不准确**。Plus 和 Pro 系列也是 CPU 卡，FeliCa 更是典型的 CPU 卡。

---

### 0.2 FeliCa 和 Ultralight 又是什么？

它们分属两个完全不同的阵营。

**FeliCa：索尼的 CPU 卡。**

- 1988 年前后开始研发，1997 年随香港八达通商用；
- 芯片内含 8 位 RISC 处理器、FeliCa OS 和加密协处理器；
- 支持动态密钥交换和 DES/3DES，安全性建立在主动计算上；
- 通信速率最高 424 kbps，是 Classic（106 kbps）的四倍；
- 使用**专有命令集**，不是 APDU；
- 典型应用：八达通、Suica、Edy。

**MIFARE Ultralight：NXP 的超低成本存储卡。**

- 2001 年推出，定位是替代纸票的一次性/有限次场景；
- 基础型号是 16 页 × 4 字节，页 0–1 是 UID，页 2 是锁字节，页 3 是 OTP，页 4–15 是用户区；
- 没有 CPU，没有操作系统；
- 后续变体 Ultralight C（3DES）、EV1（计数器/认证）、AES（AES-128）都是在「存储 + 轻量安全」上做增强；
- 典型应用：单程票、活动腕带、一次性通行证。

一句话：

> **FeliCa 是「会算的卡」，Ultralight 是「便宜到可以扔的卡」，Classic 是夹在中间的「带锁的存储卡」。**

---

### 0.3 MIFARE 家族是怎么长出来的？它是最早的非接卡吗？

先纠正一个常见误解：**MIFARE 不是非接触式技术的发明者。**

非接触通信的构想可以追溯到更早：

- 1948 年，Harry Stockman 发表《利用反射功率的通信》，奠定 RFID 的理论基础；
- 二战期间的 IFF 系统是技术雏形；
- 1967 年，德国工程师 Helmut Gröttrup 的专利已经描述了电感耦合的非接触通信；
- 索尼在 1988 年前后就开始研发后来成为 FeliCa 的技术。

MIFARE 的真正贡献是：**在正确的时间，以可大规模量产的成本，把非接触 IC 卡推向了公共交通和门禁。**

它的名字来自 **MI**kron **FARE**-collection System。1994 年由 Mikron Group 开发，同年飞利浦（后来的 NXP）推出首款产品，并成为 ISO/IEC 14443 Type A 的基础。

家族演化大致是：

```
1994  Classic 1K        —— 逻辑加密存储卡，家族的奠基产品
1996  首尔地铁商用       —— 第一个大规模部署
1997  MIFARE Pro        —— 早期 CPU 卡探索，3DES 协处理器
1999  MIFARE Pro X      —— 加入 PKI
2001  Ultralight        —— 极致低成本分支
2002  DESFire           —— 真正的 CPU 卡起点
2006  DESFire EV1       —— 首个支持 AES-128 的型号
2008  MIFARE Plus       —— Classic 的 AES 升级路径
2008  Ultralight C      —— 3DES
2012  Ultralight EV1    —— 计数器与密码认证
2013  DESFire EV2       —— 性能与隐私增强
2020  DESFire EV3       —— CC EAL5+
2022  Ultralight AES    —— AES-128
```

注意：这不是一条单线进化，而是**两个分支 + 一条升级路径**：

- **低成本分支**：Ultralight；
- **基础安全分支**：Classic；
- **高安全分支**：DESFire / Plus / Pro。

所以「Classic → Ultralight → DESFire」这个顺序**在时间上大致成立，在谱系上不成立**。它们更像同一家族里三个不同生态位，而不是祖孙三代。

---

### 0.4 各种非接卡都用 APDU 吗？

**不是。** 是否使用 APDU，取决于卡片有没有 ISO/IEC 14443-4 及以上的协议层。

APDU 是 ISO 7816-4 定义的应用层命令格式：

```
CLA INS P1 P2 Lc Data Le
```

它假设卡片和读卡器之间已经建立了一个可靠的消息通道。Classic 和 Ultralight 没有这一层，所以它们只能使用专有命令。

| 技术 | 年代 | CPU | 协议层 | 软件原语 | 典型场景 |
|---|---|---|---|---|---|
| MIFARE Classic | 1994 | 否 | ISO 14443-3 Type A | 认证 / 读 / 写 / 值操作（专有） | 门禁、公交、校园卡 |
| MIFARE Ultralight | 2001 | 否 | ISO 14443-3 Type A | 页读写 / 锁 / OTP / 计数器（专有） | 单程票、腕带 |
| MIFARE DESFire | 2002 | 是 | ISO 14443-4 | APDU 风格的文件/应用命令 | 高安全门禁、交通、支付 |
| MIFARE Plus | 2008 | 是 | ISO 14443-3/4 | Classic 兼容 → AES / APDU | Classic 升级 |
| FeliCa | 1988/1997 | 是 | JIS X 6319-4 | 专有高速命令 | 八达通、Suica、Edy |
| NFC / SE | 2002/2004 | 视实现 | ISO 18092 / 14443 | APDU + AID | 手机支付、HCE |

一句话：

> **APDU 是 CPU 卡时代的通用语言；存储卡时代是各自的方言。**

---

## 1. 非接触卡简史

### 1.1 理论前史：1940s–1980s

非接触 IC 卡的技术基础是 RFID：通过无线电波进行非接触数据通信。

- 1948 年 Stockman 的论文奠定了理论基础；
- 二战 IFF 系统提供了技术雏形；
- 1950–1980 年代，RFID 主要停留在实验室和动物追踪等早期工业应用，还没有和「卡片」形态结合。

### 1.2 1990s：卡片形态的确立

1990 年代初，RFID 与 IC 卡技术融合，非接触 IC 卡正式诞生。关键变化是工作频率迁移到 **13.56 MHz**，为全球标准化奠定基础。

- **1994**：MIFARE Classic 1K 推出；ISO/IEC 14443 的标准化工作也在这一年启动。
- **1996**：首尔地铁大规模商用，Classic 成为交通卡和门禁的主流选择。
- **1997**：FeliCa 随八达通商用；ISO/IEC 15693 标准化工作启动。
- **1997/1999**：MIFARE Pro / Pro X 探索 CPU 卡方向。

### 1.3 2000s：标准化与安全分化

- **2001**：Ultralight 推出，主打极致低成本。
- **2002**：DESFire 推出，真正的 CPU 卡起点；同年飞利浦和索尼联合开发 NFC。
- **2004**：NFC 论坛成立。
- **2006**：DESFire EV1，首个支持 AES-128 的型号。
- **2008**：MIFARE Plus 推出，作为 Classic 的 AES 升级路径；同年 Crypto-1 被公开攻破。
- **2008**：Ultralight C 引入 3DES。

### 1.4 2010s 至今：NFC 与移动化

- **2012**：Ultralight EV1。
- **2013**：DESFire EV2。
- **2014**：Apple Pay 推动 NFC 在手机中的普及。
- **2020**：DESFire EV3，获得 CC EAL5+。
- **2022**：Ultralight AES。

形态从「塑料卡片」逐渐转向「手机里的一个端点」，最终又转向「云端的一次性证明」。

---

## 2. 为什么需要第一性原理建模

### 2.1 盖尔定律

> **一个能工作的复杂系统，总是从一个能工作的简单系统演化而来。**
> **从零设计的复杂系统通常不能工作，也无法修补到能工作；你必须从一个能工作的简单系统重新开始。**
>
> —— John Gall，《Systemantics》

这条定律解释了为什么 MIFARE Classic 的设计看起来「不优雅」：

- 为什么权限粒度是扇区，而不是任意字节？
- 为什么密钥和权限藏在每个扇区的最后一块里？
- 为什么访问位是 3 字节的固定编码，而不是一个通用 ACL？
- 为什么 KeyA 永远读不出来？

因为这些都不是「从零设计的最优解」，而是**在无 CPU、极低成本、EEPROM 页结构、ISO 14443 协议、既有生态等约束下，一步步长出来的可行折中**。

### 2.2 byte[N] 是数据面，不是完整系统

一个 `byte[N]` 大数组只回答了「数据放在哪」。但任何真实系统还必须回答四个问题：

```
谁？            → 身份 / 密钥
现在能干什么？  → 认证状态
允许干什么？    → 策略
什么时候生效？  → 提交语义
```

`byte[N]` 一个都答不了。你一旦给它补上这四个答案，它就不再是 `byte[N]`：

- 补在硬件里 → 尾块 + 访问位（Classic）；
- 补在文件系统里 → AID + ACL（DESFire / SE）；
- 补在服务端 → Token + cryptogram（Apple Pay）。

所以：

> **`byte[N]` 是「没有权力的存储」。卡片的历史，就是给存储逐步加上权力、再把权力挪到更合适的层的过程。**

### 2.3 阶梯是设计空间，不是产品年表

接下来的八级阶梯，是一条**设计空间的推导链**，不是严格的产品发布时间线。

比如：

- Classic（1994）比 Ultralight（2001）早，它不是从 Ultralight 演化来的；
- Classic 没有 OTP 锁位，它用「密钥 + 访问位」替代了静态锁；
- Classic 的 value block 不是 OTP 计数器，它支持 Increment，不是单向的。

阶梯的作用是：**每一步都问「上一代暴露了什么问题，最小增量是什么」**。这比背产品参数更能解释设计为什么长这样。

---

## 3. 八级阶梯：从 byte[N] 到 Token

### Step 0：裸存储卡 —— `byte[N]`

**问题**：我需要通过射频读写一小块 EEPROM，存一个 ID 或少量数据。

**最小可行系统**：

```pseudo
type Memory = Byte[N]

function read(addr) -> Byte {
    return Memory[addr]
}

function write(addr, val) -> OK {
    Memory[addr] = val
    return OK
}
```

**能力**：按地址读写任意字节。

**第一性原理抽象**：`Card = byte[N] + read + write`。

**暴露的新问题**：任何人拿到读卡器都能改任意字节。对于门禁、票务，这等于没有安全。

---

### Step 1：锁位与 OTP —— 给存储加静态保护

**问题**：某些字节在某个时刻之后，永远不能再改。

#### 1.1 锁位：一个 bit 管一段地址

不引入密钥，不引入身份，只加一排开关：

```pseudo
LockBits = Bool[M]          // 每个 bit 管一段连续地址（region）

function write(addr, val):
    region = addr / REGION_SIZE
    if LockBits[region]:
        return DENIED
    Memory[addr] = val
    return OK

function lock(region):
    LockBits[region] = true     // 只能 true，没有 unlock
```

三个性质：

1. 检查发生在写之前；
2. 单向，只能 `false → true`；
3. 与身份无关。

**解决了什么**：发行方可以在发卡前锁死配置区/密钥区，之后连自己都改不了。

**没解决什么**：

- 锁之前谁都能改，锁之后谁都不能改；
- 粒度粗；
- 不能「验证通过才放行」。

#### 1.2 OTP：只能写一次的位

OTP = One-Time Programmable。物理上是一排「按下去就弹不回来」的开关：

```pseudo
OTP = Bool[K]        // 出厂全 0

function burn(i) -> Result {
    if OTP[i]: return DENIED
    OTP[i] = true   // 0 → 1，物理不可逆
    return OK
}
```

没有 `writeOTP(i, false)`，也没有 `clear()`。

锁位本身通常就是用 OTP 位实现的——只是它没有暴露给你读写，而是接到了「禁止写入」的逻辑上。

#### 1.3 锁位和 OTP 是正交的

它们是同一个原语的两种用法：

| | 单向性用在哪 | 作用 |
|---|---|---|
| 锁位 | 控制面 | 决定「别人还能不能写这块数据」 |
| OTP | 数据面 | 它自己就是数据，记录「这件事发生过」 |

所以完全可以只用一半：

| | 没有 OTP 数据区 | 有 OTP 数据区 |
|---|---|---|
| 没有锁位 | Step 0：裸存储 | 数据可改 + 一排不可擦的标记 |
| 有锁位 | 先可反复写，最后永久只读 | 典型 Ultralight：锁配置 + 烧使用标记 |

**暴露的新问题**：锁位和 OTP 都只会说「能 / 不能」，不会说「是谁」。

---

### Step 2：计数器 —— 把「发生过」变成「还剩多少」

**问题**：单程票要回答「还能用几次」，而不是「这一位烧了没有」。

**关键一步**：把一排 OTP 位从「K 个独立标记」改成「一个数的刻度」：

```
0 0 0 0   → 还剩 4 次
1 0 0 0   → 还剩 3 次
1 1 0 0   → 还剩 2 次
1 1 1 0   → 还剩 1 次
1 1 1 1   → 还剩 0 次，用完
```

```pseudo
OTP = Bool[4]

function remaining() -> int {
    return count(OTP[i] == false)
}

function decrement() -> Result {
    if remaining() == 0: return DENIED
    i = first index where OTP[i] == false
    burn(i)          // 复用 Step 1 的 burn
    return OK
}
```

**性质**：

1. 只能减，不能加；
2. 到 0 自动锁死；
3. 一次减法 = 烧一位 = 原子操作。

**代价**：容量 N 次需要 N 个位。早期芯片用这种「温度计码」，后来成本允许了才出现「二进制计数器 + 一次性递减逻辑 + 备份位」的做法。

**暴露的新问题**：谁有资格减？

---

### Step 3：认证 —— 从「能不能」到「谁可以」

**问题**：任何人都能烧你的余额。

**错误的最小做法**：把密钥一起发过去。空中的无线电谁都能听，密钥一旦发出去就泄露了。

**正确的最小机制：挑战—应答。**

```pseudo
1. 卡 → 终端：  随机数 c
2. 终端：       r = f(key, c)
3. 终端 → 卡：   r
4. 卡：         自己算 f(key, c)，比对
                相等 → authed = true
```

密钥从头到尾没有上过空口；空中传的 `r` 每次都不同，录下来重放也没用。

```pseudo
struct Card {
    key:    Byte[4]
    authed: Bool = false
}

function authenticate() -> Result {
    c = random()
    send(c)
    r = receive()
    if r == f(key, c):
        authed = true
        return OK
    return FAIL
}

function decrement() -> Result {
    if !authed: return DENIED
    ...
}
```

**这一步真正改变了什么**：卡片第一次有了**运行时状态**。

```
未认证 ──认证成功──► 已认证
   ▲                    │
   └──── 失败/复位 ──────┘
```

`authed` 不在 `byte[N]` 里，也不在任何 OTP 位里。它断电就没了。

**暴露的新问题**：`authed` 只是一个布尔开关，它记录的是「这张卡被打开过」，不是「被谁打开」。同一次上电场里，任何能发命令的一方都会通过。

#### Step 3.5：会话密钥 + 每命令 MAC（可选加固）

正确的做法不是只留一个 `true`，而是留一把只有双方知道的会话密钥：

```pseudo
// 认证时
sessionKey = KDF(key, c)
counter    = 0

// 之后的每条命令
function decrement(mac, n) {
    if !authed: return DENIED
    if n != counter: return DENIED
    if mac != MAC(sessionKey, n, "dec"): return DENIED
    counter++
    burnOneBit()
}
```

这样卡片验证的就不再是「现在有人在说话」，而是「这条命令来自刚才那个对端」。

---

### Step 4：扇区与访问位 —— 把开关拆细

**问题**：一把钥匙开全卡；认证一次，全卡可写。

**第一刀：按空间拆。**

```pseudo
struct Sector {
    keyA:    Key
    keyB:    Key
    blocks:  Block[4]
    authed:  KeyType?      // null / A / B
}

function authenticate(s, keyType, candidate):
    if candidate != keyFor(sectors[s], keyType):
        return FAIL
    sectors[s].authed = keyType     // 只改这一个扇区
```

关键变化：`authed` 从「卡的属性」变成了「扇区的属性」。认证扇区 3，不会让扇区 4 打开。

**第二刀：按操作拆。**

真实需求是：门禁区能读不能写，密钥区根本不能读，余额区能减不能加。于是每块再挂一张权限表：

```
access.can(keyType, op, block) -> bool
```

三个输入：用哪把钥匙、想做什么、对哪个块。

**这两样东西放哪？—— 尾块。**

每个扇区固定留最后一块，不存业务数据，专门存控制信息：

```
block 0 : data
block 1 : data
block 2 : data
block 3 : trailer = KeyA | access bits | user byte | KeyB
```

为什么放这里？因为地址译码最便宜：块地址的高位选扇区、低位选块，最后一块固定就是 trailer。这不是优雅设计，是省门电路。

**暴露的新问题**：

1. 控制面寄生在数据面里，规则和它保护的数据住在一起，而且规则本身可写；
2. Crypto-1 太弱；
3. 粒度还是扇区级；
4. 没有应用抽象，地址还是物理块号。

---

### Step 5：DESFire —— CPU + 文件系统 + APDU

**问题**：Classic 的粗粒度、弱加密、无应用抽象，不适合金融和高安全场景。

**最小增量**：给卡里塞一个 CPU。

```pseudo
struct App {
    aid:    AID
    files:  File[]
    keys:   KeySet
    auth:   AuthState
}

struct File {
    type:   FileType
    access: AccessCondition
    data:   Block[]
}

function dispatch(apdu):
    app = route(apdu.aid)
    return app.process(apdu)
```

能力：

- 多应用隔离；
- 文件系统；
- AES/3DES；
- 标准 APDU；
- 动态挑战—应答。

**代价**：成本高；APDU 开销比专有命令大；对极高速场景仍然不够快。

---

### Step 6：FeliCa —— 为速度特化的 CPU 卡

**问题**：日本交通闸机需要在人流高峰每秒处理大量交易，要求极低延迟和高速通信。

**最小增量**：保留 CPU 卡的安全架构，但特化命令集和通信协议。

```pseudo
struct FeliCa {
    CPU + OS,
    areas: Area[],
    keys:  KeySet,
    auth:  { authed: bool },
}

function processFeliCaCommand(cmd: FeliCaFrame) -> FeliCaFrame {
    switch cmd.code {
        case READ_WITHOUT_ENCRYPTION: return readArea(...)
        case READ_WITH_ENCRYPTION:    return readEncrypted(...)
        case AUTHENTICATE:            return cryptoChallengeResponse(...)
    }
}
```

**能力**：CPU 卡的安全级别 + 专有高速命令集 + 424 kbps。

**代价**：专有生态，互操作性差。

---

### Step 7：NFC + SE —— 卡从「内存」变成「端点」

**问题**：手机里要同时放门禁、公交、银行卡，不能每张卡一颗芯片。

**最小增量**：不再让读卡器直接寻址存储，而是把「卡」抽象成一个**消息端点**。

```pseudo
struct SecureElement {
    apps:     Map<AID, App>
    selected: AID?
}

function dispatch(apdu: APDU) -> APDU {
    app = route(apdu, selected)
    return app.process(apdu)
}
```

**真正的断裂点**：寻址方式从「地址」变成了「消息」。

- Classic：`read(sector=3, block=1)` —— 你在访问内存；
- DESFire/SE：`SELECT AID` 然后 `READ FILE 0x02` —— 你在调用一个能力。

**暴露的新问题**：状态被切碎了。SE 里有认证状态，手机 OS 里有选择状态，读卡器里有会话状态。谁拥有「当前是谁、授权到哪一步」？

---

### Step 8：Tokenization / HCE —— 卡从「端点」变成「证明」

**问题**：卡号不能暴露给商户；不能每台设备一张实体卡；丢了要能立刻吊销。

**最小增量**：卡片不再保存「余额/凭证」，只保存密钥 + 计数器；每次交易现场算出一个一次性密码。

```pseudo
struct Token {
    dpan:    PAN
    key:     LimitedUseKey
    counter: uint32
    expiry:  Timestamp
}

function authorize(tx: Tx) -> Cryptogram {
    c = MAC(key, counter++, tx.amount, tx.merchant, tx.nonce)
    return c
}
```

**本质**：存储消失了，剩下的是**授权证明**。卡不再回答「我里面有什么」，只回答「我能不能为这笔交易作证」。

于是阶梯出现了一个回环：

```
byte[N] → 加锁 → 加状态机 → 加文件系统 → 加 CPU → 又把状态搬回服务器
```

不是倒退，而是把状态放到了能承受复杂策略的那一层。

---

### 阶梯总览

| 步骤 | 能工作的简单系统 | 新增原语 | 隐藏状态 | 控制面位置 | 暴露的新问题 |
|---|---|---|---|---|---|
| 0 裸存储 | `byte[N]` | read / write | 无 | 无 | 无访问控制 |
| 1 锁位 / OTP | `byte[N]` + 锁 / OTP | lock / burn | 无 | 存储阵列旁 | 无身份 |
| 2 计数器 | + Counter | decrement | 无 | OTP 区 | 只解决消耗 |
| 3 认证 | + Key | authenticate | 全局 auth | 密钥区 | 会话不绑定 |
| 4 Classic | 扇区 + 尾块 | auth / 读写 / 值操作 | 每扇区 auth | 尾块（嵌入数据面） | Crypto-1 弱，控制面可写 |
| 5 DESFire | CPU + 文件系统 | APDU + AES | 每应用 auth | 文件 AccessCondition | 成本高，开销大 |
| 6 FeliCa | CPU + 专有命令 | 专有高速帧 | auth | 区域规则 | 生态封闭 |
| 7 NFC/SE | AID + 文件 | APDU 路由 | 每应用 auth | SE 内 | 状态碎片化 |
| 8 Token/HCE | 远端 | cryptogram | 每令牌 | 服务端 | 离线、隐私、信任链 |

---

## 4. 把 MIFARE Classic 拼起来

### 4.1 三个面

MIFARE Classic 的完整模型只有三个面：

```
数据面  Block = Byte[16]，Sector = Block[3] + Trailer
控制面  Trailer = KeyA | AccessBits | user byte | KeyB
隐状态  每扇区 auth + 卡片级 transfer buffer
```

### 4.2 数据结构

```pseudo
type Byte  = uint8
type Block = Byte[16]
type Key   = Byte[6]

struct Trailer {
    keyA:   Key
    access: AccessBits
    keyB:   Key
}

struct Sector {
    data:    Block[3]        // block 0..2
    trailer: Trailer         // block 3
    transient auth: Auth     // 易失，断电清零
}

struct Card {
    sectors:  Sector[16]
    transfer: Block?         // 卡片级、易失、只有一个
}
```

### 4.3 地址与块类型

```pseudo
struct Addr { sector: int, block: int }

enum Kind { DATA, VALUE, TRAILER }

function kind(card, a) -> Kind {
    if a.block == 3: return TRAILER
    return card.sectors[a.sector].trailer.access.kindOf(a.block)
}
```

注意：**一个块是不是值块，不是数据里带的标记，而是由访问位决定。** 这是很多人第一次建模时会搞错的地方。

### 4.4 两层裁决

```pseudo
function allowed(card, a, op) -> bool {
    s = card.sectors[a.sector]

    // 结构规则：与访问位无关
    if s.auth == NONE:                   return false
    if a.block == 3 && op == READ_KEY_A: return false

    // 策略规则：由访问位决定
    return s.trailer.access.decide(s.auth, op, a.block)
}
```

`allowed()` 不是「一个开关」，而是「结构规则 ∧ 策略规则」。

### 4.5 命令原语

```pseudo
authenticate(sector, keyType, key)
read(addr) / write(addr, data)
decrement(addr, n) / increment(addr, n)
restore(addr) / transfer(addr)
```

真实的命令码：

| 命令 | 码 |
|---|---|
| 认证 KeyA | `0x60` |
| 认证 KeyB | `0x61` |
| 读 | `0x30` |
| 写 | `0xA0` |
| 减 | `0xC0` |
| 加 | `0xC1` |
| 恢复 | `0xC2` |
| 提交 | `0xB0` |

### 4.6 六条不变式

```
I1  未认证扇区：除 authenticate 外，所有命令 DENIED
I2  authed = true 不等于放行，还要过 access.decide()
I3  KeyA 永远不在可读数据面
I4  写尾块会改变之后所有命令的判定（规则自指）
I5  Decrement / Increment 只改 transfer；Transfer 才改存储
I6  auth 和 transfer 都是易失状态，断电即清空
```

### 4.7 一句话

> **数据面**是按扇区切分的 16 字节块存储；  
> **控制面**是嵌在每个扇区最后一块里的 `KeyA + 访问位 + KeyB`；  
> **隐状态**是每扇区的认证状态和一个值操作暂存器。  
> 所有命令都走同一条判定：`已认证 ∧ 访问位允许`。

---

## 5. 模型批判：四个容易犯的错

一个好的模型不是「能跑就行」，而是「结构上不误导人」。下面四个问题，是这套模型在迭代中修正过的。

### 5.1 `auth` 应该放进 `Sector` 里

`auth` 的天然主键就是 sector。放在外面用 `auth[16]` 平行数组，等于靠「两个数组下标恰好对齐」来维护关系，这是模型自己给自己埋 bug。

```pseudo
struct Sector {
    data:      Block[3]
    trailer:   Trailer
    transient auth: Auth = NONE   // 易失，不进持久化镜像
}
```

用 `transient` 标出「不参与持久化」，两个问题一起解决。

### 5.2 `valueBuf` 不能删

协议层必须有。MIFARE 的值操作是两阶段的：

```
Decrement / Increment ──► 卡片内部 transfer buffer
Transfer             ──► 才真正写回块
```

没有这个缓冲，就解释不了：

- 为什么值操作有 4 条命令，而不是一条 `decrement`；
- 为什么 `Decrement` 之后马上 `Read`，读到的还是旧值；
- 为什么断电会丢掉「已减但未提交」的操作。

但它是**整张卡一个**，不是每扇区一个。所以它应该留在 `Card` 级，而不是放进 `Sector`。

### 5.3 `Trailer` 不是 `Auth`

| | Trailer | Auth |
|---|---|---|
| 性质 | 持久化策略 | 易失状态 |
| 内容 | KeyA / access bits / KeyB | `NONE / A / B` |
| 断电 | 保留 | 清零 |
| 回答的问题 | 「谁能做什么」 | 「现在谁已经进来了」 |

判定顺序是：**读 trailer → 比对 key → 设置 auth**。两者必须同时存在。union 会把「规则」和「是否满足规则」挤进同一个格子。

用门锁打比方：

- `Trailer` = 门上的锁和钥匙孔；
- `Auth` = 这扇门现在是不是已经被打开了。

### 5.4 旧伪代码的结构问题

原始版本有几处容易误导的地方：

1. 持久化与易失混在一起：`sectors[16]` 和 `auth[16]` 平行；
2. 地址用裸 `(s, b)` 两个 int，容易错位；
3. `b == 3` 的特殊判断散落各处；
4. 把「值块」当成数据属性，实际由访问位决定；
5. `KeyA 不可读` 和访问位混在一起；
6. `valueBuf` 作用域不清。

修订后的核心变化：

| 旧 | 新 | 原因 |
|---|---|---|
| `auth[16]` 平行数组 | `Sector.auth` + `transient` | 主键本来就该绑定 |
| `read(s, b)` | `read(card, Addr)` | 两个 int 太容易错位 |
| 到处判断 `b == 3` | `kind()` 统一 | 一个地方回答「这是什么块」 |
| 值块当数据属性 | `access.kindOf()` | 真实卡片由访问位决定 |
| 一张大 `permitted` | `allowed()` 分结构/策略两层 | KeyA 是结构限制，不是策略 |
| `valueBuf` 悬空 | `Card.transfer` | 一个物理缓冲，卡片级 |
| 无命令分发 | `dispatch()` | 状态机显式化 |

---

## 6. Python 实现

完整的可运行代码在 [`mifare_classic.py`](./mifare_classic.py)。这里只讲关键设计。

### 6.1 类型映射

| 伪代码 | Python |
|---|---|
| `Byte[16]` / `Byte[6]` | `NewType("Block", bytes)` / `NewType("Key", bytes)` |
| `struct Addr` | `@dataclass(frozen=True, slots=True)` |
| `enum Auth/Kind/Op` | `enum.Enum` |
| `transient auth` | `field(metadata={"transient": True})` + `reset_session()` |
| `DENIED` | `AccessDenied` |
| `INVALID` | `InvalidOperation` |
| `dispatch(cmd)` | `@dataclass` 命令联合 + `match` |

### 6.2 地址

```python
@dataclass(frozen=True, slots=True)
class Addr:
    sector: int
    block: int

    def __post_init__(self) -> None:
        if not 0 <= self.sector < SECTORS_1K:
            raise ValueError(f"sector out of range: {self.sector}")
        if not 0 <= self.block < BLOCKS_PER_SECTOR:
            raise ValueError(f"block out of range: {self.block}")

    @property
    def flat(self) -> int:
        return self.sector * BLOCKS_PER_SECTOR + self.block
```

### 6.3 两种访问位视图

```python
@dataclass(frozen=True, slots=True)
class AccessBits:
    raw: bytes = b"\xff\x03\xc0"

    @classmethod
    def from_raw(cls, raw: bytes) -> "AccessBits":
        return cls(raw)

    def to_raw(self) -> bytes:
        return self.raw

    @property
    def rules(self) -> tuple[BlockRule, BlockRule, BlockRule, BlockRule]:
        ...
```

`raw` 是唯一真相，`rules` 是解码出来的人类视图。两者可以互相转换，细节见第 7 节。

### 6.4 两层裁决

```python
def allowed(card: Card, a: Addr, op: Op) -> bool:
    sector = card.sectors[a.sector]

    # 结构规则：与访问位无关
    if sector.auth is Auth.NONE:
        return False
    if a.block == 3 and op is Op.READ_KEY_A:
        return False

    # 策略规则：由尾块里的访问位决定
    return sector.trailer.access.decide(sector.auth, op, a.block)
```

### 6.5 值块的两阶段提交

```python
def value_op(card: Card, a: Addr, delta: int) -> None:
    op = Op.DECREMENT if delta < 0 else Op.INCREMENT
    if not allowed(card, a, op):
        raise AccessDenied(...)
    if kind(card, a) is not Kind.VALUE:
        raise InvalidOperation("not a value block")

    current = decode_value(block_at(card, a))
    new_value = current.value + delta
    if new_value < 0:
        raise AccessDenied("insufficient value")

    card.transfer = encode_value(new_value, a)   # 只写暂存器


def transfer(card: Card, a: Addr) -> None:
    if not allowed(card, a, Op.TRANSFER):
        raise AccessDenied(...)
    if card.transfer is None:
        raise InvalidOperation("no staged value")

    card.sectors[a.sector].data[a.block] = card.transfer  # 唯一落盘点
    card.transfer = None
```

### 6.6 运行结果

```text
=== access bits: two views ===
raw = ff 07 80
conditions = [(0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 1)]
...
ok: raw -> rules -> raw round-trip
ok: bad redundancy rejected
ok: unencodable human rule rejected

=== card behavior ===
ok: unauthenticated read denied
ok: wrong key rejected
ok: sector 1 authenticated with key A
ok: sector 2 still locked
ok: data block read/write
ok: value op is staged until transfer
ok: trailer read masks KeyA and returns real access bits
ok: trailer write respects field permissions
ok: KeyA is write-only
ok: session state is volatile
ok: image excludes transient state
```

---

## 7. 访问位的真实编码

### 7.1 24 bit = 12 bit 有效 + 12 bit 冗余

访问位是 3 字节，也就是 24 bit。但它不是 4 个块 × 6 bit 的权限位。

真实结构是：

```
24 bit = 12 bit 有效信息 + 12 bit 反相冗余
12 bit = 4 block × 3 bit (C1 C2 C3)
```

每块的 3 bit 是**条件编号**，去查一张固定的 8 行表。

### 7.2 布局

```
byte 6:  ~C2_3 ~C2_2 ~C2_1 ~C2_0  ~C1_3 ~C1_2 ~C1_1 ~C1_0
byte 7:   C1_3  C1_2  C1_1  C1_0  ~C3_3 ~C3_2 ~C3_1 ~C3_0
byte 8:   C3_3  C3_2  C3_1  C3_0   C2_3  C2_2  C2_1  C2_0
```

每个 nibble 的 bit `i` 对应 block `i`。真副本和反相副本各存一次，读的时候做一致性校验。

### 7.3 工厂默认值 `FF 07 80`

```
C1 = 0b0000
C2 = 0b0000
C3 = 0b1000

block 0: 000
block 1: 000
block 2: 000
block 3: 001
```

正好是「块 0–2 全开，块 3 用 001」的默认配置。

### 7.4 8 行条件表（数据块）

| C1 C2 C3 | read | write | increment | decrement | transfer/restore |
|---|---|---|---|---|---|
| 000 | A/B | A/B | A/B | A/B | A/B |
| 010 | A/B | never | never | never | never |
| 100 | A/B | B | never | never | never |
| 110 | A/B | B | B | never | never |
| 001 | A/B | never | never | A/B | A/B |
| 011 | B | B | never | never | never |
| 101 | B | never | never | never | never |
| 111 | never | never | never | never | never |

注意 `001`：**能减不能加**，这是常见的「单程票」值块。`110` 则是「能加不能减」。

### 7.5 尾块条件表

| C1 C2 C3 | KeyA read | KeyA write | Access bits read | Access bits write | KeyB read | KeyB write |
|---|---|---|---|---|---|---|
| 000 | never | A | A | never | A | A |
| 010 | never | never | A | never | A | never |
| 100 | never | B | A/B | never | A/B | B |
| 110 | never | never | A/B | never | A/B | never |
| 001 | never | A | A | A | A | A |
| 011 | never | B | A/B | B | A/B | B |
| 101 | never | never | A/B | never | A/B | never |
| 111 | never | never | A/B | never | A/B | never |

KeyA 永远不可读。这就是为什么读尾块时 KeyA 返回全 0。

### 7.6 两种视图

| | 机器视图 `raw` | 人类视图 `rules` |
|---|---|---|
| 层次 | 传输 / 存储 | 策略 / 阅读 |
| 形态 | 3 字节 | 4 条 `BlockRule` |
| 有效信息 | C1/C2/C3 各 4 bit | kind + KeyA ops + KeyB ops |
| 冗余 | 12 bit 反相副本 | 不出现 |
| 构造 | `from_raw()` | `from_rules()` |
| 输出 | `to_raw()` | `.rules` / `to_rules()` |
| 完备性 | 总是可以 | 可能 `UnencodableAccess` |

转换：

```python
ab = AccessBits.from_raw(b"\xff\x07\x80")
ab.condition(3)   # (0, 0, 1)
ab.to_raw()       # b"\xff\x07\x80"
ab.rules          # 4 条 BlockRule

custom = AccessBits.from_raw(encode_access(0, 0, 0b1100))
# -> b"\xff\x03\xc0"，块 2 变成 value block

AccessBits.from_rules(custom.rules).to_raw()   # 回到 b"\xff\x03\xc0"
```

`from_rules()` 是**部分函数**：8 行表只能表达有限组合。如果你给一个「A 读写、B 只读」的规则，它会抛 `UnencodableAccess`，而不是找一个差不多的条件悄悄降级。

### 7.7 为什么不是 4×6 的 rwx？

一个很自然的第一性猜测是：

```
24 bit / 4 block = 6 bit/block
6 bit / 2 key   = 3 bit/key → rwx
x = value op
```

这是一个**更通用、更 Unix** 的设计。但真实卡片没有这么做，原因有四：

1. **没有 CPU**。卡片不能解析任意权限位，只能拿 3 bit 去查一张硬连线的小表。查表比逐位组合便宜。
2. **消除非法组合**。rwx 允许「能写不能读」「能增量不能减量」这种奇怪状态；条件表只有 8 行，每行都是设计者认可的组合。
3. **一张表同时表达两把钥匙**。同一行里已经写好了 KeyA / KeyB 各自能做什么，不需要再分 3 bit。
4. **冗余**。非接触卡供电不稳，12 bit 反相副本可以在读的时候做一致性校验。

你猜的「x = value op」有一半对：确实有一行专门表示 value block（`001`），但它不是单独一位，而是整个 3 bit 条件。

---

## 8. 三条设计定律

从这条阶梯里，可以抽出三条反复出现的规律。

### 定律一：权限边界 = 状态寄存器的边界

Classic 的权限粒度是扇区，不是因为扇区优雅，而是因为「每扇区一个认证状态寄存器」最便宜。

Ultralight 只有全局一个 `authState`，所以它只能有全局权限。

**你能保护多细，取决于你能存多少个「当前授权到哪」的位。**

### 定律二：控制面总是先寄生在数据面里

尾块、inode 表、GPT 分区表、页表、ELF 头——全是同一招：把「怎么解释这块数据」塞进「这块数据」的固定偏移里。

好处是省地址译码；代价是控制面可被数据面操作改写。Classic 的尾块可写、Crypto-1 被破，就是这个代价的账单。

### 定律三：抽象层级 ∝ 每事务可承受的计算与通信预算

- 无 CPU → 只能查表（访问位）；
- 有 CPU → 可以解析 ACL（DESFire）；
- 有网络 → 可以把策略放到服务端（Token）。

**`byte[N]` 不是被否决了，而是它需要一个外部的「权威」才能成立**：MMU、OS、HSM、发卡方。没有权威的 `byte[N]`，等于没有门锁的房子。

### 盖尔定律的机械版本

> 复杂系统不是被设计出来的，是沿着「最近一个能工作的系统」逐个坐标轴挪出来的。

所以 MIFARE Classic 不是「设计得不好」，而是「在 1994 年的成本、功耗、协议、生态约束下，能工作的最小系统」。

---

## 9. 结语：卡片史是一部授权下沉史

回头看这条阶梯，会发现它不只是卡片的历史，也是**授权机制的历史**：

```
裸存储          谁都能改
锁位 / OTP      发行方冻结
计数器          消耗不可逆
认证            持有密钥的人
扇区 + 访问位    持有不同密钥的人，做不同的事
CPU + 文件系统   应用 + 文件 + 策略
SE / NFC        手机里的安全端点
Token / HCE     云端的一次性证明
```

每一步都在回答同一个问题：

> **「谁，在什么状态下，被允许对哪块数据做什么？」**

MIFARE Classic 的答案是：

> **每个扇区独立认证；认证之后，由尾块里的访问位逐条裁决。**

它不是最优雅的答案，但它是 1994 年能大规模量产的答案。理解这一点，比记住「Classic 不是 CPU 卡」更有用。

---

## 附录 A：时间线

| 年份 | 事件 |
|---|---|
| 1948 | Stockman 发表 RFID 理论基础 |
| 1967 | Gröttrup 申请电感耦合非接触通信专利 |
| 1988 | 索尼开始研发 FeliCa 前身技术 |
| 1994 | MIFARE Classic 1K；ISO/IEC 14443 标准化启动 |
| 1996 | 首尔地铁大规模商用 |
| 1997 | FeliCa 随八达通商用；MIFARE Pro；ISO/IEC 15693 启动 |
| 1999 | MIFARE Pro X |
| 2001 | MIFARE Ultralight |
| 2002 | MIFARE DESFire；飞利浦 + 索尼开发 NFC |
| 2004 | NFC 论坛成立 |
| 2006 | DESFire EV1（AES-128） |
| 2008 | MIFARE Plus；Ultralight C；Crypto-1 被攻破 |
| 2012 | Ultralight EV1 |
| 2013 | DESFire EV2 |
| 2014 | Apple Pay |
| 2020 | DESFire EV3（CC EAL5+） |
| 2022 | Ultralight AES |

---

## 附录 B：术语表

| 术语 | 含义 |
|---|---|
| APDU | ISO 7816-4 定义的应用层命令格式 |
| AID | 应用标识符，用于在 SE/CPU 卡里路由到某个应用 |
| Crypto-1 | MIFARE Classic 使用的专有流密码，已被攻破 |
| HCE | Host Card Emulation，用手机主处理器模拟卡片 |
| OTP | One-Time Programmable，一次性可编程位 |
| SE | Secure Element，安全元件 |
| Tokenization | 用一次性令牌替代真实卡号 |
| UID | 卡片唯一标识 |
| Value Block | MIFARE Classic 的值块格式，支持减/加/恢复/提交 |

---

## 附录 C：参考资料

- NXP, *MF1S50YYX: MIFARE Classic 1K/4K Datasheet*
- NXP, *MF0ICU1: MIFARE Ultralight Datasheet*
- NXP, *MIFARE DESFire EV1/EV2/EV3 Datasheets*
- ISO/IEC 14443（近接型非接触卡）
- ISO/IEC 15693（近傍型非接触卡）
- ISO/IEC 7816-4（APDU）
- ISO/IEC 18092（NFCIP-1）
- JIS X 6319-4（FeliCa）
- John Gall, *Systemantics: How Systems Work and Especially How They Fail*
- EMVCo, *Payment Tokenisation Specification*

---

> **一句话收尾：**
>
> `byte[N]` 是数据面；密钥是身份；认证状态是「现在」；访问位是「允许」；Transfer 是「何时生效」。
> 把这五样东西分开看，MIFARE Classic 就不再是一堆奇怪的扇区和尾块，而是一个在极端成本约束下长出来的、可解释的状态机。
