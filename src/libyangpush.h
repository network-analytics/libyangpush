#include <libxml/tree.h>
#include <regex.h>
#include <string.h>
//Error codes
typedef enum
{
    XPATH_PARSED_FAILED,
    XPATH_NAMESPACE_FOUND,
    XPATH_MODULE_NAME_FOUND
}xpath_parsing_err_code_t;

typedef enum
{
    SUBTREE_PARSED_FAILED,
    SUBTREE_NAMESPACE_FOUND
}subtree_parsing_err_code_t;

#define ERR_MSG_CLEANUP(msg) \
        fprintf(stderr, "%s", msg); \
        goto cleanup;

/** 
 * Perform pattern match for string
 * @param pattern the regular expression
 * @param string the input string to be proceeded
 * 
 * @return Return:
 *         1. the match result if successfully matched
 *         2. NULL if no match
*/
char* libyangpush_pattern_match(char *pattern, char *string);

/**
 * Iterate through all namespaces, check if there is one namespace that
 * is specified for the passed-in prefix. If so, return the prefix.
 * If there is no, return NULL.
 * @param namespaces the pointer to namespaces node for an xml node
 * @param prefix a prefix extracted from an xpath
 * 
 * @return
 *      1. The namespace, if there is one for the prefix
 *      2. NULL if no namespace found
*/
char* libyangpush_find_namespace_for_prefix(xmlNs **namespaces, char *prefix);

/** 
 * Parse the input datastore-xpath-filter node.
 * @param darastore_xpath the xml node for field 'datastore-xpath-filter'
 * @param result the char pointer where the result is put
 * 
 * @return return the type xpath_parsing_err_code_t
*/
xpath_parsing_err_code_t libyangpush_parse_xpath(xmlNodePtr datastore_xpath, char **result);

/** 
 * Parse the input datastore-xpath-filter node.
 * @param darastore_subtree the xml node for field 'datastore-subtree-filter'
 * @param result the char pointer where the result is put
 * 
 * @return return the type xpath_parsing_err_code_t
*/
subtree_parsing_err_code_t libyangpush_parse_subtree(xmlNodePtr datastore_subtree, char** result);