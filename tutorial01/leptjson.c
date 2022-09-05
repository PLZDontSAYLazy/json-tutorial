#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */

// ����ʽ��� ����ǲ���ch��ͷ
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

// ���ô˽ṹ����Ϊ�˼��ٽ�������֮�䴫�ݶ������
typedef struct {
    const char* json;
}lept_context;

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
// ��鿪ͷ�ǲ���ws
// �ո� ָ�� ���� �س�
// �����˾�ת���ַ�������һ��λ��
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}
/* null  = "null" */
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

/* true = "true" */
static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

/* true = "false" */
static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

// ����ǰһλ�ǲ���ws�ļ���֮��
// ����value�ļ���
/* value = null / false / true */
/* ��ʾ���������û���� false / true����������ϰ֮һ */
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_null(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        case 't':  return lept_parse_true(c, v);
        case 'f':  return lept_parse_false(c, v);    
        default:   return LEPT_PARSE_INVALID_VALUE;
    }
}
// ��д�ĵݹ��½�������
/* ��ʾ������Ӧ���� JSON-text = ws value ws */
/* ����ʵ��û�������� ws �� LEPT_PARSE_ROOT_NOT_SINGULAR */
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    // ���value��Ч
    if (lept_parse_value(&c, v) == LEPT_PARSE_OK) {
        ++c.json;
        // ����ո�
        lept_parse_whitespace(&c);
        // ���������ո��������
        if (c.json[0] != '\0') {
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
        return LEPT_PARSE_OK;
    }
    return lept_parse_value(&c, v);
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}
