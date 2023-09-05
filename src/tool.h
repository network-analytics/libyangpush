#include <curl/curl.h>
#include <jansson.h>
#include <string.h>

#define YANG_SCHEMA_REGISTRY "http://127.0.0.1:5000/subjects"
#define SUBJECT_PREFIX "subject_"

unsigned long djb2(char *str);
int register_schema(json_t *schema, char *subject_name);
void print_schema_registry_response(void *ptr, size_t size, size_t nmemb, void *data);