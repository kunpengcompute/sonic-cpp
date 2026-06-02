# API Reference

## Functions

[**Table 1**](#optimized-functions-in-the-sonic-cpp-patch-repository) lists the optimized functions in the sonic-cpp patch repository.

**Table 1** Optimized functions in the sonic-cpp patch repository<a id="optimized-functions-in-the-sonic-cpp-patch-repository"></a>

|Name|Description|
|--|--|
|Parse|Parses a JSON string into a DOM object, supporting full JSON syntax parsing.|
|Serialize|Serializes a DOM object into a JSON string.|

## Function Description

### Parse

**Function Usage**

Parses a JSON string into a DOM object, supporting full JSON syntax parsing, including objects, arrays, strings, numbers, boolean values, and NULL.

**Function Syntax**

```cpp
GenericDocument& Parse(StringView json)
```

**Parameters**

|Parameter|Description|Value Range|Input/Output|
|--|--|--|--|
|json|Pointer or string reference to a JSON string|Non-null pointer to a valid JSON string|Input|

**Return Value**

This function returns a reference to the Document object itself.

>![](public_sys-resources/icon-note.gif) **NOTE:**
>Syntax check is performed during parsing. If the JSON format is incorrect, you can use **HasParseError()** to check whether the parsing fails. If the parsing fails, the type of **doc** is **null**.

**Example**

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

Output:

```text
Parse success!
```

### Serialize

**Function Usage**

Serializes a DOM object into a JSON string.

**Function Syntax**

```cpp
SonicError Serialize(WriteBuffer& wb) const;
```

**Parameters**

|Parameter|Description|Value Range|Input/Output|
|--|--|--|--|
|wb|Object written to the buffer|Valid WriteBuffer object|Output|

**Return Value**

- Success: **kErrorNone**
- Failure: The following error code is returned.
  - `kSerErrorUnsupportedType`: Serialize: DOM has invalid node type.
  - `kSerErrorInfinity`: Serialize: DOM has infinity number node.
  - `kSerErrorInvalidObjKey`: Serialize: The type of object's key is not string.

The serialization result is stored in WriteBuffer. You can obtain the string by calling **wb.ToString()**.

**Example**

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

Output:

```text
Serialized: {"name":"test","value":123}
```
