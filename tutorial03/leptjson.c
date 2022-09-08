#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif               /* 上面是引入检查CRT内存泄漏的代码 */
#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)
/*
解析字符串（以及之后的数组、对象）时
需要把解析的结果先储存在一个临时的缓冲区
最后再用 lept_set_string() 把缓冲区的结果设进值之中
由于这个缓冲区的大小是不可预知的
要采用动态增长的数据结构
将会发现，无论是解析字符串、数组或对象，只需要以先进后出的方式访问这个动态数组
采用能够动态增长的栈
*/

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;

/*
* 栈的压入 每次可要求压入任意大小的数据，它会返回数据起始的指针
* 注意到这里使用了 realloc() 来重新分配内存
* c->stack 在初始化时为 NULL，realloc(NULL, size) 的行为是等价于 malloc(size) 的
* 所以我们不需要为第一次分配内存作特别处理。

* 另外，我们把初始大小以宏 LEPT_PARSE_STACK_INIT_SIZE 的形式定义
* 使用 #ifndef X #define X ... #endif 方式的好处是
* 使用者可在编译选项中自行设置宏，没设置的话就用缺省值。
*/
static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        // 这里选择的每次再分配空间大小为原来的1.5倍
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}
// 栈的弹出操作
static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}
/*
* 有了以上的工具，解析字符串的任务就变得很简单。
* 我们只需要先备份栈顶，然后把解析到的字符压栈，
* 最后计算出长度并一次性把所有字符弹出，再设置至值里便可以。
* 以下是部分实现，没有处理转义和一些不合法字符的校验。
*/
static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        
        switch (ch) {
            // 遇到第二个"就把栈中的字符串出栈
            // 并分配到v中
            case '\"':
                len = c->top - head;
                lept_set_string(v, (const char*)lept_context_pop(c, len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            // 如果没有遇到第二个"
            // 返回 缺少引号
            case '\0':
                c->top = head;
                return LEPT_PARSE_MISS_QUOTATION_MARK;
            
            case '\\': 
                // 标准的写法
                switch (*p++) {
                    case 'n': PUTC(c, '\n'); break;
                    case 'b': PUTC(c, '\b'); break;
                    case 'f': PUTC(c, '\f'); break;
                    case 'r': PUTC(c, '\r'); break;
                    case 't': PUTC(c, '\t'); break;
                    case '\/': PUTC(c, '\/');  break;
                    case '\\': PUTC(c, '\\'); break;
                    case '\"': PUTC(c, '\"'); break;
                    default:
                        // 都不是的话为非法字符
                        // 需要清空栈 清空并不是真的清空 只不过是从头覆盖
                        c->top = head;
                        return LEPT_PARSE_INVALID_STRING_ESCAPE;
                }
                break;
                /* 原写法
                ch = *p;
                if (ch == 'n') {
                    ch = '\n';
                    p++;
                }
                else if (ch == 'b') {
                    ch = '\b';
                    p++;
                } 
                else if (ch == 'f') {
                    ch = '\f';
                    p++;
                }
                else if (ch == 'r') {
                    ch = '\r';
                    p++;
                }
                else if (ch == 't') {
                    ch = '\t';
                    p++;
                }
                else if (ch == '\"') {
                    ch = '\"';
                    p++;
                }
                else if (ch == '\\') {
                    ch = '\\';
                    p++;
                }
                else if (ch == '\/') {
                    ch = '\/';
                    p++;
                }
                else {
                    return LEPT_PARSE_INVALID_STRING_ESCAPE;
                }*/
            
            /*
            * 默认情况下把当前字符压入到栈中
            * unescaped = %x20 - 21 / %x23 - 5B / %x5D - 10FFFF
            * 空缺的 %x22 是双引号， %x5C 是反斜线，都已经处理
            */
            default:
                /*
                * 注意到 char 带不带符号，是编译器的实现定义的。
                * 如果编译器定义 char 为带符号的话，(unsigned char)ch >= 0x80 的字符，
                * 都会变成负数，并产生 LEPT_PARSE_INVALID_STRING_CHAR 错误。
                * 我们现时还没有测试 ASCII 以外的字符，所以有没有转型至不带符号都不影响
                */
                if ((unsigned char)ch < 0x20) {
                    c->top = head;
                    return LEPT_PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c, ch);
        }
    }
}


static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    // 要初始化这个stack的空间
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    // 断言栈中的所有数据都被弹出
    assert(c.top == 0);
    // 释放这个stack的空间
    free(c.stack);
    return ret;
}

// 清空内存以及类型
void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    /* \TODO */
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    /* \TODO */
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    /* \TODO */
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

// 设置字符串的函数
void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    // 注意在v填充之前先清空内存
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}
