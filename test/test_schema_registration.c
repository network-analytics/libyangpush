#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <cdada/list.h>
#include <cdada/map.h>
#include <jansson.h>
#include "libyangpush.h"
#include "tool.h"

/* A helper function for loading file from disk */
json_t* load_json_from_file(char *filename) 
{
    json_error_t error;
    json_t *json;
    char* text;

    FILE *fptr = fopen(filename, "r");
    fseek(fptr, 0, SEEK_END);
    int flen = ftell(fptr); //file length
    fseek(fptr, 0, SEEK_SET); 

    text = (char*)calloc((flen+1), sizeof(char));
    fread(text, flen, 1, fptr); 
    fclose(fptr);

    json = json_loads(text, 0, &error);
    free(text);
    return json;
}

void assert_schema(char *filepath, unsigned long hash, cdada_map_t *module_set){
    struct module_info *module_ptr;

    json_t *module_schema = load_json_from_file(filepath);

    if(cdada_map_find((cdada_map_t*)module_set, &hash, (void**)&module_ptr) != CDADA_SUCCESS) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_trav_list_register_schema]Fail when finding in map\n");
#endif
        return;
    }
    json_t *references = libyangpush_create_reference(module_ptr, module_set, "1");
    json_t *schema = libyangpush_create_schema(module_ptr, references);
    assert_int_equal(json_equal(schema, module_schema), 1);

    json_decref(references);
    json_decref(schema);
    json_decref(module_schema);
}

static void test_schema_registration(void** state)
{
    (void) state;
    struct ly_ctx *test_ctx;
    if(ly_ctx_new("../test/resources/recursive_find_dependency/", 1, &test_ctx) != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }

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

    assert_schema("../test/resources/schemas/subject_a-deviate-include_6183646357426501734.json", djb2("a-deviate-include"), test_module_set);
    assert_schema("../test/resources/schemas/subject_a-module_7572081280902041.json", djb2("a-module"), test_module_set);
    assert_schema("../test/resources/schemas/subject_a-module_18315196874911194949.json", djb2("a-module-full-dependency"), test_module_set);
    assert_schema("../test/resources/schemas/subject_b-module_7572123899345018.json", djb2("b-module"), test_module_set);
    assert_schema("../test/resources/schemas/subject_c-module_7572166517787995.json", djb2("c-module"), test_module_set);
    assert_schema("../test/resources/schemas/subject_d-module_7572209136230972.json", djb2("d-module"), test_module_set);
    assert_schema("../test/resources/schemas/subject_e-module_7572251754673949.json", djb2("e-module"), test_module_set);
    assert_schema("../test/resources/schemas/subject_str-type_7572940225087085.json", djb2("str-type"), test_module_set);

    cdada_map_traverse(test_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test_module_set);
    cdada_list_destroy(test_reg_list);
    ly_ctx_destroy(test_ctx);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_schema_registration)
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}