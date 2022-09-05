#ifndef LEPTJSON_H__
#define LEPTJSON_H__

// ����json����������
// ���а�true��false��������
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
// ������ĿǰjsonӦ������ĸ�ʽҪ��
// ���涨���Ӧ�ķ���ֵ
// LEPT_PARSE_EXPECT_VALUE��ʾjson��ֻ�пհ�
// LEPT_PARSE_INVALID_VALUE��ʾĿǰ��json�г����˳�"null" "true" "false" �����ֵ
// LEPT_PARSE_ROOT_NOT_SINGULAR��ʾ�հ�֮���������ַ�
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR
};

// �˺����ķ���ֵ�������Ԫ�����ж���
int lept_parse(lept_value* v, const char* json);
// �˺����ķ���ֵΪlept_type
lept_type lept_get_type(const lept_value* v);

#endif /* LEPTJSON_H__ */
