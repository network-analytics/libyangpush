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

static void test_find_import(void** state)
{
    struct ly_ctx* ctx;
    char *bmodule;
    char search_dir[50] = "../modules/";
    int return_check = ly_ctx_new(search_dir, 1, &ctx);
    if(return_check != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

    // read b-module yang code from file
    FILE *fptr = fopen("../resources/b-module.yang", "r");
    fseek(fptr, 0, SEEK_END);
    int flen = ftell(fptr); //file length
    fseek(fptr, 0, SEEK_SET); 

    bmodule = (char*)calloc((flen+1), sizeof(char));
    fread(bmodule, flen, 1, fptr); 
    fclose(fptr);

    //load a-module and its import into context
    ly_ctx_load_module(ctx, "a-module", NULL, NULL);
    ly_ctx_load_module(ctx, "b-module", NULL, NULL);
    struct lys_module* example_module1 = ly_ctx_get_module_implemented(ctx, "a-module");
    struct lysp_import* example_module1_imports = example_module1->parsed->imports;
    int num_of_imports1 = LY_ARRAY_COUNT(example_module1_imports);

    struct lys_module* example_module2 = ly_ctx_get_module_implemented(ctx, "b-module");
    struct lysp_import* example_module2_imports = example_module2->parsed->imports;
    int num_of_imports2 = LY_ARRAY_COUNT(example_module2_imports);
    
    cdada_map_t* module_set = cdada_map_create(256);
    int hash_of_bmodule = djb2("b-module");
    struct module_info *module_ptr;

    //Test1: A valid test case. Find the import for a-module and check if it has been put into the cdada map
    assert_int_equal(libyangpush_find_import(num_of_imports1, example_module1_imports, module_set), FIND_DEPENDENCY_SUCCESS);
    assert_int_equal(cdada_map_size(module_set), 1);
    assert_int_equal(cdada_map_find(module_set, &hash_of_bmodule, (void**)&module_ptr), CDADA_SUCCESS);
    assert_string_equal(module_ptr->yang_code, bmodule);
    assert_string_equal(module_ptr->name, "b-module");

    //Test2: A non-valid test case. Call find_import for b-module which does not have import
    assert_int_equal(libyangpush_find_import(num_of_imports2, example_module2_imports, module_set), INVALID_PARAMETER);

    free(module_ptr->yang_code);
    free(module_ptr);
    cdada_map_destroy(module_set);
    cdada_map_clear(module_set);
    ly_ctx_destroy(ctx);
    free(bmodule);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_find_import),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}