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

    /* Text1: A valid exmaple */
    doc = xmlParseDoc((xmlChar*)text1);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    namespaces = node->nsDef;
    assert_string_equal(libyangpush_find_namespace_for_prefix(&namespaces, "a"), "urn:example:yang:a-module");
    xmlFreeDoc(doc);

    /* Text2: A valid exmaple */
    doc = xmlParseDoc((xmlChar*)text2);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    namespaces = node->nsDef;
    assert_string_equal( libyangpush_find_namespace_for_prefix(&namespaces, "prefix"), "some:namespace");
    xmlFreeDoc(doc);

    /* Text3: A non-valid example. Does not have the prefix that we are looking for */
    doc = xmlParseDoc((xmlChar*)text3);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    namespaces = node->nsDef;
    assert_null(libyangpush_find_namespace_for_prefix(&namespaces, "prefix"));

    /* NULL namespace and prefix */
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
    
    char text1[200] = "<datastore-xpath-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\" "
           "xmlns:a=\"urn:example:yang:a-module\">/a:a</datastore-xpath-filter>";
    char text2[200] = "<datastore-xpath-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
           "/a-module:a</datastore-xpath-filter>";
    char text3[100] = "<datastore-xpath-filter>/:</datastore-xpath-filter>";
    char text4[50] = "<anything>something</anything>";

    /* Text1: A valid example for returning namespace */
    doc = xmlParseDoc((xmlChar*)text1);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_xpath(node, &output1), XPATH_NAMESPACE_FOUND);
    assert_string_equal(output1, "urn:example:yang:a-module");
    xmlFreeDoc(doc);
    free(output1);

    /* Text2: A valid example for returning module name */
    doc = xmlParseDoc((xmlChar*)text2);
    node = xmlDocGetRootElement(doc);
    assert_int_equal(libyangpush_parse_xpath(node, &output2), XPATH_MODULE_NAME_FOUND);
    assert_string_equal(output2, "a-module");
    xmlFreeDoc(doc);
    free(output2);

    /* Text3: A non-valid example for datastore-xapth-filter */
    doc = xmlParseDoc((xmlChar*)text3);
    node = xmlDocGetRootElement(doc);
    assert_int_equal(libyangpush_parse_xpath(node, &output3), XPATH_PARSED_FAILED);
    assert_null(output3);
    xmlFreeDoc(doc);

    /* A non-valid example */
    doc = xmlParseDoc((xmlChar*)text4);
    node = xmlDocGetRootElement(doc);
    assert_int_equal(libyangpush_parse_xpath(node, &output4), XPATH_PARSED_FAILED);
    assert_null(output4);
    xmlFreeDoc(doc);

    /* For NULL node */
    assert_int_equal(libyangpush_parse_xpath(NULL, &output5), XPATH_PARSED_FAILED);
    assert_null(output5); 
    return;
}

