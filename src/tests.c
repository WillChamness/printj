#include <string.h>
#include <assert.h>
#include "parser.h"
#include "json.h"

#define PRINT_TEST_HEADER(msg) printf("===========TEST CASE: %s===========\n", (msg))
#define PRINT_INPUT(str) printf("INPUT IS:\n%s\n", (str))
#define PRINT_TEST_PASSED printf("ALL ASSERTIONS PASSED\n")
#define PRINT_TEST_FAILED_AND_EXIT(reason) do {\
    printf("TEST FALIED:\n");\
    printf("error: %s\n", (reason));\
    exit(1);\
} while(0)

#define ASSERT_TYPE_EQ(json_value, json_type) assert((json_value).type == (json_type))


void test_string() {
    PRINT_TEST_HEADER("string");
    char json_str[] = "\"hello world\"";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));
    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_STR);
    PRINT_TEST_PASSED;
    printf("string is: %s\n", unwrap_json_or_halt(either).data.string);
    deinit_json(either);
}

void test_numbers() {
    PRINT_TEST_HEADER("numbers");
    char *json_str = "123";
    PRINT_INPUT(json_str);
    EitherErrJson *n1 = json_loads(json_str);
    if(is_error(n1))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(n1));
    json_str = "-321";
    PRINT_INPUT(json_str);
    EitherErrJson *n2 = json_loads(json_str);
    if(is_error(n2))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(n2));
    json_str = "98.0";
    PRINT_INPUT(json_str);
    EitherErrJson *n3 = json_loads(json_str);
    if(is_error(n3))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(n3));
    json_str = "-543.21";
    PRINT_INPUT(json_str);
    EitherErrJson *n4 = json_loads(json_str);
    if(is_error(n4))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(n4));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(n1), JSON_INT);
    ASSERT_TYPE_EQ(unwrap_json_or_halt(n2), JSON_INT);
    ASSERT_TYPE_EQ(unwrap_json_or_halt(n3), JSON_DOUBLE);
    ASSERT_TYPE_EQ(unwrap_json_or_halt(n4), JSON_DOUBLE);

    int n1_value = unwrap_json_or_halt(n1).data.integer;
    int n2_value = unwrap_json_or_halt(n2).data.integer;
    double n3_value = unwrap_json_or_halt(n3).data.real;
    double n4_value = unwrap_json_or_halt(n4).data.real;
    const double tolerance = 0.00001;

    assert(n1_value == 123);
    assert(n2_value == -321);
    assert(-tolerance <= n3_value - 98.0 && n3_value - 98.0 <= tolerance); 
    assert(-tolerance <= n4_value - (-543.21) && n4_value - (-543.21) <= tolerance); 

    PRINT_TEST_PASSED;
    printf("the numbers are: %d, %d, %lf, %lf\n",
        unwrap_json_or_halt(n1).data.integer,
        unwrap_json_or_halt(n2).data.integer,
        unwrap_json_or_halt(n3).data.real,
        unwrap_json_or_halt(n4).data.real);

    deinit_json(n1);
    deinit_json(n2);
    deinit_json(n3);
    deinit_json(n4);
}

void test_booleans() {
    PRINT_TEST_HEADER("booleans");
    char input1[] = "true";
    char input2[] = "false";
    PRINT_INPUT(input1);
    EitherErrJson *b1 = json_loads(input1);
    if(is_error(b1))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(b1));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(b1), JSON_BOOL);

    PRINT_INPUT(input2);
    EitherErrJson *b2 = json_loads(input2);
    if(is_error(b2))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(b2));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(b2), JSON_BOOL);
    PRINT_TEST_PASSED;

    printf("the booleans are: %s, %s\n",
        unwrap_json_or_halt(b1).data.boolean ? "true" : "false",
        unwrap_json_or_halt(b2).data.boolean ? "true" : "false");

    deinit_json(b1);
    deinit_json(b2);
}

void test_null() {
    PRINT_TEST_HEADER("null");
    char json_str[] = "null";
    PRINT_INPUT(json_str);

    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_NULL);
    PRINT_TEST_PASSED;
}

