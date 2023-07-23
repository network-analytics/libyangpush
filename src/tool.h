#include <curl/curl.h>
#include <jansson.h>

unsigned long djb2(char *str);
void register_schema(json_t *schema);
void print_schema_registry_response(void *ptr, size_t size, size_t nmemb, void *data);