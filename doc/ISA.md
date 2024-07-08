# ISA

### 体系结构

![架构图](./架构图.bmp)



### Setting

+ 指令字长：32bit

+ 通用寄存器个数：32，5bit编址

+ 专用寄存器个数：32

  + 0-15：pim单元专用寄存器
    - 0：input bit width，输入的bit长度
    - 1：output bit width，输出的bit长度
    - 2：weight bit width，权重的bit长度
    - 3：group size，macro group的大小，即包含多少个macro，仅允许设置为config文件里设置的数值之一
    - 4：activation group num，激活的group的数量
    - 5：activation element col num，每个group内激活的element列的数量
    - 6：group input step/offset addr，每一组输入向量的起始地址相对于上一组的增量（step），或相对于rs1的偏移量的地址（offset addr）
    - 7：value sparse mask addr，值稀疏掩码Mask的起始地址
    - 8：bit sparse meta addr，Bit级稀疏Meta数据的起始地址
    - 9-15：留作扩展
  + 16-31：SIMD单元专用寄存器
    - 16：input 1 bit width：输入向量1每个元素的bit长度
    - 17：input 2 bit width：输入向量2每个元素的bit长度
    - 18：input 3 bit width：输入向量3每个元素的bit长度
    - 19：input 4 bit width：输入向量4每个元素的bit长度
    - 20：output bit width：输出向量每个元素的bit长度
    - 21-31：留作其他指令扩展
  + 专用寄存器绑定通用寄存器：对于运算过程中数值频繁变化的专用寄存器，为了减小赋值专用寄存器指令的数量，可在config文件中指定，某个专用寄存器绑定到某个通用寄存器上，即该通用寄存器既可以用作通用运算，又可在部分运算时作为专用寄存器，类似MIPS32架构里的sp、ra等寄存器。绑定格式如下：

  ```json
  {
      "special register binding": [
          {
              "special": 7,
              "general": 30    // 即为7号专用寄存器绑定到30号通用寄存器
          },
          {
              // xxx
          }
      ]
  }
  ```

+ 指令类别码：class，[31, 30]或[31, 29]，2或3bit长

  + 00：Pim指令

  + 01：SIMD指令

  + 10：寄存器运算指令

  + 110：数据传输指令

  + 111：控制指令

+ 所有core内存储器（SRAM，寄存器等）使用统一编址，格式如下：

```json
{
    "local memory list": [
        {
            "name": "local mem",
            "type": "sram",
            "addressing": {
                "offset": 0,        // 统一编址后，该存储器的地址相对于地址0的偏移量，以byte为单位
                "size": 1024        // 该存储器的大小，以byte为单位
            }
        },
        {
            //xxx
        }
    ]
}
```





### Pim指令

指令类型码：type，[29]，1bit长

+ 0：pim运算
+ 1：pim批处理

#### pim计算：pim-compute

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为00
+ [29, 29]，1bit：type，指令类型码，值为0
+ [28, 25]，4bit：reserve，保留字段
+ [24, 20]，5bit：flag，功能扩展字段
  + [24]，1bit：value sparse，表示是否使用值稀疏，稀疏掩码Mask的起始地址由专用寄存器给出
  + [23]，1bit：bit sparse，表示是否使用bit级稀疏，稀疏Meta数据的起始地址由专用寄存器给出
  + [22]，1bit：group，表示是否进行分组，组大小及激活的组数量由专用寄存器给出
  + [21]，1bit：group input mode，表示多组输入的模式
    + 0：每一组输入向量的起始地址相对于上一组的增量（步长，step）是一个定值，由专用寄存器给出
    + 1：每一组输入向量的起始地址相对于上一组的增量不是定值，其相对于rs1的偏移量（offset）在存储器中给出，地址（offset addr）由专用寄存器给出

  + [20]，1bit：accumulate，表示是否进行累加

+ [19, 15]，5bit：rs1，通用寄存器1，表示input向量起始地址
+ [14, 10]，5bit：rs2，通用寄存器2，表示input向量长度
+ [9, 5]，5bit：rs3，通用寄存器3，表示激活的row的index
+ [4, 0]，5bit：rd，通用寄存器4，表示output写入的起始地址