void test_unnested_array() {
    PRINT_TEST_HEADER("unnested array");
    char json_str[] = "[\"hello\", \"world\", true, false, 123, -321, 98.76, -543.21, null]";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));
    JsonValue value = unwrap_json_or_halt(either);
    assert(value.type == JSON_ARRAY);
    JsonArray *array = value.data.array;
    assert(array->values[0].type == JSON_STR && array->values[1].type == JSON_STR);
    assert(array->values[2].type == JSON_BOOL && array->values[3].type == JSON_BOOL);
    assert(array->values[4].type == JSON_INT && array->values[5].type == JSON_INT);
    assert(array->values[6].type == JSON_DOUBLE && array->values[7].type == JSON_DOUBLE);
    assert(array->values[8].type == JSON_NULL);
    PRINT_TEST_PASSED;
    deinit_json(either);
}

void test_unnested_object() {
    PRINT_TEST_HEADER("unnested object");
    char json_str[] = "{\"key1\": \"value1\", \"key2\": true, \"key3\": false, "
    "\"key4\": 123, \"key5\": -321, \"key6\": 98.76, \"key7\": -543.21, \"key8\": null}";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_OBJECT);
    JsonObject *obj = unwrap_json_or_halt(either).data.object;

    ASSERT_TYPE_EQ(obj->values[0], JSON_STR);
    ASSERT_TYPE_EQ(obj->values[1], JSON_BOOL);
    ASSERT_TYPE_EQ(obj->values[2], JSON_BOOL);
    ASSERT_TYPE_EQ(obj->values[3], JSON_INT);
    ASSERT_TYPE_EQ(obj->values[4], JSON_INT);
    ASSERT_TYPE_EQ(obj->values[5], JSON_DOUBLE);
    ASSERT_TYPE_EQ(obj->values[6], JSON_DOUBLE);
    ASSERT_TYPE_EQ(obj->values[7], JSON_NULL);
    PRINT_TEST_PASSED;

    deinit_json(either);
}

void test_nested_array() {
    PRINT_TEST_HEADER("nested array");
    char json_str[] = "[1, [1,2.1], 2.0, {\"key1\":1, \"key2\":[1, 2]}, 3]";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);

    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_ARRAY);
    JsonArray *arr = unwrap_json_or_halt(either).data.array;

    ASSERT_TYPE_EQ(arr->values[0], JSON_INT);
    ASSERT_TYPE_EQ(arr->values[1], JSON_ARRAY);
    ASSERT_TYPE_EQ(arr->values[2], JSON_DOUBLE);
    ASSERT_TYPE_EQ(arr->values[3], JSON_OBJECT);
    ASSERT_TYPE_EQ(arr->values[4], JSON_INT);

    JsonArray *nested_arr = arr->values[1].data.array;
    JsonObject *nested_obj = arr->values[3].data.object;

    ASSERT_TYPE_EQ(nested_arr->values[0], JSON_INT);
    ASSERT_TYPE_EQ(nested_arr->values[1], JSON_DOUBLE);

    ASSERT_TYPE_EQ(nested_obj->values[0], JSON_INT);
    ASSERT_TYPE_EQ(nested_obj->values[1], JSON_ARRAY);

    PRINT_TEST_PASSED;
    deinit_json(either);
}

void test_nested_object() {
    PRINT_TEST_HEADER("nested object");
    char json_str[] = "{\"key1\": 1, \"key2\": [1,2.1], \"key3\": 2.0, \"key4\": {\"subkey1\": 1,\"subkey2\": [1,2]}, \"key5\": 3}";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_OBJECT);

    JsonObject *obj = unwrap_json_or_halt(either).data.object;
    
    ASSERT_TYPE_EQ(obj->values[0], JSON_INT);
    ASSERT_TYPE_EQ(obj->values[1], JSON_ARRAY);
    ASSERT_TYPE_EQ(obj->values[2], JSON_DOUBLE);
    ASSERT_TYPE_EQ(obj->values[3], JSON_OBJECT);
    ASSERT_TYPE_EQ(obj->values[4], JSON_INT);

    JsonArray *nested_arr = obj->values[1].data.array;
    JsonObject *nested_obj = obj->values[3].data.object;

    ASSERT_TYPE_EQ(nested_arr->values[0], JSON_INT);
    ASSERT_TYPE_EQ(nested_arr->values[1], JSON_DOUBLE);
    
    ASSERT_TYPE_EQ(nested_obj->values[0], JSON_INT);
    ASSERT_TYPE_EQ(nested_obj->values[1], JSON_ARRAY);

    PRINT_TEST_PASSED;
    deinit_json(either);
}

