#include <curl/curl.h>
#include <jansson.h>

#define YANG_SCHEMA_REGISTRY "http://127.0.0.1:5000/subjects"
#define SUBJECT_PREFIX "hackathon_demo_"

unsigned long djb2(char *str);
void register_schema(json_t *schema, char* subject_name);
void print_schema_registry_response(void *ptr, size_t size, size_t nmemb, void *data);