/* libyangpush functionality */
#include "libyangpush.h"
#include "tool.h"

void libyangpush_trav_clear_map(const cdada_map_t* s, const void* k, void* v, void* opaque)
{
    (void) k;
    (void) opaque;
    (void) s;
    struct module_info *value = v;
    free(value->yang_code);
    free(value->name);
    free(value->subject_prefix);
    free(value);
    return;
}

/* opaque is the pointer to the list of parent module */
void libyangpush_trav_copy_list(const cdada_list_t* s, const void* k, void* opaque)
{
    (void) s;
    unsigned long hash = *(unsigned long*)k;
    if(cdada_list_push_back(opaque, &hash) != CDADA_SUCCESS){
#if debug
        fprintf(stderr, "%s", "[libyangpush_trav_copy_list]Fail when inserting \n");
#endif
        return;
    }
    return;
}

void libyangpush_create_schema(struct module_info *module_ptr, cdada_map_t *map, unsigned long hash)
{
    json_t *references = json_array();

    for(int i = 0; i < (int)cdada_list_size(module_ptr->dependency_list); i++){ 
        unsigned long dependency_hash;
        struct module_info *dependency_module_ptr;
        cdada_list_get(module_ptr->dependency_list, i, &dependency_hash);
        if(dependency_hash == hash){
            continue;
        }
        cdada_map_find(map, &dependency_hash, (void**)&dependency_module_ptr);
        char subject_name[200];
        sprintf(subject_name, "%s%s", SUBJECT_PREFIX, dependency_module_ptr->name);
        json_t *reference = json_pack("{s:s,s:s,s:i}", //schematype, reference, schema
                                "subject",   subject_name,
                                "name",      dependency_module_ptr->name,
                                "version",   1);
        json_array_append(references, reference);
        json_decref(reference);
        dependency_module_ptr = NULL;
    }
    json_t *schema = json_pack("{s:s,s:o*,s:s}", //schematype, reference, schema
                               "schemaType",    "YANG",
                               "references",    references,
                               "schema",    module_ptr->yang_code);
    char subject_name[200];
    sprintf(subject_name, "%s%s", module_ptr->subject_prefix, module_ptr->name);

    register_schema(schema, subject_name);

    json_decref(references);
    json_decref(schema);

    cdada_list_clear(module_ptr->dependency_list);
    cdada_list_destroy(module_ptr->dependency_list);
}

void libyangpush_trav_list_n_clear_dep_list(const cdada_list_t* s, const void* k, void* opaque)
{
    (void) s;
    unsigned long hash = *(unsigned long*)k;
    cdada_map_t *map = opaque;

    struct module_info *module_ptr;
    // unsigned long hash = djb2(module_name);
    if(cdada_map_find(map, &hash, (void**)&module_ptr) != CDADA_SUCCESS) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_trav_register_schema]Fail when finding in map\n");
#endif
        goto cleanup;
    }
    // printf("module name: %s\n", module_ptr->name);
    libyangpush_create_schema(module_ptr, map, hash);

cleanup:
    cdada_list_clear(module_ptr->dependency_list);
    cdada_list_destroy(module_ptr->dependency_list);
    return;
}

struct module_info* libyangpush_load_module_into_map(cdada_map_t *map, cdada_list_t *list, struct lys_module* module)
{
    if(map == NULL || module == NULL || list == NULL){
        return NULL;
    }

    unsigned long hash = djb2((char*)(module->name)); //hash the module name
    struct module_info* yang_module_info_ptr = NULL;
    yang_module_info_ptr = (struct module_info*)malloc(sizeof(struct module_info));
    yang_module_info_ptr->dependency_list = cdada_list_create(unsigned long);
    yang_module_info_ptr->subject_prefix = calloc((strlen(SUBJECT_PREFIX)+1), sizeof(char));
    strcpy(yang_module_info_ptr->subject_prefix, SUBJECT_PREFIX);

    //write the module content to module_info->yang_code
    lys_print_mem(&(yang_module_info_ptr->yang_code), module, LYS_OUT_YANG, 0);

    //write the module name to module_info->name
    yang_module_info_ptr->name = calloc((strlen(module->name)+1), sizeof(char));
    strncpy(yang_module_info_ptr->name, (char*)(module->name), strlen(module->name));

    if((cdada_map_insert(map, &hash, yang_module_info_ptr) != CDADA_SUCCESS) || (cdada_list_push_back(list, &hash) != CDADA_SUCCESS)) { //inserted the module_info struct
#if debug
                fprintf(stderr, "%s%s", "[libyangpush_load_module_into_map]Fail when inserting \n", module->name);
#endif
        return NULL;
    }
    return yang_module_info_ptr;
}

struct module_info* libyangpush_load_submodule_into_map(cdada_map_t *map, cdada_list_t *list, struct lysp_submodule* module)
{
    if(map == NULL || module == NULL){
        return NULL;
    }

    unsigned long hash = djb2((char*)module->name); //hash the module name
    struct module_info* yang_module_info_ptr = NULL;
    yang_module_info_ptr =  (struct module_info*)malloc(sizeof(struct module_info)); //fill in the module_info struct
    yang_module_info_ptr->dependency_list = cdada_list_create(unsigned long);
    yang_module_info_ptr->subject_prefix = calloc((strlen(SUBJECT_PREFIX)+1), sizeof(char));
    strcpy(yang_module_info_ptr->subject_prefix, SUBJECT_PREFIX);

