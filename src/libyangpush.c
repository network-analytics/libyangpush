/* libyangpush functionality */
#include "libyangpush.h"
#include "tool.h"

json_t* libyangpush_create_reference(struct module_info *module_ptr, cdada_map_t *module_set, char *version, char *subject_prefix)
{
    unsigned long hash = djb2(module_ptr->name);
    if(module_ptr == NULL || module_set == NULL) {
        return NULL;
    }
    json_t *references = json_array();

    //for each module in the dependency list, create a reference
    for(int i = 0; i < (int)cdada_list_size(module_ptr->dependency_list); i++) {
        char subject[256];
        unsigned long dependency_hash;
        struct module_info *dependency_module_ptr = NULL;
        cdada_list_get(module_ptr->dependency_list, i, &dependency_hash);
        if(dependency_hash == hash) { //If the parent happends to be in the dep_list, continue
            continue;
        }
        cdada_map_find(module_set, &dependency_hash, (void**)&dependency_module_ptr);
        sprintf(subject, "%s%s_%lu", subject_prefix, dependency_module_ptr->name, dependency_hash);
        json_t *reference = json_pack("{s:s,s:s,s:s}", //schematype, reference, schema
                                "subject",   subject,
                                "name",      dependency_module_ptr->name,
                                "version",   version);
        json_array_append(references, reference);
        json_decref(reference);
    }
    return references;
}

json_t* libyangpush_create_schema(struct module_info *module_ptr, json_t *references)
{
    json_t *schema = json_pack("{s:s,s:o*,s:s}", //schematype, reference, schema
                               "schemaType",    "YANG",
                               "references",    references,
                               "schema",    module_ptr->yang_code);
    return schema;
}

void libyangpush_trav_list_register_schema(const cdada_list_t* reg_list, const void* key, void* schema_info)
{
    (void) reg_list;
    cdada_map_t *module_set = ((struct schema_info*)schema_info)->module_set;
    unsigned long hash = *(unsigned long*)key;

    struct module_info *module_ptr;
    if(cdada_map_find((cdada_map_t*)module_set, &hash, (void**)&module_ptr) != CDADA_SUCCESS) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_trav_list_register_schema]Fail when finding in map\n");
#endif
        return;
    }
    char subject[256];
    sprintf(subject, "%s%s_%lu", ((struct schema_info*)schema_info)->schema_subject_prefix, module_ptr->name, hash);

    json_t *references = libyangpush_create_reference(module_ptr, module_set, ((struct schema_info*)schema_info)->version, ((struct schema_info*)schema_info)->schema_subject_prefix);
    json_t *schema = libyangpush_create_schema(module_ptr, references);
    ((struct schema_info*)schema_info)->schema_id = register_schema(((struct schema_info*)schema_info)->schema_registry_address, schema, subject);
#if check_schema
    char file_path[512];
    sprintf(file_path, "../resources/schemas/%s.json", subject);
    printf("json dump %d\n", json_dump_file(schema, file_path, JSON_ENSURE_ASCII));

    char* json_schema = json_dumps(schema, JSON_ENSURE_ASCII);
    printf("%s\n\n", json_schema);
    free(json_schema);
#endif
    json_decref(references);
    json_decref(schema);
    return;
}

void libyangpush_trav_clear_module_set_map(const cdada_map_t* traversed_map, const void* key, void* val, void* user_define_data)
{
    (void) key;
    (void) user_define_data;
    (void) traversed_map;
    struct module_info *value = val;
    free(value->yang_code);
    free(value->name);
    cdada_list_destroy(value->dependency_list);
    free(value);
    return;
}

void libyangpush_trav_clear_subscription_filter_map(const cdada_map_t* traversed_map, const void* key, void* val, void* user_define_data)
{
    (void) key;
    (void) user_define_data;
    (void) traversed_map;
    struct subscription_filter_info *value = val;
    if(value->module_num != 0) {
        for(int i = 0; i < value->module_num; i++) {
            free(value->filter[i]);
        }
        free(value->filter);
    }
    free(value);
}

struct module_info* libyangpush_load_module_into_map(cdada_map_t *map, struct lys_module* module, unsigned long hash_index)
{
    if(map == NULL || module == NULL) {
        return NULL;
    }

