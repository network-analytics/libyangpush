#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <cmocka.h>
#include <libyang/libyang.h>
#include <libyang/tree_schema.h>
#include <cdada/map.h>
#include <cdada/list.h>
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

/* Check the correctness of module stored in 'module_set'*/
cdada_list_t* assert_found_module(char *module_name, char *yang_code, cdada_map_t *module_set, cdada_list_t *reg_list, int position){
    unsigned long hash = djb2(module_name);
    struct module_info *module_ptr = NULL;
    assert_int_equal(cdada_map_find(module_set, &hash, (void**)&module_ptr), CDADA_SUCCESS); // If module is inthe map or not
    assert_string_equal(module_ptr->yang_code, yang_code);
    assert_string_equal(module_ptr->name, module_name);
    if(reg_list != NULL){
        cdada_list_get(reg_list, position, &hash);
        assert_int_equal(hash, djb2(module_name));
    }
    return module_ptr->dependency_list;
}

static void test_load_module_to_map(void** state)
{
    (void) state;
    struct ly_ctx *test_ctx;
    if(ly_ctx_new("../test/resources/load_module_test", 1, &test_ctx)){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    char *cmodule_text = load_yang_example("../test/resources/load_module_test/c-module.yang");
    char *amodule_text = load_yang_example("../test/resources/load_module_test/a-module.yang");

    struct lys_module* test_amodule = ly_ctx_load_module(test_ctx, "a-module", NULL, NULL);
    cdada_map_t* test_map = cdada_map_create(unsigned long);

    // Test 1: Valid test case. Load a-module and e-module in the map.
    assert_non_null(libyangpush_load_module_into_map(test_map, test_amodule));
    assert_non_null(libyangpush_load_submodule_into_map(test_map, test_amodule->parsed->includes->submodule));

    assert_int_equal(cdada_map_size(test_map), 2);
    assert_found_module("a-module", amodule_text, test_map, NULL, 0);
    assert_found_module("c-module", cmodule_text, test_map, NULL, 0);

    // Test 2: Non-valid test case. Passed null pointer into function
    assert_null(libyangpush_load_module_into_map(NULL, NULL));
    assert_null(libyangpush_load_module_into_map(NULL, NULL));

    cdada_map_traverse(test_map, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test_map);
    ly_ctx_destroy(test_ctx);
    free(cmodule_text);
    free(amodule_text);
}

static void test_find_import(void** state)
{
    (void) state;
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
    
    cdada_map_t* test1_module_set = cdada_map_create(unsigned long);
    cdada_list_t *test1_reg_list = cdada_list_create(unsigned long);
    cdada_list_t *test1_dep_list = cdada_list_create(unsigned long);

    //Test1: A valid test case. Find the import for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_import(test1_amodule->parsed->imports, test1_module_set, test1_reg_list, test1_dep_list), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 3); //num of modules in the map
    assert_int_equal(cdada_list_size(test1_reg_list), 3);
    cdada_list_t *e_module_dep_list = assert_found_module("e-module", test1_emodule_text, test1_module_set, test1_reg_list, 0);
    cdada_list_t *c_module_dep_list = assert_found_module("c-module", test1_cmodule_text, test1_module_set, test1_reg_list, 1);
    cdada_list_t *b_module_dep_list = assert_found_module("b-module", test1_bmodule_text, test1_module_set, test1_reg_list, 2);
    //For dependency_list of each module
    assert_int_equal(cdada_list_size(test1_dep_list), 1);
    assert_found_module("b-module", test1_bmodule_text, test1_module_set, test1_dep_list, 0);
    assert_int_equal(cdada_list_size(e_module_dep_list), 0); // e-module's dep_list's empty
    assert_int_equal(cdada_list_size(c_module_dep_list), 0); // c-module's dep_list's empty
    assert_int_equal(cdada_list_size(b_module_dep_list), 2); // e-module's dep_list has two modules
    assert_found_module("e-module", test1_emodule_text, test1_module_set, b_module_dep_list, 0);
    assert_found_module("c-module", test1_cmodule_text, test1_module_set, b_module_dep_list, 1);

    //load b-module in scenario 2 into test2 context
    struct lys_module* test2_bmodule = ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);
    cdada_map_t* test2_module_set = cdada_map_create(unsigned long);
    cdada_list_t *test2_reg_list = cdada_list_create(unsigned long);
    cdada_list_t *test2_dep_list = cdada_list_create(unsigned long);

    //Test2: A non-valid test case. Call find_import for b-module which does not have import
    assert_int_equal(libyangpush_find_import(test2_bmodule->parsed->imports, test2_module_set, test2_reg_list, test2_dep_list), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    cdada_map_traverse(test1_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    cdada_list_destroy(test1_reg_list);
    cdada_list_destroy(test1_dep_list);
    cdada_list_destroy(test2_reg_list);
    cdada_list_destroy(test2_dep_list);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    free(test1_bmodule_text);
    free(test1_cmodule_text);
    free(test1_emodule_text);
}

static void test_find_include(void** state)
{
    (void) state;
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

    cdada_map_t *test1_module_set = cdada_map_create(unsigned long);
    cdada_list_t *test1_reg_list = cdada_list_create(unsigned long);
    cdada_list_t *test1_dep_list = cdada_list_create(unsigned long);

    //Test1: A valid test case. Find the include for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_include(test1_amodule->parsed->includes, test1_module_set, test1_reg_list, test1_dep_list), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 3);
    assert_int_equal(cdada_list_size(test1_reg_list), 3);
    cdada_list_t *e_module_dep_list = assert_found_module("e-module", test1_emodule_text, test1_module_set, test1_reg_list, 0);
    cdada_list_t *cc_module_dep_list = assert_found_module("cc-module", test1_ccmodule_text, test1_module_set, test1_reg_list, 1);
    cdada_list_t *c_module_dep_list = assert_found_module("c-module", test1_cmodule_text, test1_module_set, test1_reg_list, 2);
    //For dependency_list of each module
    assert_int_equal(cdada_list_size(test1_dep_list), 2);
    assert_found_module("c-module", test1_cmodule_text, test1_module_set, test1_dep_list, 0);
    assert_found_module("cc-module", test1_ccmodule_text, test1_module_set, test1_dep_list, 1);
    assert_int_equal(cdada_list_size(e_module_dep_list), 0); // e-module's dep_list's empty
    assert_int_equal(cdada_list_size(cc_module_dep_list), 0); // c-module's dep_list's empty
    assert_int_equal(cdada_list_size(c_module_dep_list), 2); // e-module's dep_list has two modules
    assert_found_module("e-module", test1_emodule_text, test1_module_set, c_module_dep_list, 0);
    assert_found_module("cc-module", test1_ccmodule_text, test1_module_set, c_module_dep_list, 1);

    //load b-module in scenario 2 into test2 context
    struct lys_module* test2_bmodule = ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);
    cdada_map_t* test2_module_set = cdada_map_create(unsigned long);
    cdada_list_t *test2_reg_list = cdada_list_create(unsigned long);
    cdada_list_t *test2_dep_list = cdada_list_create(unsigned long);

    //Test2: A non-valid test case. Call find_include for b-module which does not have include
    assert_int_equal(libyangpush_find_include(test2_bmodule->parsed->includes, test2_module_set, test2_reg_list, test2_dep_list), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    cdada_map_traverse(test1_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    cdada_list_destroy(test1_reg_list);
    cdada_list_destroy(test1_dep_list);
    cdada_list_destroy(test2_reg_list);
    cdada_list_destroy(test2_dep_list);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    free(test1_cmodule_text);
    free(test1_ccmodule_text);
    free(test1_emodule_text);
}

static void test_find_reverse_dep(void** state)
{
    (void) state;
    struct ly_ctx *test1_ctx, *test2_ctx, *test3_ctx;
    int return_check1 = ly_ctx_new("../test/resources/find_augment_test1", 1, &test1_ctx);
    int return_check2 = ly_ctx_new("../test/resources/test2", 1, &test2_ctx);
    int return_check3 = ly_ctx_new("../test/resources/find_deviate_test3", 1, &test3_ctx);
    if(return_check1 != LY_SUCCESS || return_check2 != LY_SUCCESS || return_check3 != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    char *test1_amodule_text = load_yang_example("../test/resources/find_augment_test1/a-module.yang");
    char *test1_dmodule_text = load_yang_example("../test/resources/find_augment_test1/d-module.yang");
    char *test1_cmodule_text = load_yang_example("../test/resources/find_augment_test1/c-module.yang");
    char *test1_emodule_text = load_yang_example("../test/resources/find_augment_test1/e-module.yang");
    char *test3_amodule_text = load_yang_example("../test/resources/find_deviate_test3/a-module.yang");
    char *test3_deviate_text = load_yang_example("../test/resources/find_deviate_test3/a-module-deviations.yang");
    char *test3_strmodule_text = load_yang_example("../test/resources/find_deviate_test3/str-type.yang");
    char *test3_cmodule_text = load_yang_example("../test/resources/find_deviate_test3/c-module.yang");

    //load a-module and its augment into context
    struct lys_module* test1_amodule = ly_ctx_load_module(test1_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "d-module", NULL, NULL);
    ly_ctx_load_module(test1_ctx, "e-module", NULL, NULL);
    
    cdada_map_t* test1_module_set = cdada_map_create(unsigned long);
    cdada_list_t* test1_reg_list = cdada_list_create(unsigned long);
    cdada_list_t* test1_dep_list = cdada_list_create(unsigned long);
    assert_non_null(libyangpush_load_module_into_map(test1_module_set, test1_amodule));

    //Test1: A valid test case for finding augment. Find the augment for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_reverse_dep(test1_amodule->augmented_by, test1_module_set, test1_reg_list, test1_dep_list), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test1_module_set), 4);
    assert_int_equal(cdada_list_size(test1_reg_list), 3);
    cdada_list_t *d_module_dep_list = assert_found_module("d-module", test1_dmodule_text, test1_module_set, test1_reg_list, 2);
    cdada_list_t *e_module_dep_list = assert_found_module("e-module", test1_emodule_text, test1_module_set, test1_reg_list, 0);
    cdada_list_t *c_module_dep_list = assert_found_module("c-module", test1_cmodule_text, test1_module_set, test1_reg_list, 1);
    //For dependency_list of each module
    assert_int_equal(cdada_list_size(test1_dep_list), 1); // for a-module's dep_list
    assert_found_module("d-module", test1_dmodule_text, test1_module_set, test1_dep_list, 0);
    assert_int_equal(cdada_list_size(e_module_dep_list), 0); // e-module's dep_list's empty
    assert_int_equal(cdada_list_size(c_module_dep_list), 0); // c-module's dep_list's empty
    assert_int_equal(cdada_list_size(d_module_dep_list), 3); // d-module's dep_list has three modules(a, e, c)
    assert_found_module("a-module", test1_amodule_text, test1_module_set, d_module_dep_list, 0);
    assert_found_module("e-module", test1_emodule_text, test1_module_set, d_module_dep_list, 1);
    assert_found_module("c-module", test1_cmodule_text, test1_module_set, d_module_dep_list, 2);

    //load b-module in scenario 2 into test2 context
    struct lys_module* test2_bmodule = ly_ctx_load_module(test2_ctx, "b-module", NULL, NULL);
    cdada_map_t* test2_module_set = cdada_map_create(unsigned long);
    cdada_list_t* test2_reg_list = cdada_list_create(unsigned long);
    cdada_list_t* test2_dep_list = cdada_list_create(unsigned long);

    //Test2: An invalid test case. Call find_include for b-module which does not have include
    assert_int_equal(libyangpush_find_reverse_dep(test2_bmodule->augmented_by, test2_module_set, test2_reg_list, test2_dep_list), INVALID_PARAMETER);
    assert_int_equal(cdada_map_empty(test2_module_set), 1);

    //load a-module and its deviations into context
    struct lys_module* test3_amodule = ly_ctx_load_module(test3_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test3_ctx, "str-type", NULL, NULL);
    ly_ctx_load_module(test3_ctx, "a-module-deviations", NULL, NULL);

    cdada_map_t* test3_module_set = cdada_map_create(unsigned long);
    cdada_list_t* test3_reg_list = cdada_list_create(unsigned long);
    cdada_list_t* test3_dep_list = cdada_list_create(unsigned long);
    assert_non_null(libyangpush_load_module_into_map(test3_module_set, test3_amodule));

    //Test3: A valid test case for finding deviate. Find the deviations for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_reverse_dep(test3_amodule->deviated_by, test3_module_set, test3_reg_list, test3_dep_list), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test3_module_set), 4);
    assert_int_equal(cdada_list_size(test3_reg_list), 3);
    cdada_list_t *str_deviate_dep_list = assert_found_module("str-type", test3_strmodule_text, test3_module_set, test3_reg_list, 0);
    cdada_list_t *c_deviate_dep_list = assert_found_module("c-module", test3_cmodule_text, test3_module_set, test3_reg_list, 1);
    cdada_list_t *a_deviate_dep_list = assert_found_module("a-module-deviations", test3_deviate_text, test3_module_set, test3_reg_list, 2);
    //For dependency_list of each module
    assert_int_equal(cdada_list_size(test3_dep_list), 1); //for a-module's dep_list
    assert_found_module("a-module-deviations", test3_deviate_text, test3_module_set, test3_dep_list, 0); 
    assert_int_equal(cdada_list_size(c_deviate_dep_list), 0); // e-module's dep_list's empty
    assert_int_equal(cdada_list_size(str_deviate_dep_list), 0); // c-module's dep_list's empty
    assert_int_equal(cdada_list_size(a_deviate_dep_list), 3); // e-module's dep_list has two modules
    assert_found_module("a-module", test3_amodule_text, test3_module_set, a_deviate_dep_list, 0);
    assert_found_module("str-type", test3_strmodule_text, test3_module_set, a_deviate_dep_list, 1);
    assert_found_module("c-module", test3_cmodule_text, test3_module_set, a_deviate_dep_list, 2);

    cdada_map_traverse(test1_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_traverse(test3_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test1_module_set);
    cdada_map_destroy(test2_module_set);
    cdada_map_destroy(test3_module_set);
    cdada_list_destroy(test1_reg_list);
    cdada_list_destroy(test2_reg_list);
    cdada_list_destroy(test3_reg_list);
    cdada_list_destroy(test1_dep_list);
    cdada_list_destroy(test2_dep_list);
    cdada_list_destroy(test3_dep_list);
    ly_ctx_destroy(test1_ctx);
    ly_ctx_destroy(test2_ctx);
    ly_ctx_destroy(test3_ctx);
    free(test1_amodule_text);
    free(test1_dmodule_text);
    free(test1_cmodule_text);
    free(test1_emodule_text);
    free(test3_amodule_text);
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