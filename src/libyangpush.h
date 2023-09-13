#include <libxml/tree.h>
#include <regex.h>
#include <string.h>
#include <libyang/tree_schema.h>
#include <libyang/libyang.h>
#include <cdada/map.h>
#include <cdada/list.h>
#include <jansson.h>
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
    INSERT_FAIL,
    INSERT_SUCCESS,
    INVALID_PARAMETER
}find_dependency_err_code_t;

typedef enum
{
    DIRECT_FORWARD,
    SUBSCRIPTION_ID,
    MODULE_NAMESPACE,
    MODULE_NAME,
}
parse_subscription_err_code_t;

struct subscription_filter_info
{
    parse_subscription_err_code_t filter_type;
    int subscription_id;
    int module_num;
    char **filter;
};

/**
 * @brief cache the yang code and module name
 */
struct module_info
{
    char    *name;   /* The name of this module in the set */
    char    *yang_code; /* The yang code of this module */
    cdada_list_t *dependency_list;
};

/**
 * @brief defined for being passed as user_data to the list traverse function 
*/
struct schema_info
{
    cdada_map_t *module_set;
    int schema_id;
    char *version;
    char *schema_registry_address;
    char *schema_subject_prefix;
};

/**
 * create schema references accordingto the dependency list in the module_info 
 * struct and return the json reference object
 * @param module_ptr the pointer to the module_info struct
 * @param module_set the module set that contains all modules for one subscription
 * 
 * @return json object of the schema reference
 */
json_t* libyangpush_create_reference(struct module_info *module_ptr, cdada_map_t *module_set, char *version, char *subject_prefix);

/**
 * create schema for the module stored in module_ptr
 * @param module_ptr pointer to the struct module_info that stores information of the module being registered
 * @param module_set the map that stores all modules concered in the schema registration
 * @param module_name_hash the djb2 hash of the name of the currently processing module
 * 
 * @return the json object of the schema
*/
json_t* libyangpush_create_schema(struct module_info *module_ptr, json_t *references);

/**
 * A traverse function for cdada_list. The function is ran for each key in the list. 
 * user_data should contain the pointer to struct cdadamap_n_schemaid. The struct 
 * include the module set map and an integer to record the schema id. The key is 
 * recorded as the hash of the module name. The function searches for the hash in 
 * the module set and find the module_info of the module. The information in module_info 
 * is used for creating schema
 * @param reg_list the registration list
 * @param key each element in the list
 * @param user_data contains the cdadamap_n_schemaid struct
 * 
 * @return the schema id for this subscription is recorded in the schema_id in user_data
*/
void libyangpush_trav_list_register_schema(const cdada_list_t* reg_list, const void* key, void* schema_info);

/**
 * The function for cdada map to traversly free the module_info struct
 * @param traversed_map the cdada_map that's being traversed
 * @param key the key of the current element
 * @param val the pointer to the value of the current element
 * @param user_define_data User data (opaque ptr)
*/
void libyangpush_trav_clear_module_set_map(const cdada_map_t* traversed_map, const void* key, void* val, void* user_define_data);

/**
 * The function for cdada map to traversly free the subscription_filter_info struct
 * @param traversed_map the cdada_map that's being traversed
 * @param key the key of the current element
 * @param val the pointer to the value of the current element
 * @param user_define_data User data (opaque ptr)
*/
void libyangpush_trav_clear_subscription_filter_map(const cdada_map_t* traversed_map, const void* key, void* val, void* user_define_data);

/**
 * create module_info struct for yang module 'module' and insert it into cdada map
 * @param map the map in which the module_info is to be inserted
 * @param module the yang module to be loaded
 * @param hash_index the hash index od the element in the map
 * 
 * @return the inserted module_info struct
*/
struct module_info* libyangpush_load_module_into_map(cdada_map_t *map, struct lys_module* module, unsigned long hash_index);

/**
 * create module_info struct for yang submodule 'module' and insert it into cdada map
 * @param map the map in which the module_info is to be inserted
 * @param module the yang submodule to be loaded
 * @param hash_index the hash index od the element in the map
 * 
 * @return the inserted module_info struct
*/
struct module_info* libyangpush_load_submodule_into_map(cdada_map_t *map, struct lysp_submodule* module, unsigned long hash_index);

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
 * This function load the module into module_set map, register list
 * and its direct dependency list into module's module_info
 * 
 * @param module_set the cdada map tha contains all modules concern in a dependency-search
 * @param register_list the ordered list of module to be registered to schema registry
 * @param dependency_list this list belongs to the current module. It should be put into module_info struct
 * @param module the yang module struct of current module
 * 
 * @return find_dependency_err_code
*/
find_dependency_err_code_t libyangpush_load_module_direct_dependency_into_map_and_list(cdada_map_t *module_set, cdada_list_t *register_list, cdada_list_t *dependency_list, struct lys_module *module);