    struct module_info* yang_module_info_ptr = NULL;
    yang_module_info_ptr = (struct module_info*)malloc(sizeof(struct module_info));

    //write the module content to module_info->yang_code
    lys_print_mem(&(yang_module_info_ptr->yang_code), module, LYS_OUT_YANG, 0);

    //write the module name to module_info->name
    yang_module_info_ptr->name = calloc((strlen(module->name)+1), sizeof(char));
    strncpy(yang_module_info_ptr->name, (char*)(module->name), strlen(module->name));
    if(cdada_map_insert(map, &hash_index, yang_module_info_ptr) != CDADA_SUCCESS) { //inserted the module_info struct
#if debug
        fprintf(stderr, "%s%s\n", "[libyangpush_load_module_into_map]Fail when inserting ", module->name);
#endif
        return NULL;
    }
    return yang_module_info_ptr;
}

struct module_info* libyangpush_load_submodule_into_map(cdada_map_t *map, struct lysp_submodule* module, unsigned long hash_index)
{
    if(map == NULL || module == NULL) {
        return NULL;
    }

    struct module_info* yang_module_info_ptr = NULL;
    yang_module_info_ptr =  (struct module_info*)malloc(sizeof(struct module_info)); //fill in the module_info struct

    //write the module content to module_info->yang_code
    struct ly_out* out = NULL;
    ly_out_new_memory(&(yang_module_info_ptr->yang_code), 0, &out);//store submodule content in char
    lys_print_submodule(out, module, LYS_OUT_YANG, 0, LY_PRINT_SHRINK);
    ly_out_free(out, 0, 0);

    //write the module name to module_info->name
    yang_module_info_ptr->name = calloc((strlen(module->name)+1), sizeof(char));
    strncpy(yang_module_info_ptr->name, (char*)(module->name), strlen(module->name));

    if(cdada_map_insert(map, &hash_index, yang_module_info_ptr) != CDADA_SUCCESS) { //inserted the module_info struct
#if debug
        fprintf(stderr, "%s%s", "[libyangpush_load_module_into_map]Fail when inserting \n", module->name);
#endif
        return NULL;
    }
    return yang_module_info_ptr;
}

char* libyangpush_pattern_match(char* pattern, char* string)
{
    int has_pattern_matched = 0;
    const size_t maxMatches = 1;
    regex_t regex_pattern;
    regmatch_t matched_string_offset[maxMatches];
    char* result = NULL;

    if(pattern == NULL || string == NULL) {
    #if debug
        fprintf(stderr, "[libyangpush_pattern_match]Invalid arguments");
    #endif
        goto cleanup;
    }

    has_pattern_matched = regcomp(&regex_pattern, pattern, REG_EXTENDED); //Compile the pattern, return 0 if successfuel, vice versa

    if (has_pattern_matched) {
#if debug
        fprintf(stderr, "[libyangpush_pattern_match][libyangpush_pattern_match]pattern compilation filed");
#endif
        goto cleanup;
    }
    has_pattern_matched = regexec(&regex_pattern, string, maxMatches, matched_string_offset, REG_EXTENDED);
    if (has_pattern_matched == REG_NOERROR) { //find match
        result = (char*)calloc((matched_string_offset[0].rm_eo - matched_string_offset[0].rm_so + 1), sizeof(char));
        strncpy(result, &(string)[matched_string_offset[0].rm_so], matched_string_offset[0].rm_eo - matched_string_offset[0].rm_so);
#if debug
        printf("[libyangpush_pattern_match]Pattern found. result = %s\n", result);
#endif
    }
    // If pattern not found
    else if (has_pattern_matched == REG_NOMATCH) {
#if debug
        fprintf(stderr, "[libyangpush_pattern_match]Pattern not found.\n");
#endif
        goto cleanup;
    }

cleanup:
    regfree(&regex_pattern);
    return result;
}

char* libyangpush_find_namespace_for_prefix(xmlNs** namespaces, char *xpath_prefix)
{
    if(namespaces == NULL || xpath_prefix == NULL) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_find_namespace_for_prefix]Invalid arguments\n");
