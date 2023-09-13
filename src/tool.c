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

int register_schema(char *schema_registry_address, json_t *schema, char *subject_name)
{
    CURL *curl;
    CURLcode res;
    int schema_id = 0;
    struct curl_slist *chunk = NULL;
    char url[200];
    sprintf(url, "%s/%s/versions", schema_registry_address, subject_name);

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

/* Look for the node with key as the elem_name in xml tree */
xmlNodePtr xml_find_node(xmlNodePtr node, char* elem_name)
{
    xmlNodePtr result = NULL;
    while(node != NULL) {
        if((node->type == XML_ELEMENT_NODE) && (!xmlStrcmp(node->name, (const xmlChar*)elem_name)))
            return node;

        result = xml_find_node(node->children, elem_name);

        if(result!= NULL) {
            return result;
        }
            
        node = node->next; 
    }
    return NULL;
}

message_parse_error_code_t validate_message_structure(void *message, xmlNodePtr *subscription_list_ptr, int *sub_id)
{
    xmlDocPtr xmlmsg;
    xmlmsg = xmlParseDoc((xmlChar*)message);
    xmlNodePtr current_node, prev_node;
    int message_validation_result = 0;

    prev_node = xmlDocGetRootElement(xmlmsg);
    
    current_node = xml_find_node(prev_node, "notification"); // The root node if notification
    if(current_node != NULL){
        prev_node = current_node;
        current_node = xml_find_node(prev_node, "push-update"); // if it has push-update field
        if(current_node == NULL){
            current_node = xml_find_node(prev_node, "push-change-update"); //if it has push-change-update field
        }
        if(current_node != NULL){
            current_node = xml_find_node(current_node, "id");
            if(current_node != NULL){
                char *id = (char*)xmlNodeGetContent(current_node);
                int int_id = atoi(id);
                *sub_id = int_id; //store the id into sub_id
                free(id);
                current_node = xml_find_node(current_node, "datastore-contents");
                if(current_node != NULL){
                    current_node = xml_find_node(current_node, "subscriptions");
                    if(current_node != NULL){
                        current_node = xml_find_node(current_node, "subscription");
                        *subscription_list_ptr = xmlCopyNodeList(current_node);
                    }
                }
                else {
#if debug
                    fprintf(stderr, "%s", "no datastore-content found\n");
#endif
                    message_validation_result = MESSAGE_STRUCTURE_INVALID;
                    goto cleanup;
                } 
            }
            else {
#if debug
                fprintf(stderr, "%s", "no subscription id found\n");
#endif
                message_validation_result = MESSAGE_STRUCTURE_INVALID;
                goto cleanup;
            }
        }
        else {
#if debug
            fprintf(stderr, "%s", "not a push-update\n");
#endif
            message_validation_result = MESSAGE_STRUCTURE_INVALID;
            goto cleanup;
        }
    }
    else {
#if debug
        fprintf(stderr, "%s", "not a notification message\n");
#endif
        message_validation_result = MESSAGE_STRUCTURE_INVALID;
        goto cleanup;
    }

cleanup:
    xmlFreeDoc(xmlmsg);
    return message_validation_result;
}