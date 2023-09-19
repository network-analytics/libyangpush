#include <libxml/tree.h>
#include <regex.h>
#include <string.h>
#include <libyang/tree_schema.h>
#include <libyang/libyang.h>
#include <cdada/map.h>
//Error codes
typedef enum
{
    XPATH_PARSED_FAILED,
    XPATH_NAMESPACE_FOUND,
    XPATH_MODULE_NAME_FOUND
}xpath_parsing_err_code_t;

typedef enum
{
    FIND_DEPENDENCY_SUCCESS,
    INSERT_SUCCESS,
    INSERT_FAIL,
    INVALID_PARAMETER
}find_dependency_err_code_t;

/**
 * @brief cache the yang code and module name
 */
struct module_info{
    char    *name;   /* The name of this module in the set */
    char    *yang_code; /* The yang code of this module */
};

/**
 * The function for cdada map to traversly free the module_info struct
 * @param s the cdada_map that's being traversed
 * @param k the key of the current element
 * @param v the pointer to the value of the current element
 * @param opaque User data (opaque ptr)
*/
void libyangpush_trav_clear_map(const cdada_map_t* s, const void* k, void* v, void* opaque);

/**
 * create module_info struct for yang module 'module' and insert it into cdada map
 * @param map the map in which the module_info is to be inserted
 * @param module the yang module to be loaded
 * 
 * @return find_dependency_err_code_t
*/
find_dependency_err_code_t libyangpush_load_module_into_map(cdada_map_t *map, struct lys_module* module);

/**
 * create module_info struct for yang submodule 'module' and insert it into cdada map
 * @param map the map in which the module_info is to be inserted
 * @param module the yang submodule to be loaded
 * 
 * @return find_dependency_err_code_t
*/
find_dependency_err_code_t libyangpush_load_submodule_into_map(cdada_map_t *map, struct lysp_submodule* module);

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
 * Parse the input datastore-subtree-filter node. 
 * The ***result either contains all module namespaces of the subtree, or none of them
 * 
 * @param darastore_subtree the xml node for field 'datastore-subtree-filter'
 * @param result the char pointer where the result is put
 * 
 * @return return the number of modules found in subtree. Returning 0 means no module found or parsing error
*/
size_t libyangpush_parse_subtree(xmlNodePtr datastore_subtree, char ***result);

/**
 * Find the import module for the passed in 'imported_module'
 * the 'module_set' stores all modules concerned in a find_dependency
 * the found module will call find_dependency. The process is recursive.
 * 
 * @param imported_module the sized array for all import module of one module
 * @param module_set the cdada map that stores all modules
 * 
 * @return the error code for find_dependency
*/
find_dependency_err_code_t libyangpush_find_import(struct lysp_import *imported_module, cdada_map_t *module_set);

/**
 * Find the include module for the passed in 'include_module'
 * the 'module_set' stores all modules concerned in a find_dependency.
 * the found module will call find_dependency. The process is recursive.
 * 
 * @param include_module the sized array for all include module of one module
 * @param module_set the cdada map that stores all modules
 * 
 * @return the error code for find_dependency
*/
find_dependency_err_code_t libyangpush_find_include(struct lysp_include *include_module, cdada_map_t *module_set);

/**
 * Find the reverse dependency modules(augment & deviate) for the passed in 'module'
 * the 'module_set' stores all modules concerned in a find_dependency call.
 * the found module will call find_dependency. The process is recursive.
 * 
 * @param module the sized array for this reverse dependency module
 * @param module_set the cdada map that stores all modules
 * 
 * @return the error code for find_dependency
*/
find_dependency_err_code_t libyangpush_find_reverse_dep(struct lys_module **module, cdada_map_t *module_set);