使用的专用寄存器：

+ input bit width：输入的bit长度
+ output bit width：输出的bit长度
+ weight bit width：权重的bit长度
+ group size：macro group的大小，即包含多少个macro，仅允许设置为config文件里设置的数值之一
+ activation group num：激活的group的数量
+ activation element col num：每个group内激活的element列的数量
+ group input step/offset addr：每一组输入向量的起始地址相对于上一组的增量（step），或相对于rs1的偏移量的地址（offset addr）
+ value sparse mask addr：值稀疏掩码Mask的起始地址
+ bit sparse meta addr：Bit级稀疏Meta数据的起始地址

#### pim批处理：pim-batch

pim-batch指令进行批处理的预处理，即设置批处理的部分参数，后需要紧跟一条pim-compute开启批处理运算

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为00
+ [29, 29]，1bit：type，指令类型码，值为1
+ [28, 23]，6bit：reserve，保留字段
+ [22, 20]，3bit：flag，功能字段
  + [22]，1bit：表示不同批次间值稀疏掩码Mask的起始地址的变化模式
  + [21]，1bit：表示不同批次间Bit级稀疏Meta数据的起始地址的变化模式
  + [20]，1bit：表示不同批次间input向量起始地址的变化模式
    + 0：该批次的起始地址相对于上一批次的增量（步长，step）是一个定值，由rs2/3/4给出
    + 1：该批次的起始地址相对于上一批次的增量不是定值，其相对于第一条指令对应的起始地址的偏移量（offset）在存储器中给出，地址（offset addr）由rs2/3/4给出
+ [19, 15]，5bit：rs1，通用寄存器1，表示一次批处理运算的长度
+ [14, 10]，5bit：rs2，通用寄存器2，表示输入向量起始地址的增量或偏移量地址
+ [9, 5]，5bit：rs3，通用寄存器3，表示Bit级稀疏Meta数据的起始地址的增量或偏移量地址
+ [4, 0]，5bit：rs4，通用寄存器4，表示值稀疏掩码Mask的起始地址的增量或偏移量地址





### SIMD计算指令

#### SIMD计算：SIMD-compute

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为01
+ [29, 28]，2bit：input num，input向量的个数，范围是1到4
  + 00：1个输入向量，地址由rs1给出
  + 01：2个输入向量，地址由rs1和rs2给出
  + 10：3个输入向量，地址由rs1，rs1+1，rs2给出
  + 11：4个输入向量，地址由rs1，rs1+1，rs2，rs2+1给出
+ [27, 20]，8bit：opcode，操作类别码，表示具体计算的类型
+ [19, 15]，5bit：rs1，通用寄存器1，表示input向量起始地址1
+ [14, 10]，5bit：rs2，通用寄存器2，表示input向量起始地址2
+ [9, 5]，5bit：rs3，通用寄存器3，表示input向量长度
+ [4, 0]，5bit：rd，通用寄存器4，表示output写入的起始地址

使用的专用寄存器：

+ input 1 bit width：输入向量1每个元素的bit长度
+ input 2 bit width：输入向量2每个元素的bit长度
+ input 3 bit width：输入向量3每个元素的bit长度
+ input 4 bit width：输入向量4每个元素的bit长度
+ output bit width：输出向量每个元素的bit长度

备注：SIMD指令也可执行向量与标量的运算，每条指令的每个输入数的标量or向量是固定的，并需要再config文件中指定



### 标量计算指令

指令类型码：type，[29, 28]，2bit长

+ 00：R-R型整数运算指令
+ 01：R-I型整数运算指令
+ 10：Load/Store指令
+ 11：立即数赋值指令

