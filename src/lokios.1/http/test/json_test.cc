#include "../json.h"
#include <tmock/tmock.h>

class tmock_test
{
    TMOCK_TEST(test_empty_obj)
    {
        json::object obj;
        char text[] = "{}";
        size_t len = json::parse_object(text,&obj);
        tmock::assert_equiv(len,sizeof(text)-1);
        TASSERT(obj.empty());
    }

    static void test_one_field(char* text, size_t text_len)
    {
        json::object obj;
        size_t len = json::parse_object(text,&obj);
        tmock::assert_equiv(len,text_len);
        tmock::assert_equiv(obj.size(),1U);
        TASSERT(obj.contains("test"));
        tmock::assert_equiv(obj["test"],"val");
    }

    TMOCK_TEST(test_one_field)
    {
        char text[] = "{ \"test\" : \"val\" }";
        test_one_field(text,sizeof(text)-1);
    }

    TMOCK_TEST(test_one_field_squishy)
    {
        char text[] = "{\"test\":\"val\"}";
        test_one_field(text,sizeof(text)-1);
    }

    TMOCK_TEST(test_two_fields)
    {
        json::object obj;
        char text[] = "{\r\n"
                      "    \"one\" : \"alpha\",\r\n"
                      "    \"two\" : \"beta\"\r\n"
                      "}";
        size_t len  = json::parse_object(text,&obj);
        tmock::assert_equiv(len,sizeof(text)-1);
        tmock::assert_equiv(obj.size(),2U);
        TASSERT(obj.contains("one"));
        TASSERT(obj.contains("two"));
        tmock::assert_equiv(obj["one"],"alpha");
        tmock::assert_equiv(obj["two"],"beta");
    }
};

TMOCK_MAIN();
