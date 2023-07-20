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

/* A helper function */
char* load_yang_example(char *filename) {
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

static void test_find_import(void** state)
{
    struct ly_ctx *test1_ctx, *test2_ctx;
    char *test1_bmodule_text;
    int return_check1 = ly_ctx_new("../test/resources/find_import_test1", 1, &test1_ctx);
    int return_check2 = ly_ctx_new("../test/resources/test2", 1, &test2_ctx);
    if(return_check1 != LY_SUCCESS || return_check2 != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    // load b-module yang code from file
    test1_bmodule_text = load_yang_example("../test/resources/find_import_test1/b-module.yang");

    //load a-module and b-module in scenario 1 into test1 context
    ly_ctx_load_module(test1_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "b-module", NULL, NULL);
    struct lys_module* test1_amodule = ly_ctx_get_module_implemented(test1_ctx, "a-module");
    struct lysp_import* test1_amodule_imports = test1_amodule->parsed->imports;
    int test1_num_of_imports = LY_ARRAY_COUNT(test1_amodule_imports);
    
    cdada_map_t* test1_module_set = cdada_map_create(256);
    int hash_of_bmodule = djb2("b-module");
    struct module_info *module_ptr;

    //Test1: A valid test case. Find the import for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_import(test1_num_of_imports, test1_amodule_imports, test1_module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 1);
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_bmodule, (void**)&module_ptr), CDADA_SUCCESS);
    assert_string_equal(module_ptr->yang_code, test1_bmodule_text);
    assert_string_equal(module_ptr->name, "b-module");

    //load b-module in scenario 2 into test2 context
    ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);

    cdada_map_t* test2_module_set = cdada_map_create(256);
    struct lys_module* test2_bmodule = ly_ctx_get_module_implemented(test1_ctx, "b-module");
    struct lysp_import* test2_bmodule_imports = test2_bmodule->parsed->imports;
    int test2_num_of_imports = LY_ARRAY_COUNT(test2_bmodule_imports);

    //Test2: A non-valid test case. Call find_import for b-module which does not have import
    assert_int_equal(libyangpush_find_import(test2_num_of_imports, test2_bmodule_imports, test2_module_set), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    free(module_ptr->yang_code);
    free(module_ptr);
    cdada_map_clear(test1_module_set);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    free(test1_bmodule_text);
}

static void test_find_include(void** state)
{
    struct ly_ctx *test1_ctx, *test2_ctx;
    char *test1_cmodule_text;
    int return_check1 = ly_ctx_new("../test/resources/find_include_test1", 1, &test1_ctx);
    int return_check2 = ly_ctx_new("../test/resources/test2", 1, &test2_ctx);
    if(return_check1 != LY_SUCCESS || return_check2 != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    // load b-module yang code from file
    test1_cmodule_text = load_yang_example("../test/resources/find_include_test1/c-module.yang");

    //load a-module and c-module in scenario 1 into test1 context
    ly_ctx_load_module(test1_ctx, "a-module", NULL, NULL);
    struct lys_module* test1_amodule = ly_ctx_get_module_implemented(test1_ctx, "a-module");
    struct lysp_include* test1_amodule_includes = test1_amodule->parsed->includes;
    int test1_num_of_includes = LY_ARRAY_COUNT(test1_amodule_includes);
    
    cdada_map_t* test1_module_set = cdada_map_create(256);
    int hash_of_bmodule = djb2("c-module");
    struct module_info *module_ptr;

    //Test1: A valid test case. Find the include for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_include(test1_num_of_includes, test1_amodule_includes, test1_module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 1);
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_bmodule, (void**)&module_ptr), CDADA_SUCCESS);
    assert_string_equal(module_ptr->yang_code, test1_cmodule_text);
    assert_string_equal(module_ptr->name, "c-module");

    //load b-module in scenario 2 into test2 context
    ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);

    cdada_map_t* test2_module_set = cdada_map_create(256);
    struct lys_module* test2_bmodule = ly_ctx_get_module_implemented(test2_ctx, "b-module");
    struct lysp_include* test2_bmodule_includes = test2_bmodule->parsed->includes;
    int test2_num_of_includes = LY_ARRAY_COUNT(test2_bmodule_includes);

    //Test2: A non-valid test case. Call find_include for b-module which does not have include
    assert_int_equal(libyangpush_find_include(test2_num_of_includes, test2_bmodule_includes, test2_module_set), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    free(module_ptr->yang_code);
    free(module_ptr);
    cdada_map_clear(test1_module_set);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    free(test1_cmodule_text);
}

