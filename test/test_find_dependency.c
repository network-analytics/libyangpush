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

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_find_import),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}