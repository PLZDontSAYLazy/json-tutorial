#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
/* to deal with to large number */
#include <math.h>
#include <errno.h>

// 定义宏函数往往是为了减少代码中的重复部分
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
// 简化代码的宏函数
#define ISDIGIT(ch)         ( (ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ( (ch) >= '1' && (ch) <= '9')
#define ISCHARACTER(ch)     ( ((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'))


typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

// 为了减少代码的重复
// 把lept_parse_true lept_parse_false lept_parse_null
// 整合为一份功能的代码
// ret表示应当的类型
static int lept_parse_literal(lept_context* c, lept_value* v, const char* curr_str, int ret) {
    EXPECT(c, *curr_str);
    curr_str += 1;
    while (*curr_str != '\0') {
        if (*curr_str != *c->json) {
            return LEPT_PARSE_INVALID_VALUE;
        }
        ++curr_str;
        ++c->json;
    }
    v->type = ret;
    return LEPT_PARSE_OK;
}


static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}

static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}


// 校验数字
// 数字的组合为 number = ["-"] int[frac][exp]
static int lept_parse_number(lept_context* c, lept_value* v) {
    /* \TODO validate number */
    const char* p = c->json;

    if (*p == '-') p++;
    // 0开头的直接跳过 比如0 0123 0x123等
    if (*p == '0') p++;
    else {
        if(!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    // 遇到. 跳过
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    } 

    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '-' || *p == '+') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }

    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    
    // 形如 1x123 0x123 1234q12123这种的交给lept_parse处理
    // 因此c->json设置为p
    c->json = p;
    v->type = LEPT_NUMBER;

    return LEPT_PARSE_OK;
}

#if 0
// 以下为校验数字的标准答案
// 使用一个指针p来表示当前的解析字符位置
// 在某些编译器下比起 c->json 性能更好
// 代码更简单
// 数字的组合为 number = ["-"] int[frac][exp]
// 按这个顺序判别即可
static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    /* 负号 */
    if (*p == '-') p++;

    /*  整数
        有两种合法情况
        0开头 这种情况直接跳过
        以及1-9再加上任意数量的digit 
        这种情况需要第一个字符为1-9 否则非法
    */
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }

    /*  小数
        如果出现小数点 就跳过这个小数点
        然后检查它至少应该有一个digit 不是就返回错误码
        跳过这个digit之后 再跳过后面的digit
    */
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); ++p);
    }

    /*  指数
        如果出现e或者E 就跳过这个字符
        然后检查它后面有无+ -
        之后应该有一个digit 不是就返回错误码
        跳过这个digit之后 再跳过后面的digit
    */
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); ++p);
    }
    errno = 0;
    v->n = strtod(c->json, NULL);
    /*  数字过大的处理 */
    if(errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}
#endif

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
