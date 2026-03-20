# API参考

## 函数说明

ksonic-cpp库已优化函数如[**表 1** ksonic-cpp库已优化函数列表](#ksonic-cpp库已优化函数列表)所示。

**表 1** ksonic-cpp库已优化函数列表<a id="ksonic-cpp库已优化函数列表"></a>

|名称|说明|
|--|--|
|Parse|将JSON字符串解析为DOM文档对象，支持完整的JSON语法解析。|
|Serialize|将DOM文档对象序列化为JSON字符串。|

## 函数定义

### Parse

**函数功能**

将JSON字符串解析为DOM文档对象，支持完整的JSON语法解析，包括对象、数组、字符串、数字、布尔值和null。

**函数定义**

```cpp
GenericDocument& Parse(StringView json)
```

**参数说明**

|参数名|描述|取值范围|输入/输出|
|--|--|--|--|
|json|指向JSON字符串的指针或字符串引用。|非空指针，指向有效的JSON格式字符串|输入|

**返回值**

- 返回Document对象自身的引用。

>![](public_sys-resources/icon-note.gif) **说明：** 
>解析过程中会进行语法检查，如果JSON格式不正确，可通过HasParseError()检查是否解析失败。如果解析失败，doc的类型为null。

**示例**

```cpp
#include <iostream>
#include <string>
#include "sonic/sonic.h"

int main() {
    std::string json = R"({"name": "test", "value": 123})";
    sonic_json::Document doc;
    doc.Parse(json);
    
    if (doc.HasParseError()) {
        std::cout << "Parse failed!" << std::endl;
        return -1;
    }
    std::cout << "Parse success!" << std::endl;
    return 0;
}
```

运行结果：

```text
Parse success!
```

### Serialize

**函数功能**

将DOM文档对象序列化为JSON字符串。

**函数定义**

```cpp
SonicError Serialize(WriteBuffer& wb) const;
```

**参数说明**

|参数名|描述|取值范围|输入/输出|
|--|--|--|--|
|wb|写入缓冲区对象。|有效的WriteBuffer对象|输出|

**返回值**

- 成功：返回 kErrorNone
- 失败：返回如下错误码
  - `kSerErrorUnsupportedType`：Serialize: DOM has invalid node type.
  - `kSerErrorInfinity`：Serialize: DOM has infinity number node.
  - `kSerErrorInvalidObjKey`：Serialize: The type of object's key is not string.

序列化结果存储在WriteBuffer中，可通过wb.ToString()获取字符串。

**示例**

```cpp
#include <iostream>
#include <string>
#include "sonic/sonic.h"

int main() {
    std::string json = R"({"name": "test", "value": 123})";
    sonic_json::Document doc;
    doc.Parse(json);
    
    if (doc.HasParseError()) {
        std::cout << "Parse failed!" << std::endl;
        return -1;
    }
    
    sonic_json::WriteBuffer wb;
    doc.Serialize(wb);
    std::cout << "Serialized: " << wb.ToString() << std::endl;
    return 0;
}
```

运行结果：

```text
Serialized: {"name":"test","value":123}
```