#endif
        return NULL;
    }
    /*check if this namespace has no prefix, or if the prefix not equal to xpath_prefix*/
    while ((*namespaces)->prefix == NULL || strcmp(xpath_prefix, (char*)(*namespaces)->prefix) != 0) {
        *namespaces = (*namespaces)->next;
        /*Looping until the end of namespace, still no prefix is equals to xpath_prefix*/
        if ((*namespaces) == NULL) {
            return NULL;
        }
    }
    /*namespace found, return its URL*/
    return (char*)(*namespaces)->href;
}

xpath_parsing_err_code_t libyangpush_parse_xpath(xmlNodePtr datastore_xpath, char** result)
{
    int parsing_result = 0;
    if (datastore_xpath == NULL || strcmp((char*)datastore_xpath->name, "datastore-xpath-filter")) { //if this is not the datastore-xpath-filter field, parsed fail
        return parsing_result;
    }
    char *raw_prefix, *prefix = NULL;
    char *xpath = (char*)xmlNodeGetContent(datastore_xpath);
    raw_prefix = libyangpush_pattern_match("/[^/:]*?:", xpath);
    free(xpath);
    if (raw_prefix == NULL) { //prefix not found or it is not valid prefix
#if debug
        printf("[libyangpush_parse_xpath]no prefix found in xpath\n");
#endif
        parsing_result = XPATH_PARSED_FAILED;
        return parsing_result;
    }
    else if (strcmp(raw_prefix, "/:") == 0) {
#if debug
        printf("[libyangpush_parse_xpath]prefix invalid in xpath\n");
#endif
        parsing_result = XPATH_PARSED_FAILED;
        free(raw_prefix);
        return parsing_result;
    }
    else { //valid prefix found
        prefix = (char*)calloc((strlen(raw_prefix) - 1), sizeof(char));
        strncpy(prefix, raw_prefix+1, strlen(raw_prefix)-2); //remove the '/' and ':' at the end and beginning of pattern match result

        xmlNs* namespaces = datastore_xpath->nsDef;

        *result = libyangpush_find_namespace_for_prefix(&namespaces, prefix);
        if((*result) == NULL) {//Namespace not found
            *result = prefix;
            parsing_result = XPATH_MODULE_NAME_FOUND;
            free(raw_prefix);
            return parsing_result;
        }
        //Namespace found
        else {
            *result = (char*)calloc((strlen((char*)namespaces->href)+1), sizeof(char));
            strncpy(*result, (char*)namespaces->href, strlen((char*)namespaces->href));
            parsing_result = XPATH_NAMESPACE_FOUND;
            free(prefix);
            free(raw_prefix);
        }
    }
    
    return parsing_result;
}

size_t libyangpush_parse_subtree(xmlNodePtr datastore_subtree, char ***result)
{
    int child_index = 0;

    if (datastore_subtree == NULL || strcmp((char*)datastore_subtree->name, "datastore-subtree-filter")) { //if this is not the datastore-xpath-filter field, parsed fail
        return 0;
    }
    xmlNodePtr subtree = datastore_subtree->children; //get the first subtree node
    if(subtree == NULL) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_parse_subtree]Invalid subtree\n");
#endif
        return 0;
    }

    if(xmlChildElementCount(datastore_subtree)) { //Check num of child. Allocate memory for result pointer
        *result = (char**)malloc(xmlChildElementCount(datastore_subtree)*sizeof(char*));
    }
    
    xmlNs *subtree_namespace;
    while(subtree != NULL) {
        subtree_namespace = subtree->ns;
        if(subtree_namespace == NULL) {
#if debug
            fprintf(stderr, "%s", "[libyangpush_parse_subtree]Invalid subtree\n");
#endif
            for(int i = 0; i < child_index; i++) { //free the previously allocated space
                free((*result)[i]);
            }
            free(*result);
            return 0;
        }
        else if(subtree->ns->href != NULL) {
            (*result)[child_index] = (char*)calloc((strlen((char*)subtree->ns->href)+1), sizeof(char));
            strncpy((*result)[child_index], (char*)subtree->ns->href, strlen((char*)subtree->ns->href));
            (child_index)++;
        }
        subtree = subtree->next;
    }
    return child_index;
}

