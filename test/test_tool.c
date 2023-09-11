#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <cmocka.h>
#include "tool.h"


/* A helper function for loading file from disk */
char* load_file(char *filename) 
{
    char* text;

    FILE *fptr = fopen(filename, "r");
    fseek(fptr, 0, SEEK_END);
    int flen = ftell(fptr); //file length
    fseek(fptr, 0, SEEK_SET); 

    text = (char*)calloc((flen+1), sizeof(char));
    fread(text, flen, 1, fptr); 
    fclose(fptr);

    return text;
}


static void test_djb2(void** state){
    (void) state;
    assert_int_equal(djb2("helloworld"), 0x72711934fffdad81);
    assert_int_not_equal(djb2("libyangpush"), 0);
    return;
}

static void test_find_node(void** state){
    (void) state;
    char *ex_msg = load_file("../resources/push-update.xml");
    xmlDocPtr ex_xml_msg = xmlParseDoc((xmlChar*)ex_msg);
    xmlNodePtr root_node = xmlDocGetRootElement(ex_xml_msg);

    xmlNodePtr sub_node = xml_find_node(root_node, "subscription");
    assert_non_null(sub_node);

    xmlNodePtr sub_id_node = xml_find_node(sub_node, "id");
    assert_non_null(sub_id_node);
    char *sub_id = (char*)xmlNodeGetContent(sub_id_node);
    assert_string_equal(sub_id, "2222");
    free(sub_id);

    sub_node = sub_node->next;
    sub_id_node = xml_find_node(sub_node, "id");
    assert_non_null(sub_id_node);
    sub_id = (char*)xmlNodeGetContent(sub_id_node);
    assert_string_equal(sub_id, "6666");

    free(sub_id);
    xmlFreeDoc(ex_xml_msg);
    free(ex_msg);
}

static void test_validate_message_structure(void** state){
    (void)state;
    char text1[300] =
        "<notification xmlns=\"urn:ietf:params:xml:ns:netconf:notification:1.0\">"
            "<push-update xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<id>1</id>"
                    "<datastore-contents>test1</datastore-contents>"
            "</push-update>"
        "</notification>";
    char text2[300] =
        "<notification xmlns=\"urn:ietf:params:xml:ns:netconf:notification:1.0\">"
            "<push-change-update xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<id>2</id>"
                    "<datastore-contents>test2</datastore-contents>"
            "</push-change-update>"
        "</notification>";
    char text3[300] =
        "<push-update xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
            "<id>3</id>"
            "<datastore-contents>test1</datastore-contents>"
        "</push-update>";
    char text4[300] =
        "<notification xmlns=\"urn:ietf:params:xml:ns:netconf:notification:1.0\">"
            "<id>4</id>"
            "<datastore-contents>test1</datastore-contents>"
        "</notification>";
    char text5[300] =
        "<notification xmlns=\"urn:ietf:params:xml:ns:netconf:notification:1.0\">"
            "<push-update xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                    "<datastore-contents>test1</datastore-contents>"
            "</push-update>"
        "</notification>";
    char text6[300] =
        "<notification xmlns=\"urn:ietf:params:xml:ns:netconf:notification:1.0\">"
            "<push-update xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<id>5</id>"
            "</push-update>"
        "</notification>";

    xmlNodePtr test1_datastore, test2_datastore, test3_datastore, 
                    test4_datastore, test5_datastore, test6_datastore;
    char *test1_datastore_content, *test2_datastore_content;
    int test1_sub_id, test2_sub_id, test3_sub_id, test4_sub_id, test5_sub_id, test6_sub_id;

    assert_int_equal(validate_message_structure(text1, &test1_datastore, &test1_sub_id), MESSAGE_STRUCTURE_VALID);
    test1_datastore_content = (char*)xmlNodeGetContent(test1_datastore);
    assert_string_equal(test1_datastore_content, "test1");
    assert_int_equal(test1_sub_id, 1);

    assert_int_equal(validate_message_structure(text2, &test2_datastore, &test2_sub_id), MESSAGE_STRUCTURE_VALID);
    test2_datastore_content = (char*)xmlNodeGetContent(test2_datastore);
    assert_string_equal(test2_datastore_content, "test2");
    assert_int_equal(test2_sub_id, 2);

    assert_int_equal(validate_message_structure(text3, &test3_datastore, &test3_sub_id), MESSAGE_STRUCTURE_INVALID);
    assert_int_equal(validate_message_structure(text4, &test4_datastore, &test4_sub_id), MESSAGE_STRUCTURE_INVALID);
    assert_int_equal(validate_message_structure(text5, &test5_datastore, &test5_sub_id), MESSAGE_STRUCTURE_INVALID);
    assert_int_equal(validate_message_structure(text6, &test6_datastore, &test6_sub_id), MESSAGE_STRUCTURE_INVALID);

    free(test1_datastore_content);
    free(test2_datastore_content);
    xmlFreeNode(test1_datastore);
    xmlFreeNode(test2_datastore);
}

int main(void){
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_djb2),
        cmocka_unit_test(test_find_node),
        cmocka_unit_test(test_validate_message_structure)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}