#### R-R型整数运算指令：scalar-RR

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为10
+ [29, 28]，2bit：type，指令类型码，值为00
+ [27, 26]，2bit：reserve，保留字段
+ [25, 21]，5bit：rs1，通用寄存器1，表示运算数1的值
+ [20, 16]，5bit：rs2，通用寄存器2，表示运算数2的值
+ [15, 11]，5bit：rd，通用寄存器3，即运算结果写回的寄存器
+ [10, 3]，8bit：reserve，保留字段
+ [2, 0]，3bit：opcode，操作类别码，表示具体计算的类型
  + 000：add，整型加法
  + 001：sub，整型减法
  + 010：mul，整型乘法，结果寄存器仅保留低32位
  + 011：div，整型除法，结果寄存器仅保留商
  + 100：sll，逻辑左移
  + 101：srl，逻辑右移
  + 110：sra，算数右移

#### R-I型整数运算指令：scalar-RI

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为10
+ [29, 28]，2bit：type，指令类型码，值为01
+ [27, 26]，2bit：opcode，操作类别码，表示具体计算的类型
  + 00：addi，整型立即数加法
  + 01：muli，整型立即数乘法，结果寄存器仅保留低32位
  + 10：lui，高16位立即数赋值
+ [25, 21]，5bit：rs，通用寄存器1，表示运算数1的值
+ [20, 16]，5bit：rd，通用寄存器2，即运算结果写回的寄存器
+ [15, 0]，16bit：imm，立即数，表示运算数2的值

#### Load/Store指令：scalar-SL

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为10
+ [29, 28]，2bit：type，指令类型码，值为10
+ [27, 26]，2bit：opcode，操作类别码，表示具体操作的类型
  + 00：本地存储load至寄存器
  + 01：寄存器值store至本地存储
  + 10：全局存储load至寄存器
  + 11：寄存器值store至全局存储
+ [25, 21]，5bit：rs1，通用寄存器1，即寻址的基址寄存器base
+ [20, 16]，5bit：rs2，通用寄存器2，即存储load/store值的寄存器
+ [15, 0]，16bit：offset，立即数，表示寻址的偏移值
  + 地址计算公式：$rs + offset

#### 其他指令

##### 通用寄存器立即数赋值指令：general-li

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为10
+ [29, 28]，2bit：type，指令类型码，值为11
+ [27, 26]，2bit：opcode，指令操作码，值为00
+ [25, 21]，5bit：rd，通用寄存器编号，即要赋值的通用寄存器
+ [20, 0]，21bit：imm，立即数，表示将要赋给寄存器的值

##### 专用寄存器立即数赋值指令：special-li

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为10
+ [29, 28]，2bit：type，指令类型码，值为11
+ [27, 26]，2bit：opcode，指令操作码，值为01
+ [25, 21]，5bit：rd，专用寄存器编号，即要赋值的通用寄存器
+ [20, 0]，21bit：imm，立即数，表示将要赋给寄存器的值

##### 专用/通用寄存器赋值指令：special-general-assign

指令字段划分：

+ [31, 30]，2bit：class，指令类别码，值为10
+ [29, 28]，2bit：type，指令类型码，值为11
+ [27, 26]，2bit：opcode，指令操作码
  + 10：表示将通用寄存器的值赋给专用寄存器
  + 11：表示将专用寄存器的值赋给通用寄存器
+ [25, 21]，5bit：rs1，通用寄存器编号，即涉及赋值的通用寄存器
+ [20, 16]，5bit：rs2，专用寄存器编号，即涉及赋值的专用寄存器
+ [15, 0]，16bit：reserve，保留字段



### 数据传输指令

指令类型码：type，[28]或[28, 27]，1或2bit长

+ 0：**核内**数据传输指令
+ 10：**核间**数据发送指令
+ 11：**核间**数据接收指令

#### 核内数据传输指令：trans

指令字段划分：

+ [31, 29]，3bit：class，指令类别码，值为110
+ [28, 28]，1bit：type，指令类型码，值为0
+ [27, 26]，1bit：offset mask，偏移值掩码，0表示该地址不使用偏移值，1表示使用偏移值
  + [27]，1bit：source offset mask，源地址偏移值掩码
  + [26]，1bit：destination offset mask，目的地址偏移值掩码
