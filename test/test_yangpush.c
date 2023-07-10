#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <cmocka.h>
#include "libyangpush.h"

static void test_pattern_match(void** state)
{
    char pattern1[20] = "/[^/:]*?:";
    char text1[50] = "/ietf-interfaces:myinterfaces/interfaces";
    char text2[50] = "ietf-interfaces:myinterface/interfaces";
    char text3[50] = "?????";
    char text4[50] = "/:///anything";
    char text5[100] = "/ietf-interfaces:interfaces/interface/ietf-ip:ipv4";
    char text6[5] = "";
    char* output1 = NULL, *output4 = NULL, *output5 = NULL;

    //For text1
    output1 = libyangpush_pattern_match(pattern1, text1);
    assert_string_equal(output1, "/ietf-interfaces:");
    free(output1);
    //For text2
    assert_null(libyangpush_pattern_match(pattern1, text2));
    //For text3
    assert_null(libyangpush_pattern_match(pattern1, text3));
    //For text4
    output4 = libyangpush_pattern_match(pattern1, text4);
    assert_string_equal(output4, "/:");
    free(output4);
    //For text5
    output5 = libyangpush_pattern_match(pattern1, text5);
    assert_string_equal(output5, "/ietf-interfaces:");
    free(output5);
    //For text6
    assert_null(libyangpush_pattern_match(pattern1, text6));
    //If pattern is null
    assert_null(libyangpush_pattern_match(NULL, text6));
    //If string is null
    assert_null(libyangpush_pattern_match(pattern1, NULL));
    return;
}

static void test_find_namespace(void **state)
{
    xmlDocPtr doc= NULL;
    xmlNodePtr node;
    xmlNs* namespaces;
    char text1[200] = "<datastore-xpath-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\" "
           "xmlns:b=\"urn:example:yang:b-module\" xmlns:a=\"urn:example:yang:a-module\">/a:something</datastore-xpath-filter>";
    char text2[200] = "<some-field xmlns=\"some:namespace\" xmlns:prefix=\"some:namespace\">/prefix:container</some-field>";
    char text3[200] = "<some-field xmlns=\"some:namespace\" xmlns:pf=\"some:namespace\">/prefix:container</some-field>";

    //text1
    doc = xmlParseDoc((xmlChar*)text1);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    namespaces = node->nsDef;
    assert_string_equal(libyangpush_find_namespace_for_prefix(&namespaces, "a"), "urn:example:yang:a-module");
    xmlFreeDoc(doc);

    //text2
    doc = xmlParseDoc((xmlChar*)text2);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    namespaces = node->nsDef;
    assert_string_equal( libyangpush_find_namespace_for_prefix(&namespaces, "prefix"), "some:namespace");
    xmlFreeDoc(doc);

    //text3
    doc = xmlParseDoc((xmlChar*)text3);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    namespaces = node->nsDef;
    assert_null(libyangpush_find_namespace_for_prefix(&namespaces, "prefix"));

    //NULL namespace and prefix
    assert_null(libyangpush_find_namespace_for_prefix(NULL, "prefix"));
    assert_null(libyangpush_find_namespace_for_prefix(&namespaces, NULL));

    xmlFreeDoc(doc);
    return;
}

static void test_parse_xpath(void** state)
{
    xmlDocPtr doc= NULL;
    xmlNodePtr node;
    char* output1 = NULL, *output2 = NULL, *output3 = NULL, *output4 = NULL, *output5 = NULL;
    //A valid example for returning namespace
    char text1[200] = "<datastore-xpath-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\" "
           "xmlns:a=\"urn:example:yang:a-module\">/a:a</datastore-xpath-filter>";
    //A valid example for returning module name
    char text2[200] = "<datastore-xpath-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
           "/a-module:a</datastore-xpath-filter>";
    //A non-valid example for datastore-xapth-filter
    char text3[100] = "<datastore-xpath-filter>/:</datastore-xpath-filter>";
    //A non-valid example
    char text4[50] = "<anything>something</anything>";

    //text1
    doc = xmlParseDoc((xmlChar*)text1);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_xpath(node, &output1), 1);
    assert_string_equal(output1, "urn:example:yang:a-module");
    xmlFreeDoc(doc);
    free(output1);

    //text2
    doc = xmlParseDoc((xmlChar*)text2);
    node = xmlDocGetRootElement(doc);
    assert_int_equal(libyangpush_parse_xpath(node, &output2), 2);
    assert_string_equal(output2, "a-module");
    xmlFreeDoc(doc);
    free(output2);

    //text3
    doc = xmlParseDoc((xmlChar*)text3);
    node = xmlDocGetRootElement(doc);
    assert_int_equal(libyangpush_parse_xpath(node, &output3), 0);
    assert_null(output3);
    xmlFreeDoc(doc);

    //text4
    doc = xmlParseDoc((xmlChar*)text4);
    node = xmlDocGetRootElement(doc);
    assert_int_equal(libyangpush_parse_xpath(node, &output4), 0);
    assert_null(output4);
    xmlFreeDoc(doc);

    //For NULL node
    assert_int_equal(libyangpush_parse_xpath(NULL, &output5), 0);
    assert_null(output5); 
    return;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pattern_match),
        cmocka_unit_test(test_parse_xpath),
        cmocka_unit_test(test_find_namespace)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}