static void test_find_reverse_dep(void** state)
{
    struct ly_ctx *test1_ctx, *test2_ctx, *test3_ctx;
    char *test1_dmodule_text, *test3_deviate_text;
    int return_check1 = ly_ctx_new("../test/resources/find_augment_test1", 1, &test1_ctx);
    int return_check2 = ly_ctx_new("../test/resources/test2", 1, &test2_ctx);
    int return_check3 = ly_ctx_new("../test/resources/find_deviate_test3", 1, &test3_ctx);
    if(return_check1 != LY_SUCCESS || return_check2 != LY_SUCCESS || return_check3){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    //load b-module yang code from file
    test1_dmodule_text = load_yang_example("../test/resources/find_augment_test1/d-module.yang");
    test3_deviate_text = load_yang_example("../test/resources/find_deviate_test3/a-module-deviations.yang");

    //load a-module and its augment into context
    ly_ctx_load_module(test1_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "d-module", NULL, NULL);
    struct lys_module* test1_amodule = ly_ctx_get_module_implemented(test1_ctx, "a-module");
    struct lys_module** test1_amodule_augment = test1_amodule->augmented_by;
    int test1_num_of_augment = LY_ARRAY_COUNT(test1_amodule_augment);
    
    cdada_map_t* test1_module_set = cdada_map_create(256);
    int hash_of_dmodule = djb2("d-module");
    struct module_info *module_ptr1;

    //Test1: A valid test case for finding augment. Find the augment for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_reverse_dep(test1_num_of_augment, test1_amodule_augment, test1_module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 1);
    assert_int_equal(cdada_map_find(test1_module_set, &hash_of_dmodule, (void**)&module_ptr1), CDADA_SUCCESS);
    assert_string_equal(module_ptr1->yang_code, test1_dmodule_text);
    assert_string_equal(module_ptr1->name, "d-module");

    //load b-module in scenario 2 into test2 context
    ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);

    cdada_map_t* test2_module_set = cdada_map_create(256);
    struct lys_module* test2_bmodule = ly_ctx_get_module_implemented(test2_ctx, "b-module");
    struct lys_module** test2_bmodule_augment = test2_bmodule->augmented_by;
    int test2_num_of_augment = LY_ARRAY_COUNT(test2_bmodule_augment);

    //Test2: An invalid test case. Call find_include for b-module which does not have include
    assert_int_equal(libyangpush_find_reverse_dep(test2_num_of_augment, test2_bmodule_augment, test2_module_set), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    //load a-module and its deviations into context
    ly_ctx_load_module(test3_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test3_ctx, "a-module-deviations", NULL, NULL);
    struct lys_module* test3_amodule = ly_ctx_get_module_implemented(test3_ctx, "a-module");
    struct lys_module** test3_amodule_deviate = test3_amodule->deviated_by;
    int test3_num_of_deviate = LY_ARRAY_COUNT(test3_amodule_deviate);
    
    cdada_map_t* test3_module_set = cdada_map_create(256);
    int hash_of_deviate = djb2("a-module-deviations");
    struct module_info *module_ptr2;

    //Test3: A valid test case for finding deviate. Find the deviations for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_reverse_dep(test3_num_of_deviate, test3_amodule_deviate, test3_module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test3_module_set), 1);
    assert_int_equal(cdada_map_find(test3_module_set, &hash_of_deviate, (void**)&module_ptr2), CDADA_SUCCESS);
    assert_string_equal(module_ptr2->yang_code, test3_deviate_text);
    assert_string_equal(module_ptr2->name, "a-module-deviations");

    free(module_ptr1->yang_code);
    free(module_ptr1);
    free(module_ptr2->yang_code);
    free(module_ptr2);
    cdada_map_clear(test1_module_set);
    cdada_map_clear(test3_module_set);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    cdada_map_destroy(test3_module_set);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    ly_ctx_destroy(test3_ctx);
    free(test1_dmodule_text);
    free(test3_deviate_text);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_find_import),
        cmocka_unit_test(test_find_include),
        cmocka_unit_test(test_find_reverse_dep)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}