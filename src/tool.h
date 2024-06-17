#include <curl/curl.h>
#include <jansson.h>
#include <string.h>
#include <cdada/map.h>
#include <cdada/list.h>
#include <libxml/tree.h>

typedef enum
{
    MESSAGE_STRUCTURE_VALID,
    MESSAGE_STRUCTURE_INVALID
}message_parse_error_code_t;

/**
 * A hash function used for converting string into a hash value
 * 
 * @param str the string to be converted
 * @return the unsigned long type hash
*/
unsigned long djb2(char *str);

/**
 * register schema into the configured schema registry
 * @param schema_registry_address the address for connecting to schema registry
 * @param schema the schema json object
 * @param the subject name for the schema
 * 
 * @return the schema id for the registered schema
*/
int register_schema(char *schema_registry_address, json_t *schema, char *subject_name);

/**
 * The callback function for collecting the response of schema
 * register request to the schema registry server.
 * 
 * @param response the pointer to the response struct from schema registry server
*/
size_t print_schema_registry_response_clb(void *response, size_t size, size_t nmemb, void *data);


/**
 * The function for cdada list to be copied on top of another list passed through user_define_data
 * @param traversed_list the cdada_map that's being traversed
 * @param key the key of the current element
 * @param user_define_data User data (opaque ptr)
*/
void trav_copy_list(const cdada_map_t* traversed_list, const void* key, void* user_define_data);

/**
 * Look for the xml node with the node-key being the elem_name in parameter
 * @param node the xml node pointer pointing to the root node that we are searching in
 * @param elem_name the key name that we are searching for
*/
xmlNodePtr xml_find_node(xmlNodePtr node, char* elem_name);

/**
 * The function for validating the YANG push notification message structure
 * The message should contain "notification" as root node, with sequentially
 * "push-update" or "push-change-update", "id", "datastore-contents" in its 
 * childs.
 * @param message the YANG push message to be validated
 * @param subscription_list_ptr the xml node pointer pointing to the subscription list
 * @param sub_id the integer pointer pointing to where we will store the subscription id if validation succeeds.
 * 
 * @return message_parse_error_code_t
*/
message_parse_error_code_t validate_message_structure(void *message, xmlNodePtr *subscription_list_ptr, int *sub_id);