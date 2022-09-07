#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
/* to deal with to large number */
#include <math.h>
#include <errno.h>

// ����꺯��������Ϊ�˼��ٴ����е��ظ�����
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
// �򻯴���ĺ꺯��
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

// Ϊ�˼��ٴ�����ظ�
// ��lept_parse_true lept_parse_false lept_parse_null
// ����Ϊһ�ݹ��ܵĴ���
// ret��ʾӦ��������
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


// У������
// ���ֵ����Ϊ number = ["-"] int[frac][exp]
static int lept_parse_number(lept_context* c, lept_value* v) {
    /* \TODO validate number */
    const char* p = c->json;

    if (*p == '-') p++;
    // 0��ͷ��ֱ������ ����0 0123 0x123��
    if (*p == '0') p++;
    else {
        if(!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    // ����. ����
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
    
    // ���� 1x123 0x123 1234q12123���ֵĽ���lept_parse����
    // ���c->json����Ϊp
    c->json = p;
    v->type = LEPT_NUMBER;

    return LEPT_PARSE_OK;
}

#if 0
// ����ΪУ�����ֵı�׼��
// ʹ��һ��ָ��p����ʾ��ǰ�Ľ����ַ�λ��
// ��ĳЩ�������±��� c->json ���ܸ���
// �������
// ���ֵ����Ϊ number = ["-"] int[frac][exp]
// �����˳���б𼴿�
static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    /* ���� */
    if (*p == '-') p++;

    /*  ����
        �����ֺϷ����
        0��ͷ �������ֱ������
        �Լ�1-9�ټ�������������digit 
        ���������Ҫ��һ���ַ�Ϊ1-9 ����Ƿ�
    */
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }

    /*  С��
        �������С���� ���������С����
        Ȼ����������Ӧ����һ��digit ���Ǿͷ��ش�����
        �������digit֮�� �����������digit
    */
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); ++p);
    }

    /*  ָ��
        �������e����E ����������ַ�
        Ȼ��������������+ -
        ֮��Ӧ����һ��digit ���Ǿͷ��ش�����
        �������digit֮�� �����������digit
    */
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); ++p);
    }
    errno = 0;
    v->n = strtod(c->json, NULL);
    /*  ���ֹ���Ĵ��� */
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
