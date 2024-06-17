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

struct module_info* get_dependency_list(cdada_map_t *module_set, char *module_name)
{
    unsigned long hash = djb2(module_name);
    struct module_info *module_info_ptr = NULL;
    cdada_map_find(module_set, &hash, (void**)&module_info_ptr);
    return module_info_ptr->dependency_list;
}

/* Check the correctness of module stored in 'module_set'*/
void assert_found_module_in_map(char *module_name, char *yang_code, cdada_map_t *module_set, unsigned long hash)
{
    struct module_info *module_ptr = NULL;
    assert_int_equal(cdada_map_find(module_set, &hash, (void**)&module_ptr), CDADA_SUCCESS); // If module is in the map or not
    assert_string_equal(module_ptr->yang_code, yang_code);
    assert_string_equal(module_ptr->name, module_name);
}

/* Check the correctness of module stored in 'module_set'*/
void assert_module_position_in_list(char *module_name, cdada_list_t *reg_list, int position){
    unsigned long hash;
    cdada_list_get(reg_list, position, &hash);
    assert_int_equal(hash, djb2(module_name));
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
    struct module_info *test_amodule_info_ptr = libyangpush_load_module_into_map(test_map, test_amodule, djb2("a-module"));
    test_amodule_info_ptr->dependency_list = cdada_list_create(unsigned long);
    struct module_info *test_cmodule_info_ptr = libyangpush_load_submodule_into_map(test_map, test_amodule->parsed->includes->submodule, djb2("c-module"));
    test_cmodule_info_ptr->dependency_list = cdada_list_create(unsigned long);
    assert_non_null(test_amodule_info_ptr);
    assert_non_null(test_cmodule_info_ptr);

    assert_int_equal(cdada_map_size(test_map), 2);
    assert_found_module_in_map("a-module", amodule_text, test_map, djb2("a-module"));
    assert_found_module_in_map("c-module", cmodule_text, test_map, djb2("c-module"));

    // Test 2: Non-valid test case. Passed null pointer into function
    assert_null(libyangpush_load_module_into_map(NULL, NULL, 0));

    cdada_map_traverse(test_map, &libyangpush_trav_clear_module_set_map, NULL);
    cdada_map_destroy(test_map);
    ly_ctx_destroy(test_ctx);
    free(cmodule_text);
    free(amodule_text);
}