find_dependency_err_code_t libyangpush_load_module_direct_dependency_into_map_and_list(cdada_map_t *module_set, cdada_list_t *register_list, cdada_list_t *dependency_list, struct lys_module *module)
{
    unsigned long hash = djb2((char*)module->name);
    struct module_info *module_info_ptr = NULL;
    if(cdada_list_push_front(register_list, &hash) != CDADA_SUCCESS) { //put into register list
        return INSERT_FAIL;
    }
    if (cdada_map_find(module_set, &hash, (void**)&module_info_ptr) == CDADA_E_NOT_FOUND) { //module is not cached
        module_info_ptr = libyangpush_load_module_into_map(module_set, module, hash);
        if(module_info_ptr != NULL) {
            module_info_ptr->dependency_list = dependency_list;
            return FIND_DEPENDENCY_SUCCESS;
        }
        else { //load module into map fail
            return INSERT_FAIL;
        }
    }
    else if (cdada_map_find(module_set, &hash, (void**)&module_info_ptr) == CDADA_SUCCESS) { //module is cached
        if(module_info_ptr != NULL) {
            module_info_ptr->dependency_list = dependency_list;
            return FIND_DEPENDENCY_SUCCESS;
        }
        else {
            return INSERT_FAIL;
        }
    }
    else { // map is corrupted
        return INSERT_FAIL;
    }
}

find_dependency_err_code_t libyangpush_load_submodule_direct_dependency_into_map_and_list(cdada_map_t *module_set, cdada_list_t *register_list, cdada_list_t *dependency_list, struct lysp_submodule *module)
{
    unsigned long hash = djb2((char*)module->name);
    struct module_info * module_info_ptr = NULL;
    if(cdada_list_push_front(register_list, &hash) != CDADA_SUCCESS) { //put into register list
        return INSERT_FAIL;
    }
    if(cdada_map_find(module_set, &hash, (void**)&module_info_ptr) == CDADA_E_NOT_FOUND) { //module is not cached
        module_info_ptr = libyangpush_load_submodule_into_map(module_set, module, hash);
        if(module_info_ptr != NULL) {
            module_info_ptr->dependency_list = dependency_list;
            return FIND_DEPENDENCY_SUCCESS;
        }
        else {
            return INSERT_FAIL;
        }
    }
    else if (cdada_map_find(module_set, &hash, (void**)&module_info_ptr) == CDADA_SUCCESS) {
        if(module_info_ptr != NULL) {
            module_info_ptr->dependency_list = dependency_list;
            return FIND_DEPENDENCY_SUCCESS;
        }
        else {
            return INSERT_FAIL;
        }
    }
    else { //module_set map is corrupted
        return INVALID_PARAMETER;
    }
}

find_dependency_err_code_t libyangpush_load_module_reverse_dependency_into_map_and_list(cdada_map_t *module_set, cdada_list_t *register_list, cdada_list_t *dependency_list, struct lys_module *module)
{
    unsigned long hash = djb2((char*)module->name);
    char module_new_name[100];
    struct module_info * module_info_ptr = NULL;
    
    if (cdada_map_find(module_set, &hash, (void**)&module_info_ptr) == CDADA_SUCCESS) { //module is cached
        sprintf(module_new_name, "%s-full-dependency", module->name);
        hash = djb2(module_new_name);
        struct module_info *new_module_info_ptr = libyangpush_load_module_into_map(module_set, module, hash);
        new_module_info_ptr->dependency_list = cdada_list_create(unsigned long); 
        cdada_list_traverse(module_info_ptr->dependency_list, trav_copy_list, new_module_info_ptr->dependency_list);

        if(cdada_list_push_front(register_list, &hash) != CDADA_SUCCESS) { //put into register list
            return INSERT_FAIL;
        }
        if(module_info_ptr != NULL) {
            cdada_list_traverse(dependency_list, trav_copy_list, new_module_info_ptr->dependency_list);
            cdada_map_insert(module_set, &hash, new_module_info_ptr);
            return FIND_DEPENDENCY_SUCCESS;
        }
        else {
            return INSERT_FAIL;
        }
    }
    else { //module_set map is corrupted or module is not cached(main module has be in the map for reverse dependency)
        return INVALID_PARAMETER;
    }
}

