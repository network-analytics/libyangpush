/* Tools functions */
#include "tool.h"
#include "libyangpush.h"

unsigned long djb2(char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

void print_schema_registry_response_clb(void *ptr, size_t size, size_t nmemb, void *data)
{
    int schemaID;
    printf("response:\n%s", (char*)ptr);
    sscanf((char*)ptr, "{\"id\":%d}", &schemaID);
    // *(int*)data = schemaID;
    printf("schema id :%d\n", schemaID);
}

void register_schema(json_t *schema, char* subject_name)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *chunk = NULL;
    char url[200];
    sprintf(url, "%s/hackathon_demo%s/versions", YANG_SCHEMA_REGISTRY, subject_name);
    char *postthis = json_dumps(schema, JSON_ENSURE_ASCII);
    chunk = curl_slist_append(chunk, "Content-Type: application/json");
    // printf("size %d\ncontent:%s", strlen(postthis), postthis);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
        // curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, print_schema_registry_response_clb);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, print_schema_registry_response_clb);

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
    return;
}