static void test_find_module_direct_dep(void** state)
{
    (void) state;
    struct ly_ctx *test_ctx;
    if(ly_ctx_new("../test/resources/find_import_test1", 1, &test_ctx) != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    // load b-module yang code from file
    char *test_amodule_text = load_yang_example("../test/resources/find_import_test1/a-module.yang");
    char *test_bmodule_text = load_yang_example("../test/resources/find_import_test1/b-module.yang");
    char *test_cmodule_text = load_yang_example("../test/resources/find_import_test1/c-module.yang");
    char *test_emodule_text = load_yang_example("../test/resources/find_import_test1/e-module.yang");

    //load a-module and b-module in scenario 1 into test1 context
    struct lys_module* test_amodule = ly_ctx_load_module(test_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "b-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "e-module", NULL, NULL);

    cdada_map_t* test_module_set = cdada_map_create(unsigned long);
    cdada_list_t *test_reg_list = cdada_list_create(unsigned long);

    //Test1: A valid test case. Find the import for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_module_direct_dep(test_amodule, test_module_set, test_reg_list), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test_module_set), 4); //num of modules in the map
    assert_int_equal(cdada_list_size(test_reg_list), 4);
    assert_found_module_in_map("e-module", test_emodule_text, test_module_set, djb2("e-module"));
    assert_module_position_in_list("e-module", test_reg_list, 3);
    assert_found_module_in_map("c-module", test_cmodule_text, test_module_set, djb2("c-module"));
    assert_module_position_in_list("c-module", test_reg_list, 2);
    assert_found_module_in_map("b-module", test_bmodule_text, test_module_set, djb2("b-module"));
    assert_module_position_in_list("b-module", test_reg_list, 1);
    assert_found_module_in_map("a-module", test_amodule_text, test_module_set, djb2("a-module"));
    assert_module_position_in_list("a-module", test_reg_list, 0);

    cdada_list_t *e_module_dep_list = get_dependency_list(test_module_set, "e-module");
    assert_int_equal(cdada_list_size(e_module_dep_list), 0);
    cdada_list_t *c_module_dep_list = get_dependency_list(test_module_set, "c-module");
    assert_int_equal(cdada_list_size(c_module_dep_list), 0);

    cdada_list_t *b_module_dep_list = get_dependency_list(test_module_set, "b-module");
    assert_int_equal(cdada_list_size(b_module_dep_list), 2);
    assert_module_position_in_list("e-module", b_module_dep_list, 0);
    assert_module_position_in_list("c-module", b_module_dep_list, 1);

    cdada_list_t *a_module_dep_list = get_dependency_list(test_module_set, "a-module");
    assert_int_equal(cdada_list_size(a_module_dep_list), 1);
    assert_module_position_in_list("b-module", a_module_dep_list, 0);

    //Test2: A non-valid test case. Call find_import for b-module which does not have import
    assert_int_equal(libyangpush_find_module_direct_dep(NULL, NULL, NULL), INVALID_PARAMETER);

    cdada_map_traverse(test_module_set, &libyangpush_trav_clear_module_set_map, NULL);
    cdada_map_destroy(test_module_set);
    cdada_list_destroy(test_reg_list);
    ly_ctx_destroy(test_ctx);
    free(test_amodule_text);
    free(test_bmodule_text);
    free(test_cmodule_text);
    free(test_emodule_text);
}


static void test_find_submodule_direct_dep(void** state)
{
    (void) state;
    struct ly_ctx *test_ctx;
    if(ly_ctx_new("../test/resources/find_include_test1", 1, &test_ctx) != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    // load b-module yang code from file
    char *test_cmodule_text = load_yang_example("../test/resources/find_include_test1/c-module.yang");
    char *test_ccmodule_text = load_yang_example("../test/resources/find_include_test1/cc-module.yang");
    char *test_emodule_text = load_yang_example("../test/resources/find_include_test1/e-module.yang");

    //load a-module, c-module, cc-module & e-module in scenario 1 into test1 context
    struct lys_module* test_amodule = ly_ctx_load_module(test_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "e-module", NULL, NULL);

    cdada_map_t *test_module_set = cdada_map_create(unsigned long);
    cdada_list_t *test_reg_list = cdada_list_create(unsigned long);
    cdada_list_t *test_dep_list = cdada_list_create(unsigned long);

    //Test1: A valid test case. Find the include for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_submodule_direct_dep(test_amodule->parsed->includes->submodule, test_module_set, test_reg_list), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test_module_set), 3); //num of modules in the map
    assert_int_equal(cdada_list_size(test_reg_list), 3);
    assert_found_module_in_map("e-module", test_emodule_text, test_module_set, djb2("e-module"));
    assert_module_position_in_list("e-module", test_reg_list, 2);
    assert_found_module_in_map("cc-module", test_ccmodule_text, test_module_set, djb2("cc-module"));
    assert_module_position_in_list("cc-module", test_reg_list, 1);
    assert_found_module_in_map("c-module", test_cmodule_text, test_module_set, djb2("c-module"));
    assert_module_position_in_list("c-module", test_reg_list, 0);

    cdada_list_t *e_module_dep_list = get_dependency_list(test_module_set, "e-module");
    assert_int_equal(cdada_list_size(e_module_dep_list), 0);
    cdada_list_t *cc_module_dep_list = get_dependency_list(test_module_set, "cc-module");
    assert_int_equal(cdada_list_size(cc_module_dep_list), 0);
    
    cdada_list_t *c_module_dep_list = get_dependency_list(test_module_set, "c-module");
    assert_int_equal(cdada_list_size(c_module_dep_list), 2);
    assert_module_position_in_list("e-module", c_module_dep_list, 0);
    assert_module_position_in_list("cc-module", c_module_dep_list, 1);

    //Test2: A non-valid test case. Call find_include for b-module which does not have include
    assert_int_equal(libyangpush_find_submodule_direct_dep(NULL, NULL, NULL), INVALID_PARAMETER);

    cdada_map_traverse(test_module_set, &libyangpush_trav_clear_module_set_map, NULL);
    cdada_map_destroy(test_module_set);
    cdada_list_destroy(test_reg_list);
    cdada_list_destroy(test_dep_list);
    ly_ctx_destroy(test_ctx);
    free(test_cmodule_text);
    free(test_ccmodule_text);
    free(test_emodule_text);
}

static void test_find_module_reverse_dep(void** state)
{
    (void) state;
    struct ly_ctx *test_ctx;
    if(ly_ctx_new("../test/resources/find_reverse_dependency", 1, &test_ctx) != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    //load the text of yang module
    char *test_amodule_text = load_yang_example("../test/resources/find_reverse_dependency/a-module.yang");
    char *test_dmodule_text = load_yang_example("../test/resources/find_reverse_dependency/d-module.yang");
    char *test_cmodule_text = load_yang_example("../test/resources/find_reverse_dependency/c-module.yang");
    char *test_emodule_text = load_yang_example("../test/resources/find_reverse_dependency/e-module.yang");

    char *test_deviate_text = load_yang_example("../test/resources/find_reverse_dependency/a-module-deviations.yang");
    char *test_strmodule_text = load_yang_example("../test/resources/find_reverse_dependency/str-type.yang");
    char *test_deviate_include_text = load_yang_example("../test/resources/find_reverse_dependency/a-deviate-include.yang");

    //load a-module and its augment into context
    struct lys_module* test_amodule = ly_ctx_load_module(test_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "d-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "e-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "str-type", NULL, NULL);
    ly_ctx_load_module(test_ctx, "a-module-deviations", NULL, NULL);

    //create cdada map and register list
    cdada_map_t* test_module_set = cdada_map_create(unsigned long);
    cdada_list_t* test_reg_list = cdada_list_create(unsigned long);

    unsigned long hash = djb2("a-module");
    struct module_info *test_amodule_info_ptr = libyangpush_load_module_into_map(test_module_set, test_amodule, hash);
    test_amodule_info_ptr->dependency_list = cdada_list_create(unsigned long);
    assert_non_null(test_amodule_info_ptr);
    assert_int_equal(cdada_list_push_front(test_reg_list, &hash), CDADA_SUCCESS);

    //Test1: A valid test case for finding augment. Find the augment for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_module_reverse_dep(test_amodule, test_module_set, test_reg_list), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test_module_set), 8);
    assert_int_equal(cdada_list_size(test_reg_list), 8);
    assert_found_module_in_map("a-module", test_amodule_text, test_module_set, djb2("a-module"));
    assert_module_position_in_list("a-module", test_reg_list, 7);
    assert_found_module_in_map("e-module", test_emodule_text, test_module_set, djb2("e-module"));
    assert_module_position_in_list("e-module", test_reg_list, 6);
    assert_found_module_in_map("c-module", test_cmodule_text, test_module_set, djb2("c-module"));
    assert_module_position_in_list("c-module", test_reg_list, 5);
    assert_found_module_in_map("d-module", test_dmodule_text, test_module_set, djb2("d-module"));
    assert_module_position_in_list("d-module", test_reg_list, 4);
    assert_found_module_in_map("str-type", test_strmodule_text, test_module_set, djb2("str-type"));
    assert_module_position_in_list("str-type", test_reg_list, 3);
    assert_found_module_in_map("a-deviate-include", test_deviate_include_text, test_module_set, djb2("a-deviate-include"));
    assert_module_position_in_list("a-deviate-include", test_reg_list, 2);
    assert_found_module_in_map("a-module-deviations", test_deviate_text, test_module_set, djb2("a-module-deviations"));
    assert_module_position_in_list("a-module-deviations", test_reg_list, 1);
    assert_found_module_in_map("a-module", test_amodule_text, test_module_set, djb2("a-module-full-dependency"));
    assert_module_position_in_list("a-module-full-dependency", test_reg_list, 0);
    //asser dependency_list
    cdada_list_t *a_module_dep_list = get_dependency_list(test_module_set, "a-module");
    assert_int_equal(cdada_list_size(a_module_dep_list), 0);
    cdada_list_t *e_module_dep_list = get_dependency_list(test_module_set, "e-module");
    assert_int_equal(cdada_list_size(e_module_dep_list), 0);
    cdada_list_t *c_module_dep_list = get_dependency_list(test_module_set, "c-module");
    assert_int_equal(cdada_list_size(c_module_dep_list), 0);
    cdada_list_t *str_type_dep_list = get_dependency_list(test_module_set, "str-type");
    assert_int_equal(cdada_list_size(str_type_dep_list), 0);
    cdada_list_t *a_deviate_include_dep_list = get_dependency_list(test_module_set, "a-deviate-include");
    assert_int_equal(cdada_list_size(a_deviate_include_dep_list), 0);

    cdada_list_t *d_module_dep_list = get_dependency_list(test_module_set, "d-module");
    assert_int_equal(cdada_list_size(d_module_dep_list), 3);
    assert_module_position_in_list("a-module", d_module_dep_list, 0);
    assert_module_position_in_list("e-module", d_module_dep_list, 1);
    assert_module_position_in_list("c-module", d_module_dep_list, 2);

    cdada_list_t *a_deviate_dep_list = get_dependency_list(test_module_set, "a-module-deviations");
    assert_int_equal(cdada_list_size(d_module_dep_list), 3);
    assert_module_position_in_list("a-module", a_deviate_dep_list, 0);
    assert_module_position_in_list("str-type", a_deviate_dep_list, 1);
    assert_module_position_in_list("a-deviate-include", a_deviate_dep_list, 2);

    cdada_list_t *a_module_full_dep_list = get_dependency_list(test_module_set, "a-module-full-dependency");
    assert_int_equal(cdada_list_size(a_module_full_dep_list), 2);
    assert_module_position_in_list("d-module", a_module_full_dep_list, 1);
    assert_module_position_in_list("a-module-deviations", a_module_full_dep_list, 0);

    cdada_map_traverse(test_module_set, &libyangpush_trav_clear_module_set_map, NULL);
    cdada_map_destroy(test_module_set);
    cdada_list_destroy(test_reg_list);
    ly_ctx_destroy(test_ctx);
    free(test_amodule_text);
    free(test_dmodule_text);
    free(test_cmodule_text);
    free(test_emodule_text);
    free(test_deviate_text);
    free(test_strmodule_text);
    free(test_deviate_include_text);
}


static void test_find_all_dependency(void** state)
{
    (void) state;
    struct ly_ctx *test_ctx;
    if(ly_ctx_new("../test/resources/recursive_find_dependency/", 1, &test_ctx) != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    //load the text of yang module
    char *test_amodule_text = load_yang_example("../test/resources/recursive_find_dependency/a-module.yang");
    char *test_bmodule_text = load_yang_example("../test/resources/recursive_find_dependency/b-module.yang");
    char *test_cmodule_text = load_yang_example("../test/resources/recursive_find_dependency/c-module.yang");
    char *test_dmodule_text = load_yang_example("../test/resources/recursive_find_dependency/d-module.yang");
    char *test_emodule_text = load_yang_example("../test/resources/recursive_find_dependency/e-module.yang");

    char *test_deviate_text = load_yang_example("../test/resources/recursive_find_dependency/a-module-deviations.yang");
    char *test_strmodule_text = load_yang_example("../test/resources/recursive_find_dependency/str-type.yang");
    char *test_deviate_include_text = load_yang_example("../test/resources/recursive_find_dependency/a-deviate-include.yang");

    //load a-module and its augment into context
    struct lys_module* test_amodule = ly_ctx_load_module(test_ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "b-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "d-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "e-module", NULL, NULL);
    ly_ctx_load_module(test_ctx, "str-type", NULL, NULL);
    ly_ctx_load_module(test_ctx, "a-module-deviations", NULL, NULL);

    //create cdada map and register list
    cdada_map_t* test_module_set = cdada_map_create(unsigned long);
    cdada_list_t* test_reg_list = cdada_list_create(unsigned long);

    //Test1: A valid test case for finding augment. Find the augment for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_all_dependency(test_amodule, test_module_set, test_reg_list), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(test_module_set), 9);
    assert_int_equal(cdada_list_size(test_reg_list), 9);
    assert_found_module_in_map("e-module", test_emodule_text, test_module_set, djb2("e-module"));
    assert_module_position_in_list("e-module", test_reg_list, 8);
    assert_found_module_in_map("b-module", test_bmodule_text, test_module_set, djb2("b-module"));
    assert_module_position_in_list("b-module", test_reg_list, 7);
    assert_found_module_in_map("c-module", test_cmodule_text, test_module_set, djb2("c-module"));
    assert_module_position_in_list("c-module", test_reg_list, 6);
    assert_found_module_in_map("a-module", test_amodule_text, test_module_set, djb2("a-module"));
    assert_module_position_in_list("a-module", test_reg_list, 5);
    assert_found_module_in_map("d-module", test_dmodule_text, test_module_set, djb2("d-module"));
    assert_module_position_in_list("d-module", test_reg_list, 4);
    assert_found_module_in_map("str-type", test_strmodule_text, test_module_set, djb2("str-type"));
    assert_module_position_in_list("str-type", test_reg_list, 3);
    assert_found_module_in_map("a-deviate-include", test_deviate_include_text, test_module_set, djb2("a-deviate-include"));
    assert_module_position_in_list("a-deviate-include", test_reg_list, 2);
    assert_found_module_in_map("a-module-deviations", test_deviate_text, test_module_set, djb2("a-module-deviations"));
    assert_module_position_in_list("a-module-deviations", test_reg_list, 1);
    //assert dependency_list
    cdada_list_t *e_module_dep_list = get_dependency_list(test_module_set, "e-module");
    assert_int_equal(cdada_list_size(e_module_dep_list), 0);
    cdada_list_t *str_type_dep_list = get_dependency_list(test_module_set, "str-type");
    assert_int_equal(cdada_list_size(str_type_dep_list), 0);
    cdada_list_t *a_deviate_include_dep_list = get_dependency_list(test_module_set, "a-deviate-include");
    assert_int_equal(cdada_list_size(a_deviate_include_dep_list), 0);

    cdada_list_t *c_module_dep_list = get_dependency_list(test_module_set, "c-module");
    assert_int_equal(cdada_list_size(c_module_dep_list), 1);
    assert_module_position_in_list("e-module", c_module_dep_list, 0);

    cdada_list_t *b_module_dep_list = get_dependency_list(test_module_set, "b-module");
    assert_int_equal(cdada_list_size(b_module_dep_list), 1);
    assert_module_position_in_list("e-module", b_module_dep_list, 0);

    cdada_list_t *a_module_dep_list = get_dependency_list(test_module_set, "a-module");
    assert_int_equal(cdada_list_size(a_module_dep_list), 2);
    assert_module_position_in_list("c-module", a_module_dep_list, 1);
    assert_module_position_in_list("b-module", a_module_dep_list, 0);

    cdada_list_t *d_module_dep_list = get_dependency_list(test_module_set, "d-module");
    assert_int_equal(cdada_list_size(d_module_dep_list), 2);
    assert_module_position_in_list("a-module", d_module_dep_list, 0);
    assert_module_position_in_list("b-module", d_module_dep_list, 1);

    cdada_list_t *a_deviate_dep_list = get_dependency_list(test_module_set, "a-module-deviations");
    assert_int_equal(cdada_list_size(a_deviate_dep_list), 3);
    assert_module_position_in_list("a-module", a_deviate_dep_list, 0);
    assert_module_position_in_list("str-type", a_deviate_dep_list, 1);
    assert_module_position_in_list("a-deviate-include", a_deviate_dep_list, 2);

    cdada_list_t *a_module_full_dep_list = get_dependency_list(test_module_set, "a-module-full-dependency");
    assert_int_equal(cdada_list_size(a_module_full_dep_list), 4);
    assert_module_position_in_list("b-module", a_module_full_dep_list, 3);
    assert_module_position_in_list("c-module", a_module_full_dep_list, 2);
    assert_module_position_in_list("d-module", a_module_full_dep_list, 1);
    assert_module_position_in_list("a-module-deviations", a_module_full_dep_list, 0);
    
    cdada_map_traverse(test_module_set, &libyangpush_trav_clear_module_set_map, NULL);
    cdada_map_destroy(test_module_set);
    cdada_list_destroy(test_reg_list);
    ly_ctx_destroy(test_ctx);
    free(test_amodule_text);
    free(test_bmodule_text);
    free(test_dmodule_text);
    free(test_cmodule_text);
    free(test_emodule_text);
    free(test_deviate_text);
    free(test_strmodule_text);
    free(test_deviate_include_text);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_load_module_to_map),
        cmocka_unit_test(test_find_module_direct_dep),
        cmocka_unit_test(test_find_submodule_direct_dep),
        cmocka_unit_test(test_find_module_reverse_dep),
        cmocka_unit_test(test_find_all_dependency)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}