find_dependency_err_code_t libyangpush_find_module_direct_dep(struct lys_module *module, cdada_map_t *module_set, cdada_list_t *reg_list)
{
    if(module == NULL || module_set == NULL || reg_list == NULL) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_find_module_direct_dep]parameter imported_module is NULL\n");
#endif
        return INVALID_PARAMETER;
    }
    int find_dependency_result = FIND_DEPENDENCY_SUCCESS;
    unsigned long hash;
    struct lysp_import *imported_module = module->parsed->imports;
    struct lysp_include *included_module = module->parsed->includes; 
    cdada_list_t *dependency_list = cdada_list_create(unsigned long);

    /* For imported module */
    for(int i = 0; i < (int)LY_ARRAY_COUNT(imported_module); i++) {
        void *module_info;
        imported_module+=i;
        hash = djb2((char*)imported_module->name); //hash the module name
        if(cdada_list_push_back(dependency_list, &hash) != CDADA_SUCCESS) { //put into dependency list
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
        if (cdada_map_find(module_set, &hash, &module_info) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        else if (cdada_map_find(module_set, &hash, &module_info) != CDADA_E_NOT_FOUND) { //module_set map is corrupted
            find_dependency_result = INVALID_PARAMETER;
            goto cleanup;
        }
        if(libyangpush_find_module_direct_dep(imported_module->module, module_set, reg_list) == INSERT_FAIL) { //recursive find dependency
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
    }
    /* For included module */
    for(int i = 0; i < (int)LY_ARRAY_COUNT(included_module); i++) {
        void *module_info;
        included_module+=i;
        hash = djb2((char*)included_module->name); //hash the module name
        if(cdada_list_push_back(dependency_list, &hash) != CDADA_SUCCESS) { //put into dependency list
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
        if (cdada_map_find(module_set, &hash, &module_info) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        else if (cdada_map_find(module_set, &hash, &module_info) != CDADA_E_NOT_FOUND) { //module_set map is corrupted
            find_dependency_result = INVALID_PARAMETER;
            goto cleanup;
        }
        if(libyangpush_find_submodule_direct_dep(included_module->submodule, module_set, reg_list) == INSERT_FAIL) { //recursive find dependency
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
    }

    find_dependency_result = libyangpush_load_module_direct_dependency_into_map_and_list(module_set, reg_list, dependency_list, module);
    if(find_dependency_result == FIND_DEPENDENCY_SUCCESS) {
        return find_dependency_result;
    }

cleanup:
    cdada_list_destroy(dependency_list);
    return find_dependency_result;
}

find_dependency_err_code_t libyangpush_find_submodule_direct_dep(struct lysp_submodule *module, cdada_map_t *module_set, cdada_list_t *reg_list)
{
    if(module == NULL || module_set == NULL || reg_list == NULL) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_find_submodule_direct_dep]Invalid input parameter\n");
#endif
        return INVALID_PARAMETER;
    }
    int find_dependency_result = FIND_DEPENDENCY_SUCCESS;
    unsigned long hash;
    struct lysp_include *included_module = module->includes;
    struct lysp_import *imported_module = module->imports;
    cdada_list_t *dependency_list = cdada_list_create(unsigned long);

    /* For imported module */
    for(int i = 0; i < (int)LY_ARRAY_COUNT(imported_module); i++) {
        void *module_info;
        imported_module+=i;
        hash = djb2((char*)imported_module->name); //hash the module name
        if(cdada_list_push_back(dependency_list, &hash) != CDADA_SUCCESS) { //put into dependency list
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
        if (cdada_map_find(module_set, &hash, &module_info) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        else if (cdada_map_find(module_set, &hash, &module_info) != CDADA_E_NOT_FOUND) { //module_set map is corrupted
            find_dependency_result = INVALID_PARAMETER;
            goto cleanup;
        }
        if(libyangpush_find_module_direct_dep(imported_module->module, module_set, reg_list) == INSERT_FAIL) {
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
    }
    /* For included module */
    for(int i = 0; i < (int)LY_ARRAY_COUNT(included_module); i++) {
        void *module_info;
        included_module+=i;
        hash = djb2((char*)included_module->name); //hash the module name
        if(cdada_list_push_back(dependency_list, &hash) != CDADA_SUCCESS) { //put into dependency list
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
        if (cdada_map_find(module_set, &hash, &module_info) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        else if (cdada_map_find(module_set, &hash, &module_info) != CDADA_E_NOT_FOUND) { //module_set map is corrupted
            find_dependency_result = INVALID_PARAMETER;
            goto cleanup;
        }
        if(libyangpush_find_submodule_direct_dep(included_module->submodule, module_set, reg_list) == INSERT_FAIL) {
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
    }

    find_dependency_result = libyangpush_load_submodule_direct_dependency_into_map_and_list(module_set, reg_list, dependency_list, module);
    if(find_dependency_result == FIND_DEPENDENCY_SUCCESS) {
        return find_dependency_result;
    }

cleanup:
    cdada_list_destroy(dependency_list);
    return find_dependency_result;
}

find_dependency_err_code_t libyangpush_find_module_reverse_dep(struct lys_module *module, cdada_map_t *module_set, cdada_list_t *reg_list)
{
    if(module == NULL || module_set == NULL || reg_list == NULL) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_find_module_reverse_dep]Invalid input parameter\n");
#endif
        return INVALID_PARAMETER;
    }
    int find_dependency_result = 0;
    unsigned long hash;
    struct lys_module **augment_module = module->augmented_by;
    struct lys_module **deviate_module = module->deviated_by;
    cdada_list_t *dependency_list = cdada_list_create(unsigned long);
    for(int i = LY_ARRAY_COUNT(augment_module); i > 0; i--) {
        void *module_info;
        hash = djb2((char*)augment_module[i-1]->name); //hash the module name
        if(cdada_list_push_back(dependency_list, &hash) != CDADA_SUCCESS) { //put into dependency list
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
        if (cdada_map_find(module_set, &hash, &module_info) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        else if (cdada_map_find(module_set, &hash, &module_info) != CDADA_E_NOT_FOUND) { //module_set map is corrupted
            find_dependency_result = INVALID_PARAMETER;
            goto cleanup;
        }
        if(libyangpush_find_module_direct_dep(augment_module[i-1], module_set, reg_list) == INSERT_FAIL) {
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
    }
    for(int i = LY_ARRAY_COUNT(deviate_module); i > 0; i--) {
        void *module_info;
        hash = djb2((char*)deviate_module[i-1]->name); //hash the module name
        if(cdada_list_push_back(dependency_list, &hash) != CDADA_SUCCESS) { //put into dependency list
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
        if (cdada_map_find(module_set, &hash, &module_info) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        else if (cdada_map_find(module_set, &hash, &module_info) != CDADA_E_NOT_FOUND) { //module_set map is corrupted
            find_dependency_result = INVALID_PARAMETER;
            goto cleanup;
        }
        if(libyangpush_find_module_direct_dep(deviate_module[i-1], module_set, reg_list) == INSERT_FAIL) {
            find_dependency_result = INSERT_FAIL;
            goto cleanup;
        }
    }

    find_dependency_result = libyangpush_load_module_reverse_dependency_into_map_and_list(module_set, reg_list, dependency_list, module);

cleanup:
    cdada_list_destroy(dependency_list);
    return find_dependency_result;
}

find_dependency_err_code_t libyangpush_find_all_dependency(struct lys_module *module, cdada_map_t *module_set, cdada_list_t *reg_list)
{
    int find_dependency_result = FIND_DEPENDENCY_SUCCESS;
    find_dependency_result = libyangpush_find_module_direct_dep(module, module_set, reg_list);
    if(find_dependency_result != FIND_DEPENDENCY_SUCCESS) {
        return find_dependency_result;
    }
    find_dependency_result = libyangpush_find_module_reverse_dep(module, module_set, reg_list);
    if(find_dependency_result != FIND_DEPENDENCY_SUCCESS) {
        return find_dependency_result;
    }
    return find_dependency_result;
}

void libyangpush_generate_subscription_info_with_xpath_filter(xmlNodePtr datastore_xpath_filter, 
            int sub_id, cdada_map_t *subscription_filter)
{
    char *filter = NULL;
    int find_xpath_error_code = libyangpush_parse_xpath(datastore_xpath_filter, &filter);
    struct subscription_filter_info *sub_filter_info = malloc(sizeof(struct subscription_filter_info));
    sub_filter_info->subscription_id = sub_id;

    switch (find_xpath_error_code) {
        case XPATH_PARSED_FAILED:
            free(sub_filter_info);
            return;
        case XPATH_NAMESPACE_FOUND:;
            sub_filter_info->filter_type = MODULE_NAMESPACE;
            sub_filter_info->module_num = 1;
            sub_filter_info->filter = (char**)malloc(sizeof(char*));
            sub_filter_info->filter[0] = filter;
            break;
        case XPATH_MODULE_NAME_FOUND:;
            sub_filter_info->filter_type = MODULE_NAME;
            sub_filter_info->module_num = 1;
            sub_filter_info->filter = (char**)malloc(sizeof(char*));
            sub_filter_info->filter[0] = filter;
            break;
        default:
            free(sub_filter_info);
            return;
    }
    cdada_map_insert(subscription_filter, &sub_id, sub_filter_info);

    return;
}

void libyangpush_generate_subscription_info_with_subtree_filter(xmlNodePtr datastore_subtree_filter, 
            int sub_id, cdada_map_t *subscription_filter)
{
    char **filter;
    int num_of_subscribed_modules = libyangpush_parse_subtree(datastore_subtree_filter, &filter);

    if (num_of_subscribed_modules != 0) {
        struct subscription_filter_info *sub_filter_info = malloc(sizeof(struct subscription_filter_info));
        sub_filter_info->filter = filter;
        sub_filter_info->module_num = num_of_subscribed_modules;
        sub_filter_info->filter_type = MODULE_NAMESPACE;
        sub_filter_info->subscription_id = sub_id;
        cdada_map_insert(subscription_filter, &sub_id, sub_filter_info);
    }
    return;
}

void libyangpush_parse_subscription_filter(xmlNodePtr subscriptions, cdada_map_t *subscription_filter)
{
    if (subscriptions == NULL || subscription_filter == NULL) {
        return;
    }
    void *map_val_ptr = NULL;
    xmlNodePtr current_subscription = xml_find_node(subscriptions, "subscription");
    while (current_subscription != NULL) {
        /* Get the subscription id of the current subscription */
        xmlNodePtr sub_id_node = xml_find_node(current_subscription, "id");
        if (sub_id_node == NULL){ // if the subscription has no sub_id, it is invalid
            return;
        }
        char* id_char = (char*)xmlNodeGetContent(sub_id_node);
        int id = atoi(id_char);
        free(id_char);

        /* Check if this subscription has been processed or not */
        if (cdada_map_find(subscription_filter, &id, map_val_ptr) == CDADA_SUCCESS) {
            current_subscription = current_subscription->next;
            continue;
        }

        xmlNodePtr datastore_filter_node = NULL;
        
        if (xml_find_node(current_subscription, "datastore-xpath-filter") != NULL) {
            datastore_filter_node = xml_find_node(current_subscription, "datastore-xpath-filter");
            libyangpush_generate_subscription_info_with_xpath_filter(datastore_filter_node, id, subscription_filter);
        }
        else if (xml_find_node(current_subscription, "datastore-subtree-filter") != NULL) {
            datastore_filter_node = xml_find_node(current_subscription, "datastore-subtree-filter");
            libyangpush_generate_subscription_info_with_subtree_filter(datastore_filter_node, id, subscription_filter);
        }
        /* datastore filter not found, id is the result */
        else {
            struct subscription_filter_info *sub_filter_info = malloc(sizeof(struct subscription_filter_info));
            sub_filter_info->module_num = 0;
            sub_filter_info->filter_type = SUBSCRIPTION_ID;
            sub_filter_info->subscription_id = id;
            cdada_map_insert(subscription_filter, &id, sub_filter_info);
        }

        current_subscription = current_subscription->next;
    }
    return;
}