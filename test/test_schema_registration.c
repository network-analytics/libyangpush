#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <cdada/list.h>
#include <cdada/map.h>
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

static void test_schema_registration(void** state)
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
    assert_int_equal(libyangpush_find_all_dependency(test_amodule, test_module_set, test_reg_list), FIND_DEPENDENCY_SUCCESS);

    struct cdadamap_n_schemaid *user_data = malloc(sizeof(cdada_map_t*)+sizeof(int));
    user_data->module_set = test_module_set;
    user_data->schema_id = 0;
    cdada_list_rtraverse(test_reg_list, libyangpush_trav_list_register_schema, user_data);

    cdada_map_traverse(test_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test_module_set);
    cdada_list_destroy(test_reg_list);
    ly_ctx_destroy(test_ctx);
    free(user_data);
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
        cmocka_unit_test(test_schema_registration)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}