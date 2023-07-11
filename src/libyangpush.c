/* libyangpush functionality */
#include "libyangpush.h"

char* libyangpush_pattern_match(char* pattern, char* string)
{
    int has_pattern_matched = 0;
    const size_t maxMatches = 1;
    regex_t regex_pattern;
    regmatch_t matched_string_offset[maxMatches];
    char* result = NULL;

    if(pattern == NULL || string == NULL) {
        ERR_MSG_CLEANUP("[libyangpush_pattern_match]Invalid arguments\n");
    }

    has_pattern_matched = regcomp(&regex_pattern, pattern, REG_EXTENDED); //Compile the pattern, return 0 if successfuel, vice versa

    if (has_pattern_matched) {
        ERR_MSG_CLEANUP("[libyangpush_pattern_match]pattern compilation filed\n");
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
        ERR_MSG_CLEANUP("[libyangpush_pattern_match]Pattern not found.\n");
    }

cleanup:
    regfree(&regex_pattern);
    return result;
}

char* libyangpush_find_namespace_for_prefix(xmlNs** namespaces, char *xpath_prefix)
{
    if(namespaces == NULL || xpath_prefix == NULL) {
        fprintf(stderr, "%s", "[libyangpush_find_namespace_for_prefix]Invalid arguments\n");
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

subtree_parsing_err_code_t libyangpush_parse_subtree(xmlNodePtr datastore_subtree, char** result)
{
    if (datastore_subtree == NULL || strcmp((char*)datastore_subtree->name, "datastore-subtree-filter")) { //if this is not the datastore-xpath-filter field, parsed fail
        return SUBTREE_PARSED_FAILED;
    }

    xmlNodePtr subtree = datastore_subtree->children; //get the subtree node
    if(subtree == NULL) {
        fprintf(stderr, "%s", "[libyangpush_parse_subtree]Invalid subtree\n");
        return SUBTREE_PARSED_FAILED;
    }
    xmlNs *subtree_namespace = subtree->nsDef;
    if(subtree_namespace == NULL) {
        fprintf(stderr, "%s", "[libyangpush_parse_subtree]Invalid subtree\n");
        return SUBTREE_PARSED_FAILED;
    }

    if(subtree->ns != NULL && subtree->ns->prefix != NULL) { //If the node has namespace prefix
        char* namespace = libyangpush_find_namespace_for_prefix(&subtree_namespace, (char*)subtree->ns->prefix);//check the namespace for the namespace prefix 
        if(namespace != NULL) {
            *result = (char*)calloc((strlen(namespace)+1), sizeof(char));
            strncpy(*result, (char*)namespace, strlen((char*)namespace));
            return SUBTREE_NAMESPACE_FOUND;
        }
    }
    else { //The node has no prefix, the first one will be the one
        *result = (char*)calloc((strlen((char*)subtree->nsDef->href)+1), sizeof(char));
        strncpy(*result, (char*)subtree->nsDef->href, strlen((char*)subtree->nsDef->href));
        return SUBTREE_NAMESPACE_FOUND;
    }
    return SUBTREE_PARSED_FAILED;
}