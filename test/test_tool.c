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
                    "<datastore-contents>"
                        "<subscriptions>"
                            "<subscription>test1</subscription>"
                        "</subscriptions>"
                    "</datastore-contents>"
            "</push-update>"
        "</notification>";
    char text2[300] =
        "<notification xmlns=\"urn:ietf:params:xml:ns:netconf:notification:1.0\">"
            "<push-change-update xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-push\">"
                "<id>2</id>"
                    "<datastore-contents>"
                        "<subscriptions>"
                            "<subscription>test2</subscription>"
                        "</subscriptions>"
                    "</datastore-contents>"
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

void element_list_clear_trav(const cdada_list_t *list, const void* k, void* opaque) 
{
    (void)list;
    (void)opaque;
 	char* key = (char*)*(void**)k;
    free(key);
} 

void element_list_print_trav(const cdada_list_t *list, const void* k, void* opaque) 
{
    (void)list;
    (void)opaque;
 	char* key = (char*)*(void**)k;
    printf("\nkey is: %s \n", key); 
}

void assert_module_position_in_list(cdada_list_t *yanglib_list, int position, char *expect_val) 
{
    void *element_val;
    cdada_list_get(yanglib_list, position, &element_val);
    assert_string_equal((char*)element_val, expect_val);
}

static void test_create_yanglib_element_list(void **state)
{
    (void) state;
    char text1[1000] = 
    "<yang-library xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-library\">"
      "<module>"
        "<name>ietf-interfaces</name>"
        "<revision>2018-02-20</revision>"
        "<namespace>urn:ietf:params:xml:ns:yang:ietf-interfaces</namespace>"
        "<location>file:///opt/dev/sysrepo/build/repository/yang/ietf-interfaces@2018-02-20.yang</location>"
        "<augmented-by xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby\">ietf-ip</augmented-by>"
        "<augmented-by xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby\">ietf-network-instance</augmented-by>"
      "</module>"
      "<module>"
        "<name>ietf-yang-library</name>"
        "<revision>2019-01-04</revision>"
        "<namespace>urn:ietf:params:xml:ns:yang:ietf-yang-library</namespace>"
        "<location>file:///opt/dev/sysrepo/build/repository/yang/ietf-yang-library@2019-01-04.yang</location>"
        "<augmented-by xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-library-augmentedby\">ietf-yang-library-augmentedby</augmented-by>"
      "</module>"
    "</yang-library>";

    char text2[300] =
    "<yang-library xmlns=\"urn:ietf:params:xml:ns:yang:ietf-yang-library\">"
        "<foo/>"
    "</yang-library>";
    
    /* text1 test */
    /* find augmentedby nodes */
    cdada_list_t* text1_ietf_interfaces_augmentedby_list = parse_yanglib_msg(text1, "ietf-interfaces", "augmented-by");
    assert_int_equal(cdada_list_size(text1_ietf_interfaces_augmentedby_list), 2);
    assert_module_position_in_list(text1_ietf_interfaces_augmentedby_list, 0, "ietf-ip");
    assert_module_position_in_list(text1_ietf_interfaces_augmentedby_list, 1, "ietf-network-instance");
    cdada_list_t* text1_ietf_yang_library_augmentedby_list = parse_yanglib_msg(text1, "ietf-yang-library", "augmented-by");
    assert_int_equal(cdada_list_size(text1_ietf_yang_library_augmentedby_list), 1);
    assert_module_position_in_list(text1_ietf_yang_library_augmentedby_list, 0, "ietf-yang-library-augmentedby");
    /* find revision node */
    cdada_list_t* text1_ietf_yang_library_revision_list = parse_yanglib_msg(text1, "ietf-yang-library", "revision");
    assert_int_equal(cdada_list_size(text1_ietf_yang_library_revision_list), 1);
    assert_module_position_in_list(text1_ietf_yang_library_revision_list, 0, "2019-01-04");


    /* text2 */ 
    cdada_list_t* text2_yanglib_list = parse_yanglib_msg(text2, "ietf-yang-library", "augmentedby");
    assert_null(text2_yanglib_list);


    /* Garbage collector */
    cdada_list_traverse(text1_ietf_interfaces_augmentedby_list, &element_list_clear_trav, NULL); 
    cdada_list_destroy(text1_ietf_interfaces_augmentedby_list);
    cdada_list_traverse(text1_ietf_yang_library_augmentedby_list, &element_list_clear_trav, NULL); 
    cdada_list_destroy(text1_ietf_yang_library_augmentedby_list);
    cdada_list_traverse(text1_ietf_yang_library_revision_list, &element_list_clear_trav, NULL); 
    cdada_list_destroy(text1_ietf_yang_library_revision_list);
}

void test_parse_yangpush_msg_revision(void **state)
{
    (void) state;
    char* ex_yangpush_msg = load_file("../resources/subscription-started.xml");
    char* ex_yangpush_msg1 = load_file("../resources/subscription-started-new.xml");

    struct module_version *test_module_version = parse_yangpush_msg_model_revision(ex_yangpush_msg); 
    struct module_version *test_module_version1 = parse_yangpush_msg_model_revision_by_keyval(ex_yangpush_msg1, "huawei-debug"); 

    assert_non_null(test_module_version);
    assert_string_equal(test_module_version->module_name, "ietf-interfaces");
    assert_string_equal(test_module_version->revision, "2014-05-08");
    assert_null(test_module_version->revision_label);

    assert_non_null(test_module_version1);
    assert_string_equal(test_module_version1->module_name, "huawei-debug");
    assert_string_equal(test_module_version1->revision, "2024-06-19");
    assert_string_equal(test_module_version1->revision_label, "1.0.0");

    /* Garbage Collection */
    free(ex_yangpush_msg);
    free(test_module_version->module_name);
    free(test_module_version->revision);
    free(test_module_version);
    free(ex_yangpush_msg1);
    free(test_module_version1->module_name);
    free(test_module_version1->revision);
    free(test_module_version1->revision_label);
    free(test_module_version1);
}


int main(void){
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_djb2),
        cmocka_unit_test(test_find_node),
        cmocka_unit_test(test_validate_message_structure),
        cmocka_unit_test(test_create_yanglib_element_list),
        cmocka_unit_test(test_parse_yangpush_msg_revision)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}