    struct ly_out* out = NULL;

    //write the module content to module_info->yang_code
    ly_out_new_memory(&(yang_module_info_ptr->yang_code), 0, &out);//store submodule content in char
    lys_print_submodule(out, module, LYS_OUT_YANG, 0, LY_PRINT_SHRINK);
    ly_out_free(out, 0, 0);

    //write the module name to module_info->name
    yang_module_info_ptr->name = calloc((strlen(module->name)+1), sizeof(char));
    strncpy(yang_module_info_ptr->name, (char*)(module->name), strlen(module->name));

    if((cdada_map_insert(map, &hash, yang_module_info_ptr) != CDADA_SUCCESS) || (cdada_list_push_back(list, &hash) != CDADA_SUCCESS)) { //inserted the module_info struct
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
            for(int i = 0; i < child_index; i++){ //free the previously allocated space
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

find_dependency_err_code_t libyangpush_find_import(struct lysp_import *imported_module, cdada_map_t *module_set, cdada_list_t *reg_list)
{
    if(imported_module == NULL){
#if debug
        fprintf(stderr, "%s", "[libyangpush_find_import]parameter imported_module is NULL\n");
#endif
        return INVALID_PARAMETER;
    }

    unsigned long hash;
    void *module_info_ptr = NULL;
    for(int i = 0; i < (int)LY_ARRAY_COUNT(imported_module); i++) {
        imported_module+=i;
        hash = djb2((char*)imported_module->name); //hash the module name
        struct module_info *module_if_ptr = NULL;
        if(cdada_map_find(module_set, &hash, &module_info_ptr) == CDADA_E_NOT_FOUND){ //module is not cached
            module_if_ptr = libyangpush_load_module_into_map(module_set, reg_list, imported_module->module);
#if demo_print
            printf("Found imported module: %s\n", module_if_ptr->name);
#endif
        }
        else if (cdada_map_find(module_set, &hash, &module_info_ptr) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        libyangpush_find_import(imported_module->module->parsed->imports, module_set, module_if_ptr->dependency_list);
        libyangpush_find_include(imported_module->module->parsed->includes, module_set, module_if_ptr->dependency_list);

        cdada_list_traverse(module_if_ptr->dependency_list, &libyangpush_trav_copy_list, reg_list);
    }
    
    return FIND_DEPENDENCY_SUCCESS;
}

find_dependency_err_code_t libyangpush_find_include(struct lysp_include *include_module, cdada_map_t *module_set, cdada_list_t *reg_list)
{
    if(include_module == NULL){
#if debug
        fprintf(stderr, "%s", "[libyangpush_find_import]Invalid input parameter\n");
#endif
        return INVALID_PARAMETER;
    }

    unsigned long hash;
    void *module_info_ptr = NULL;
    for(int i = 0; i < (int)LY_ARRAY_COUNT(include_module); i++) {
        include_module+=i;
        hash = djb2((char*)include_module->name); //hash the module name
        struct module_info *module_if_ptr = NULL;
        if(cdada_map_find(module_set, &hash, &module_info_ptr) == CDADA_E_NOT_FOUND){ //module is not cached
            module_if_ptr = libyangpush_load_submodule_into_map(module_set, reg_list, include_module->submodule);
        }
        else if (cdada_map_find(module_set, &hash, &module_info_ptr) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        libyangpush_find_import(include_module->submodule->imports, module_set, module_if_ptr->dependency_list);
        libyangpush_find_include(include_module->submodule->includes, module_set, module_if_ptr->dependency_list);
        cdada_list_traverse(module_if_ptr->dependency_list, &libyangpush_trav_copy_list, reg_list);
    }
    return FIND_DEPENDENCY_SUCCESS;
}

find_dependency_err_code_t libyangpush_find_reverse_dep(struct lys_module **module, cdada_map_t *module_set, cdada_list_t *reg_list, char* parent_module_name)
{
    if(module == NULL) {
#if debug
        fprintf(stderr, "%s", "[libyangpush_find_import]parameter imported_module is NULL\n");
#endif
        return INVALID_PARAMETER;
    }

    unsigned long hash;
    void *module_info_ptr = NULL;
    for(int i = LY_ARRAY_COUNT(module); i > 0; i--) {
        hash = djb2((char*)(module[i-1]->name)); //hash the module name
        struct module_info *module_if_ptr = NULL;
        if(cdada_map_find(module_set, &hash, &module_info_ptr) == CDADA_E_NOT_FOUND) { //module is not cached
            module_if_ptr = libyangpush_load_module_into_map(module_set, reg_list, module[i-1]);
#if demo_print
            printf("Found augmenting module: %s\n", module_if_ptr->name);
#endif
        }
        else if (cdada_map_find(module_set, &hash, &module_info_ptr) == CDADA_SUCCESS) { //modules is cached
            continue;
        }
        libyangpush_find_import(module[i-1]->parsed->imports, module_set, module_if_ptr->dependency_list);
        libyangpush_find_include(module[i-1]->parsed->includes, module_set, module_if_ptr->dependency_list);
        cdada_list_traverse(module_if_ptr->dependency_list, &libyangpush_trav_copy_list, reg_list);
        unsigned long parent_module_name_hash = djb2(parent_module_name);
        cdada_list_push_front(module_if_ptr->dependency_list, &parent_module_name_hash);
    }
    return FIND_DEPENDENCY_SUCCESS;
}