/**
 * This function load the submodule into module_set map, register list
 * and its direct dependency list into module's module_info
 * 
 * @param module_set the cdada map tha contains all modules concern in a dependency-search
 * @param register_list the ordered list of module to be registered to schema registry
 * @param dependency_list this list belongs to the current module. It should be put into module_info struct
 * @param module the yang submodule struct of current module
 * 
 * @return find_dependency_err_code
*/
find_dependency_err_code_t libyangpush_load_submodule_direct_dependency_into_map_and_list(cdada_map_t *module_set, cdada_list_t *register_list, cdada_list_t *dependency_list, struct lysp_submodule *module);

/**
 * This function load the module into module_set map, register list(if it's not in module_set map yet)
 * and its reverse dependency list into module's module_info
 * 
 * @param module_set the cdada map tha contains all modules concern in a dependency-search
 * @param register_list the ordered list of module to be registered to schema registry
 * @param dependency_list this list belongs to the current module. It should be put into module_info struct
 * @param module the yang module struct of current module
 * 
 * @return find_dependency_err_code
*/
find_dependency_err_code_t libyangpush_load_module_reverse_dependency_into_map_and_list(cdada_map_t *module_set, cdada_list_t *register_list, cdada_list_t *dependency_list, struct lys_module *module);

/**
 * find the direct dependency(include, import) of a module, and store then into module_set,
 * and register in the reg_list
 * 
 * @param module the module that we are finding dependency for
 * @param module_set the cdada_map that contains all modules concern in a find-dependency
 * @param reg_list the ordered list of module to be registered into schema registry
 * 
 * @return find_dependency_err_code
*/
find_dependency_err_code_t libyangpush_find_module_direct_dep(struct lys_module *module, cdada_map_t *module_set, cdada_list_t *reg_list);

/**
 * find the direct dependency(include, import) of a submodule, and store then into module_set,
 * and register in the reg_list
 * 
 * @param module the submodule that we are finding dependency for
 * @param module_set the cdada_map that contains all modules concern in a find-dependency
 * @param reg_list the ordered list of module to be registered into schema registry
 * 
 * @return find_dependency_err_code
*/
find_dependency_err_code_t libyangpush_find_submodule_direct_dep(struct lysp_submodule *module, cdada_map_t *module_set, cdada_list_t *reg_list);

/**
 * find the reverse dependency(augment, deviate) of module, and store then into module_set,
 * and register in the reg_list
 * 
 * @param module the module that we are finding dependency for
 * @param module_set the cdada_map that contains all modules concern in a find-dependency
 * @param reg_list the ordered list of module to be registered into schema registry
 * 
 * @return find_dependency_err_code
*/
find_dependency_err_code_t libyangpush_find_module_reverse_dep(struct lys_module *module, cdada_map_t *module_set, cdada_list_t *reg_list);

/**
 * the top level function for find all dependency(import, include, augment, deviate)
 * 
 * @param module the module that we are finding dependency for
 * @param module_set the cdada_map that contains all modules concern in a find-dependency
 * @param reg_list the ordered list of module to be registered into schema registry
 * 
 * @return find_dependency_err_code
*/
find_dependency_err_code_t libyangpush_find_all_dependency(struct lys_module *module, cdada_map_t *module_set, cdada_list_t *reg_list);

/**
 * Parse the datastore_xpath_filter to find out the filter(namespace or module name),
 * and store it in the subscription_filter cdada_map.
 * 
 * @param datastore_xpath_filter the xml xpath_filter node that needs to be parsed
 * @param sub_id the subscription id that is gping to be used as the index for filter in the 'subscription_filter' cdada map
 * @param subscription_filter the cdada mpa used to store the filter information the value is a pointer to 'struct subscription_filter_info'
*/
void libyangpush_generate_subscription_info_with_xpath_filter(xmlNodePtr datastore_xpath_filter, 
            int sub_id, cdada_map_t *subscription_filter);

/**
 * Parse the datastore_subtree_filter to find out the filter(result can only be 
 * namesapce), and store it in the subscription_filter cdada_map.
 * 
 * @param datastore_xpath_filter the xml xpath_filter node that needs to be parsed
 * @param sub_id the subscription id that is gping to be used as the index for filter in the 'subscription_filter' cdada map
 * @param subscription_filter the cdada mpa used to store the filter information the value is a pointer to 'struct subscription_filter_info'
*/
void libyangpush_generate_subscription_info_with_subtree_filter(xmlNodePtr datastore_subtree_filter, 
            int sub_id, cdada_map_t *subscription_filter);

/**
 * This function parse the list of subscription in the xml YANG push message.
 * The parsed subscription will be store in 'struct subscription_filter_info'
 * that contains the subscription filter, filter type, subscription id, numbers
 * of filter. All subscriptions will be parsed and pointers are stored in a 
 * cdada map with index being the subscription id of them. 
 * 
 * @param subscriptions the xml subscriptions node that pointes to be subscription list that needs to be parsed
 * @param subscription_filter the cdada mpa used to store the filter information the value is a pointer to 'struct subscription_filter_info'
*/
void libyangpush_parse_subscription_filter(xmlNodePtr subscriptions, cdada_map_t *subscription_filter);
