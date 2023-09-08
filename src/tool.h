#include <curl/curl.h>
#include <jansson.h>
#include <string.h>
#include <cdada/map.h>
#include <cdada/list.h>

#define YANG_SCHEMA_REGISTRY "http://127.0.0.1:5000/subjects"
#define SUBJECT_PREFIX "subject_"

/**
 * A hash function used for converting string into a hash value
 * 
 * @param str the string to be converted
 * @return the unsigned long type hash
*/
unsigned long djb2(char *str);

/**
 * register schema into the configured schema registry
 * @param schema the schema json object
 * @param the subject name for the schema
 * 
 * @return the schema id for the registered schema
*/
int register_schema(json_t *schema, char *subject_name);

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