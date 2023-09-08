/* Tools functions */
#include "tool.h"

unsigned long djb2(char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

size_t print_schema_registry_response_clb(void *response, size_t size, size_t nmemb, void *data)
{
    int schemaID;
    sscanf((char*)response, "{\"id\":%d}", &schemaID);
    *(int*)data = schemaID;

    return size * nmemb;
}

int register_schema(json_t *schema, char *subject_name)
{
    CURL *curl;
    CURLcode res;
    int schema_id = 0;
    struct curl_slist *chunk = NULL;
    char url[200];
    sprintf(url, "%s/%s/versions", YANG_SCHEMA_REGISTRY, subject_name);

    char *postthis = json_dumps(schema, JSON_INDENT(2));
    chunk = curl_slist_append(chunk, "Content-Type: application/json");
#if demo_print
    printf("url: %s\n", url);
    printf("%s\n", postthis);
#endif
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, print_schema_registry_response_clb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&schema_id);
        /* if we do not provide POSTFIELDSIZE, libcurl will strlen() by
        itself */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postthis));

        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_slist_free_all(chunk);
    free(postthis);
    return schema_id;
}

void trav_copy_list(const cdada_map_t* traversed_list, const void* key, void* result_list)
{
    (void) traversed_list;
    if(cdada_list_push_front((cdada_list_t*)result_list, key) != CDADA_SUCCESS) {
        return;
    }
}