static void test_parse_subtree(void** state)
{
    xmlDocPtr doc= NULL;
    xmlNodePtr node;
    int num_of_module = 0;
    char **output1 = NULL, **output2 = NULL, **output3 = NULL, **output4 = NULL, **output5 = NULL,
         **output6 = NULL, **output7 = NULL, **output8 = NULL, **output9 = NULL;

    char text1[300] =
            "<datastore-subtree-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<a:a-container xmlns:a=\"urn:example:yang:a-module\" xmlns:d=\"urn:example:yang:d-module\">"
                    "<d:y/>"
                "</a:a-container>"
            "</datastore-subtree-filter>";
    char text2[300] =
           "<datastore-subtree-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<a xmlns=\"urn:example:yang:a-module\">"
                    "<y xmlns:d=\"urn:example:yang:d-module\"/>"
                "</a>"
            "</datastore-subtree-filter>";
    char text3[200] = 
            "<datastore-subtree-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
            "</datastore-subtree-filter>";
    char text4[200] = 
            "<something-wrong xmlns=\"some:namespace\">"
                "<a xmlns=\"urn:example:yang:a-module\">"
                    "<y xmlns:d=\"urn:example:yang:d-module\"/>"
                "</a>"
            "</something-wrong>";
    char text5[300] =
            "<datastore-subtree-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<a:a-container xmlns:d=\"urn:example:yang:d-module\" xmlns:a=\"urn:example:yang:a-module\">"
                    "<d:y/>"
                "</a:a-container>"
            "</datastore-subtree-filter>";
    char text7[300] = 
            "<datastore-subtree-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<a xmlns=\"urn:example:yang:a-module\"/>"
                "<interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\"/>"
            "</datastore-subtree-filter>";
    char text8[400] = 
            "<datastore-subtree-filter xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<a xmlns=\"urn:example:yang:a-module\"/>"
                "<if:interfaces xmlns:if=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\"/>"
                "<in:network-instances xmlns:in=\"urn:ietf:params:xml:ns:yang:ietf-network-instance\"/>"
            "</datastore-subtree-filter>";
    char text9[400] = 
            "<datastore-subtree-filter>"
                "<a:a xmlns:a=\"urn:example:yang:a-module\"/>"
                "<network-instances/>"
            "</datastore-subtree-filter>";

    /* Text1: A valid example. The node name links to 
    a prefix of a namespace specified under the same node */
    doc = xmlParseDoc((xmlChar*)text1);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_subtree(node, &output1), 1);
    assert_string_equal(output1[0], "urn:example:yang:a-module");
    xmlFreeDoc(doc);
    free(output1[0]);
    free(output1);

    /*Text2: A valid example, while the node name has not been linked 
    to any namespace prefix. The default namesapce should be found */
    doc = xmlParseDoc((xmlChar*)text2);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_subtree(node, &output2), 1);
    assert_string_equal(output2[0], "urn:example:yang:a-module");
    free(output2[0]);
    free(output2);
    xmlFreeDoc(doc);

    /* Text3: A non-valid example. No subtree is specified under node
    datastore-subtre-filter. Function should return 0. */
    doc = xmlParseDoc((xmlChar*)text3);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_subtree(node, &output3), 0);
    assert_null(output3); 
    xmlFreeDoc(doc);

    /* Text4: A non-valid example. The root node name is not 'datastore-subtree-filter'.
    Function shoud return 0 */
    doc = xmlParseDoc((xmlChar*)text4);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_subtree(node, &output4), 0);
    assert_null(output4);
    xmlFreeDoc(doc);

    /* Text5: A valid example, while the namespace for the prfix of node name 
    has not been listed in a normally expected order. Function should still 
    be able to find the correct namespace */
    doc = xmlParseDoc((xmlChar*)text5);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_subtree(node, &output5), 1);
    assert_string_equal(output5[0], "urn:example:yang:a-module");
    xmlFreeDoc(doc);
    free(output5[0]);
    free(output5);

    /* For null root node */
    assert_int_equal(libyangpush_parse_subtree(NULL, &output6), 0);
    assert_int_equal(num_of_module, 0);
    assert_null(output6); 

    /* Text7: A valid example. There are two modules in the subtree. 
    Function shoule be able to find the namespaces for both and 
    returns 2 as the number of modules found */
    doc = xmlParseDoc((xmlChar*)text7);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_subtree(node, &output7), 2);
    assert_string_equal(output7[0], "urn:example:yang:a-module");
    assert_string_equal(output7[1], "urn:ietf:params:xml:ns:yang:ietf-interfaces");
    xmlFreeDoc(doc);
    free(output7[0]);
    free(output7[1]);
    free(output7);

    /* Text8: A valid example. There are three modules in the subtree. Some of
    them have linked to prefix while some not. Function should found all three
    namespaces and return 3 */
    doc = xmlParseDoc((xmlChar*)text8);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_subtree(node, &output8), 3);
    assert_string_equal(output8[0], "urn:example:yang:a-module");
    assert_string_equal(output8[1], "urn:ietf:params:xml:ns:yang:ietf-interfaces");
    assert_string_equal(output8[2], "urn:ietf:params:xml:ns:yang:ietf-network-instance");
    xmlFreeDoc(doc);
    free(output8[0]);
    free(output8[1]);
    free(output8[2]);
    free(output8);

    /* Text 9: A non-valid example. There is a node in subtree that does not 
    have namespace at all(neither from itself nor from its parent node). 
    The function should return 0 without any memory leak */
    doc = xmlParseDoc((xmlChar*)text9);
    node = xmlDocGetRootElement(doc); //parsing the xml text
    assert_int_equal(libyangpush_parse_subtree(node, &output9), 0);
    xmlFreeDoc(doc);
    return;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pattern_match),
        cmocka_unit_test(test_parse_xpath),
        cmocka_unit_test(test_find_namespace),
        cmocka_unit_test(test_parse_subtree)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}