void test_empty_string() {
    PRINT_TEST_HEADER("empty JSON string");
    char json_str[] = "\"\"";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_STR);
    assert(strlen(unwrap_json_or_halt(either).data.string) == 0);

    PRINT_TEST_PASSED;
    deinit_json(either);
}

void test_empty_array() {
    PRINT_TEST_HEADER("empty array");
    char json_str[] = "[]";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_ARRAY);
    assert(unwrap_json_or_halt(either).data.array->length == 0);

    PRINT_TEST_PASSED;
    deinit_json(either);
}

void test_empty_object() {
    PRINT_TEST_HEADER("empty object");
    char json_str[] = "{}";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_OBJECT);
    assert(unwrap_json_or_halt(either).data.object->length == 0);

    PRINT_TEST_PASSED;
    deinit_json(either);
}

void test_untrimmed_input_string() {
    PRINT_TEST_HEADER("untrimmed input string");
    char json_str[] = " \t\r\n[1, 2.0, true, false, {}, [], null, \"hello world\"] \t\r\n";
    PRINT_INPUT("(C string literal) \" \\t\\r\\n[1, 2.0, true, false, {}, [], null, \"hello world\"] \\t\\r\\n\"");
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_ARRAY);
    PRINT_TEST_PASSED;
    deinit_json(either);
}

void test_string_escaped_characters() {
    PRINT_TEST_HEADER("string with escaped characters");
    char json_str[] = "\"hello world. this is a backslash \\\\ this is a quote \\\" wow \"";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    if(is_error(either))
        PRINT_TEST_FAILED_AND_EXIT(parser_error_reason(either));

    ASSERT_TYPE_EQ(unwrap_json_or_halt(either), JSON_STR);
    PRINT_TEST_PASSED;
    printf("the string is: %s\n", unwrap_json_or_halt(either).data.string);
    deinit_json(either);
}

void test_unclosed_string() {
    PRINT_TEST_HEADER("unclosed string");
    char json_str[] = "\"hello world";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));

    PRINT_TEST_PASSED;
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_unclosed_array() {
    PRINT_TEST_HEADER("unclosed array");
    char json_str[] = "[1,2,3,4,5";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_unclosed_object() {
    PRINT_TEST_HEADER("unclosed object");
    char json_str[] = "{\"key1\": \"value1\", \"key2\": 2";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_array_trailing_comma() {
    PRINT_TEST_HEADER("array with trailing comma");
    char json_str[] = "[1,2,3,4,5,]";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));
    PRINT_TEST_PASSED;
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_object_trailing_comma() {
    PRINT_TEST_HEADER("object with trailing comma");
    char json_str[] = "{\"hello\": 1, \"world\": 2,}";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));
    PRINT_TEST_PASSED;
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_object_key_unclosed() {
    PRINT_TEST_HEADER("object with unclosed key");
    char json_str[] = "{\"hello\": 1, \"world: 2}";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));
    PRINT_TEST_PASSED;
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_array_missing_comma() {
    PRINT_TEST_HEADER("array with missing comma");
    char json_str[] = "[1,2,3 \"uh oh\",5]";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));
    PRINT_TEST_PASSED;
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_object_missing_comma() {
    PRINT_TEST_HEADER("object with missing comma");
    char json_str[] = "{\"key1\": 1, \"key2:\": 2 \"uh oh\": 3}";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));
    PRINT_TEST_PASSED;
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_object_missing_colon() {
    PRINT_TEST_HEADER("object with missing colon");
    char json_str[] = "{\"key1\": 1, \"key2:\": 2, \"uh oh\", 3}";
    PRINT_INPUT(json_str);
    EitherErrJson *either = json_loads(json_str);
    assert(is_error(either));
    PRINT_TEST_PASSED;
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}

void test_empty_input_string() {
    PRINT_TEST_HEADER("empty input string");
    char empty[1];
    empty[0] = '\0';
    EitherErrJson *either = json_loads(empty);
    assert(is_error(either));
    PRINT_TEST_PASSED;
    printf("error (expected): %s\n", parser_error_reason(either));
    deinit_json(either);
}