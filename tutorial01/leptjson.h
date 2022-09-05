#ifndef LEPTJSON_H__
#define LEPTJSON_H__

// 定义json的六种类型
// 其中把true和false算作两种
typedef enum { LEPT_NULL, 
            LEPT_FALSE, 
            LEPT_TRUE, 
            LEPT_NUMBER, 
            LEPT_STRING, 
            LEPT_ARRAY,
            LEPT_OBJECT } lept_type;

typedef struct {
    lept_type type;
}lept_value;

// JSON-text = ws value ws
// ws = *(% x20 / % x09 / % x0A / % x0D)
// value = null / false / true
// null = "null"
// false = "false"
// true = "true"
// 上面是目前json应当满足的格式要求
// 下面定义对应的返回值
// LEPT_PARSE_EXPECT_VALUE表示json中只有空白
// LEPT_PARSE_INVALID_VALUE表示目前的json中出现了除"null" "true" "false" 以外的值
// LEPT_PARSE_ROOT_NOT_SINGULAR表示空白之后还有其他字符
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR
};

// 此函数的返回值在上面的元组中有定义
int lept_parse(lept_value* v, const char* json);
// 此函数的返回值为lept_type
lept_type lept_get_type(const lept_value* v);

#endif /* LEPTJSON_H__ */