+ [25, 21]，5bit：rs，通用寄存器1，表示传输源地址的基址
+ [20, 16]，5bit：rd，通用寄存器2，表示传输目的地址的基址
+ [15, 0]，16bit：offset，立即数，表示寻址的偏移值
  + 源地址计算公式：$rs + offset * [27]
  + 目的地址计算公式：$rd + offset * [26]

#### 核间数据发送指令：send

指令字段划分：

+ [31, 29]，3bit：class，指令类别码，值为110
+ [28, 27]，2bit：type，指令类型码，值为10
+ [26, 26]，1bit：sync，是否进行同步通信
  + 0：表示本次通信为同步通信，即会阻塞直至对方接收到数据
  + 1：表示本次通信为异步通信，即不会阻塞，异步通信可通过wait指令进行同步
+ [25, 21]，5bit：rs，通用寄存器1，表示传输源地址
+ [20, 16]，5bit：rd1，通用寄存器2，表示传输的目的core的编号
+ [15, 11]，5bit：rd2，通用寄存器3，表示传输的目的core的目的地址
+ [10, 6]，5bit：reg-id，通信id码，唯一标识本次传输，用于通信双方进行确认
+ [5, 0]，6bit：reserve，保留字段

#### 核间数据接收指令：receive

指令字段划分：

+ [31, 29]，3bit：class，指令类别码，值为110
+ [28, 27]，2bit：type，指令类型码，值为11
+ [26, 26]，1bit：sync，是否进行同步通信
  + 0：表示本次通信为同步通信，即会阻塞直至接收到对方发送的数据
  + 1：表示本次通信为异步通信，即不会阻塞，异步通信可通过wait指令进行同步
+ [25, 21]，5bit：rs1，通用寄存器1，表示传输的源core的编号
+ [20, 16]，5bit：rs2，通用寄存器2，表示传输的源core的源地址
+ [15, 11]，5bit：rd，通用寄存器3，表示传输的目的地址
+ [10, 6]，5bit：reg-id，通信id码，唯一标识本次传输，用于通信双方进行确认
+ [5, 0]，6bit：reserve，保留字段



### 控制指令

指令类型码：type，[28, 26]，3bit长

+ 000：相等跳转指令
+ 001：不等跳转指令
+ 010：大于跳转指令
+ 011：小于跳转指令
+ 100：无条件跳转指令
+ 101：异步通信的同步指令
+ 110：屏障指令

#### 有条件跳转指令：branch

指令字段划分：

+ [31, 29]，3bit：class，指令类别码，值为111
+ [28, 26]，3bit：type，指令类型码
  + 000：beq，相等跳转
  + 001：bne，不等跳转
  + 010：bgt，大于跳转
  + 011：blt，小于跳转
+ [25, 21]，5bit：rs1，通用寄存器1，表示进行比较的操作数1
+ [20, 16]，5bit：rs2，通用寄存器2，表示进行比较的操作数2
+ [15, 0]，16bit：offset，立即数，表示跳转指令地址相对于该指令的偏移值

#### 无条件跳转指令：jmp

指令字段划分：

+ [31, 29]，3bit：class，指令类别码，值为111
+ [28, 26]，3bit：type，指令类型码，值为100
+ [25, 0]，26bit：offset，立即数，表示跳转指令地址相对于该指令的偏移值

#### 异步通信的同步指令：wait

指令字段划分：

+ [31, 29]，3bit：class，指令类别码，值为111
+ [28, 26]，3bit：type，指令类型码，值为101
+ [25, 21]，5bit：rs-core，通用寄存器1，表示通信对方的code编号
+ [20, 16]，5bit：rs-id，通用寄存器2，表示本次通信的id
+ [15, 0]，16bit：reserve，保留字段

#### 屏障指令：barrier

指令字段划分：

+ [31, 29]，3bit：class，指令类别码，值为111
+ [28, 26]，3bit：type，指令类型码，值为110
+ [25, 21]，5bit：rs-id，通用寄存器1，表示屏障id
+ [20, 16]，5bit：rs-num，通用寄存器2，表示被该屏障阻塞的code数量
+ [15, 0]，16bit：reserve，保留字段