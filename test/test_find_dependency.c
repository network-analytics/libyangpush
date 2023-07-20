#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <cmocka.h>
#include <libyang/libyang.h>
#include <libyang/tree_schema.h>
#include <cdada/map.h>
#include "libyangpush.h"
#include "tool.h"

/* A helper function for loading file from disk */
char* load_yang_example(char *filename) 
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

static void test_load_module_to_map(void** state)
{
    struct ly_ctx *test_ctx;
    if(ly_ctx_new("../test/resources/load_module_test", 1, &test_ctx)){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    char *cmodule_text = load_yang_example("../test/resources/load_module_test/c-module.yang");
    char *amodule_text = load_yang_example("../test/resources/load_module_test/a-module.yang");

    struct lys_module* test_amodule = ly_ctx_load_module(test_ctx, "a-module", NULL, NULL);
    unsigned long hash_of_amodule = djb2("a-module"), hash_of_cmodule = djb2("c-module");
    struct module_info *amodule_info = NULL, *cmodule_info = NULL;
    cdada_map_t* test_map = cdada_map_create(256);

    // Test 1: Valid test case. Load a-module and e-module in the map.
    assert_int_equal(libyangpush_load_module_into_map(test_map, test_amodule), INSERT_SUCCESS);
    assert_int_equal(libyangpush_load_submodule_into_map(test_map, test_amodule->parsed->includes->submodule), INSERT_SUCCESS);
    assert_int_equal(cdada_map_size(test_map), 2);
    assert_int_equal(cdada_map_find(test_map, &hash_of_amodule, (void**)&amodule_info), CDADA_SUCCESS);
    assert_int_equal(cdada_map_find(test_map, &hash_of_cmodule, (void**)&cmodule_info), CDADA_SUCCESS);
    assert_string_equal(amodule_info->yang_code, amodule_text);
    assert_string_equal(cmodule_info->yang_code, cmodule_text);

    // Test 2: Non-valid test case. Passed null pointer into function
    assert_int_equal(libyangpush_load_module_into_map(NULL, NULL), INVALID_PARAMETER);
    assert_int_equal(libyangpush_load_submodule_into_map(NULL, NULL), INVALID_PARAMETER);

    cdada_map_traverse(test_map, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test_map);
    ly_ctx_destroy(test_ctx);
    free(cmodule_text);
    free(amodule_text);
}

static void test_find_import(void** state)
{
    struct ly_ctx *test1_ctx, *test2_ctx;
    int return_check1 = ly_ctx_new("../test/resources/find_import_test1", 1, &test1_ctx);
    int return_check2 = ly_ctx_new("../test/resources/test2", 1, &test2_ctx);
    if(return_check1 != LY_SUCCESS || return_check2 != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    // load b-module yang code from file
    char *test1_bmodule_text = load_yang_example("../test/resources/find_import_test1/b-module.yang");
    char *test1_cmodule_text = load_yang_example("../test/resources/find_import_test1/c-module.yang");
    char *test1_emodule_text = load_yang_example("../test/resources/find_import_test1/e-module.yang");

    //load a-module and b-module in scenario 1 into test1 context
    struct lys_module* test1_amodule = ly_ctx_load_module(test1_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "b-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "e-module", NULL, NULL);
    
    cdada_map_t* test1_module_set = cdada_map_create(256);
    unsigned long hash_of_bmodule = djb2("b-module"), hash_of_cmodule = djb2("c-module"), hash_of_emodule = djb2("e-module");
    struct module_info *bmodule_ptr = NULL, *cmodule_ptr = NULL, *emodule_ptr = NULL;

    //Test1: A valid test case. Find the import for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_import(LY_ARRAY_COUNT(test1_amodule->parsed->imports), test1_amodule->parsed->imports, test1_module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 3); //num of modules in the map
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_bmodule, (void**)&bmodule_ptr), CDADA_SUCCESS); // Check b-module
    assert_string_equal(bmodule_ptr->yang_code, test1_bmodule_text);
    assert_string_equal(bmodule_ptr->name, "b-module");
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_cmodule, (void**)&cmodule_ptr), CDADA_SUCCESS); // Check c-module
    assert_string_equal(cmodule_ptr->yang_code, test1_cmodule_text);
    assert_string_equal(cmodule_ptr->name, "c-module");
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_emodule, (void**)&emodule_ptr), CDADA_SUCCESS); // Check e-module
    assert_string_equal(emodule_ptr->yang_code, test1_emodule_text);
    assert_string_equal(emodule_ptr->name, "e-module");

    //load b-module in scenario 2 into test2 context
    struct lys_module* test2_bmodule = ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);
    cdada_map_t* test2_module_set = cdada_map_create(256);

    //Test2: A non-valid test case. Call find_import for b-module which does not have import
    assert_int_equal(libyangpush_find_import(LY_ARRAY_COUNT(test2_bmodule->parsed->imports), test2_bmodule->parsed->imports, test2_module_set), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    cdada_map_traverse(test1_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    free(test1_bmodule_text);
    free(test1_cmodule_text);
    free(test1_emodule_text);
}

static void test_find_include(void** state)
{
    struct ly_ctx *test1_ctx, *test2_ctx;
    int return_check1 = ly_ctx_new("../test/resources/find_include_test1", 1, &test1_ctx);
    int return_check2 = ly_ctx_new("../test/resources/test2", 1, &test2_ctx);
    if(return_check1 != LY_SUCCESS || return_check2 != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    // load b-module yang code from file
    char *test1_cmodule_text = load_yang_example("../test/resources/find_include_test1/c-module.yang");
    char *test1_ccmodule_text = load_yang_example("../test/resources/find_include_test1/cc-module.yang");
    char *test1_emodule_text = load_yang_example("../test/resources/find_include_test1/e-module.yang");

    //load a-module, c-module, cc-module & e-module in scenario 1 into test1 context
    struct lys_module* test1_amodule = ly_ctx_load_module(test1_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "e-module", NULL, NULL);

    cdada_map_t* test1_module_set = cdada_map_create(256);
    unsigned long hash_of_cmodule = djb2("c-module"), hash_of_ccmodule = djb2("cc-module"), hash_of_emodule = djb2("e-module");
    struct module_info *cmodule_ptr = NULL, *ccmodule_ptr = NULL, *emodule_ptr = NULL;

    //Test1: A valid test case. Find the include for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_include(LY_ARRAY_COUNT(test1_amodule->parsed->includes), test1_amodule->parsed->includes, test1_module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 3);
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_cmodule, (void**)&cmodule_ptr), CDADA_SUCCESS); //for c-module
    assert_string_equal(cmodule_ptr->yang_code, test1_cmodule_text);
    assert_string_equal(cmodule_ptr->name, "c-module");
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_ccmodule, (void**)&ccmodule_ptr), CDADA_SUCCESS); //for cc-module
    assert_string_equal(ccmodule_ptr->yang_code, test1_ccmodule_text);
    assert_string_equal(ccmodule_ptr->name, "cc-module");
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_emodule, (void**)&emodule_ptr), CDADA_SUCCESS); //for e-module
    assert_string_equal(emodule_ptr->yang_code, test1_emodule_text);
    assert_string_equal(emodule_ptr->name, "e-module");

    //load b-module in scenario 2 into test2 context
    struct lys_module* test2_bmodule = ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);
    cdada_map_t* test2_module_set = cdada_map_create(256);

    //Test2: A non-valid test case. Call find_include for b-module which does not have include
    assert_int_equal(libyangpush_find_include(LY_ARRAY_COUNT(test2_bmodule->parsed->includes), test2_bmodule->parsed->includes, test2_module_set), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    cdada_map_traverse(test1_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    free(test1_cmodule_text);
    free(test1_ccmodule_text);
    free(test1_emodule_text);
}

static void test_find_reverse_dep(void** state)
{
    struct ly_ctx *test1_ctx, *test2_ctx, *test3_ctx;
    int return_check1 = ly_ctx_new("../test/resources/find_augment_test1", 1, &test1_ctx);
    int return_check2 = ly_ctx_new("../test/resources/test2", 1, &test2_ctx);
    int return_check3 = ly_ctx_new("../test/resources/find_deviate_test3", 1, &test3_ctx);
    if(return_check1 != LY_SUCCESS || return_check2 != LY_SUCCESS || return_check3){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    char *test1_dmodule_text = load_yang_example("../test/resources/find_augment_test1/d-module.yang");
    char *test1_cmodule_text = load_yang_example("../test/resources/find_augment_test1/c-module.yang");
    char *test1_emodule_text = load_yang_example("../test/resources/find_augment_test1/e-module.yang");
    char *test3_deviate_text = load_yang_example("../test/resources/find_deviate_test3/a-module-deviations.yang");
    char *test3_strmodule_text = load_yang_example("../test/resources/find_deviate_test3/str-type.yang");
    char *test3_cmodule_text = load_yang_example("../test/resources/find_deviate_test3/c-module.yang");

    //load a-module and its augment into context
    struct lys_module* test1_amodule = ly_ctx_load_module(test1_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "d-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "e-module", NULL, NULL);
    
    cdada_map_t* test1_module_set = cdada_map_create(256);
    unsigned long hash_of_dmodule = djb2("d-module"), hash_of_cmodule = djb2("c-module"), hash_of_emodule = djb2("e-module");
    struct module_info *dmodule_ptr = NULL, *cmodule_ptr = NULL, *emodule_ptr = NULL;
    libyangpush_load_module_into_map(test1_module_set, test1_amodule); /* parent module needs to be inserted into map to avoid
                                                                          their reverse dependency identifying them as an import*/

    //Test1: A valid test case for finding augment. Find the augment for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_reverse_dep(LY_ARRAY_COUNT(test1_amodule->augmented_by), test1_amodule->augmented_by, test1_module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 4);
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_dmodule, (void**)&dmodule_ptr), CDADA_SUCCESS);
    assert_string_equal(dmodule_ptr->yang_code, test1_dmodule_text);//for d-module
    assert_string_equal(dmodule_ptr->name, "d-module");
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_cmodule, (void**)&cmodule_ptr), CDADA_SUCCESS);
    assert_string_equal(cmodule_ptr->yang_code, test1_cmodule_text);//for c-module
    assert_string_equal(cmodule_ptr->name, "c-module");
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_emodule, (void**)&emodule_ptr), CDADA_SUCCESS);
    assert_string_equal(emodule_ptr->yang_code, test1_emodule_text);//for e-module
    assert_string_equal(emodule_ptr->name, "e-module");

    //load b-module in scenario 2 into test2 context
    struct lys_module* test2_bmodule = ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);
    cdada_map_t* test2_module_set = cdada_map_create(256);

    //Test2: An invalid test case. Call find_include for b-module which does not have include
    assert_int_equal(libyangpush_find_reverse_dep(LY_ARRAY_COUNT(test2_bmodule->augmented_by), test2_bmodule->augmented_by, test2_module_set), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    //load a-module and its deviations into context
    struct lys_module* test3_amodule = ly_ctx_load_module(test3_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test3_ctx, "str-type", NULL, NULL);
    ly_ctx_load_module(test3_ctx, "a-module-deviations", NULL, NULL);

    cdada_map_t* test3_module_set = cdada_map_create(256);
    unsigned long hash_of_deviate = djb2("a-module-deviations"), hash_of_strmodule = djb2("str-type");
    struct module_info *deviate_cmodule_ptr = NULL, *deviate_strmodule_ptr = NULL, *deviate_amd_ptr = NULL;
    libyangpush_load_module_into_map(test3_module_set, test3_amodule); /* parent module needs to be inserted into map to avoid
                                                                          their reverse dependency identifying them as an import*/

    //Test3: A valid test case for finding deviate. Find the deviations for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_reverse_dep(LY_ARRAY_COUNT(test3_amodule->deviated_by), test3_amodule->deviated_by, test3_module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test3_module_set), 4);
    assert_int_equal(cdada_map_find(test3_module_set, &hash_of_deviate, (void**)&deviate_amd_ptr), CDADA_SUCCESS);
    assert_string_equal(deviate_amd_ptr->yang_code, test3_deviate_text);
    assert_string_equal(deviate_amd_ptr->name, "a-module-deviations");
    assert_int_equal(cdada_map_find(test3_module_set, &hash_of_strmodule, (void**)&deviate_strmodule_ptr), CDADA_SUCCESS);
    assert_string_equal(deviate_strmodule_ptr->yang_code, test3_strmodule_text);
    assert_string_equal(deviate_strmodule_ptr->name, "str-type");
    assert_int_equal(cdada_map_find(test3_module_set, &hash_of_cmodule, (void**)&deviate_cmodule_ptr), CDADA_SUCCESS);
    assert_string_equal(deviate_cmodule_ptr->yang_code, test3_cmodule_text);
    assert_string_equal(deviate_cmodule_ptr->name, "c-module");

    cdada_map_traverse(test1_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_traverse(test3_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    cdada_map_destroy(test3_module_set);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    ly_ctx_destroy(test3_ctx);
    free(test1_dmodule_text);
    free(test1_cmodule_text);
    free(test1_emodule_text);
    free(test3_deviate_text);
    free(test3_strmodule_text);
    free(test3_cmodule_text);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_load_module_to_map),
        cmocka_unit_test(test_find_import),
        cmocka_unit_test(test_find_include),
        cmocka_unit_test(test_find_reverse_dep)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}