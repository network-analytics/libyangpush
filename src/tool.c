/* Tools functions */
#include "libyangpush.h"
#define debug 1

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
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
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
xmlNodePtr xml_find_node(xmlNodePtr node, const char* elem_name)
{
    xmlNodePtr result = NULL;
    while(node != NULL) {
        if((node->type == XML_ELEMENT_NODE) && (!xmlStrcmp(node->name, (xmlChar*)elem_name)))
            return node;

        result = xml_find_node(node->children, (char*)elem_name);

        if(result!= NULL) {
            return result;
        }
            
        node = node->next; 
    }
    return NULL;
}

/* Look for node with a specific element value within a list of nodes */
xmlNodePtr xml_find_node_by_keyval(xmlNodePtr node, const char* keyname, const char* value)
{
    xmlNodePtr childnode = xml_find_node(node, keyname);    
    while(strcmp((char*)xmlNodeGetContent(childnode), value) != 0) { // Check if the value match
        if(node == NULL)
            return NULL;
        childnode = xml_find_node(node, "name");
        if(childnode == NULL)
            return NULL;
        node = node->next;
    }
    return node;
}


cdada_list_t* parse_yanglib_msg_element_list(xmlNodePtr element_node) 
{
    if(element_node == NULL) { 
#if debug
        fprintf(stderr, "%s", "[parse_yanglib_msg_element_list]Invalid input parameter\n");
#endif
    } 
    cdada_list_t *element_val_list = cdada_list_create(sizeof(char*));
    int num_of_element = xmlChildElementCount(element_node->parent);

    char* element_val = (char*)calloc(strlen((char*)xmlNodeGetContent(element_node))+1, sizeof(char));
    strncpy(element_val, (char*)xmlNodeGetContent(element_node), strlen((char*)xmlNodeGetContent(element_node)));

    printf("stash into queue: %s\n", element_val);
    cdada_list_push_back(element_val_list, element_val);

    return element_val_list;
}

cdada_list_t* parse_yanglib_msg(const char *yanglib_msg, const char *module_name, const char *element_name)
{
    printf("%s/n", yanglib_msg);
    if(yanglib_msg == NULL || module_name ==NULL || element_name == NULL )
    {
#if debug
        fprintf(stderr, "%s", "[parse_yanglib_msg]Invalid input parameter\n");
#endif
        return NULL;
    }
    /* Initialization */
    xmlDocPtr yanglib_xmldoc = NULL;
    xmlNodePtr yanglib_xmlnode, module_node, element_node;
    cdada_list_t *element_list = NULL;

    yanglib_xmldoc = xmlParseDoc((xmlChar*)yanglib_msg); 
    yanglib_xmlnode = xmlDocGetRootElement(yanglib_xmldoc);

    if(yanglib_xmldoc != NULL) { 
        module_node = xml_find_node(yanglib_xmlnode, "module"); // Find the module node

        if(module_node != NULL) {
            module_node = xml_find_node_by_keyval(module_node, "name", module_name);
#if debug
            printf("\nnode content: %s\n", xmlNodeGetContent(xml_find_node(module_node, "name")));
#endif
            element_node = xml_find_node(module_node, (const char*)element_name); // Find the element node with specific name
            while(element_node != NULL) { // Element node Processing
                element_list = parse_yanglib_msg_element_list(element_node);
#if debug
                printf("\nelement_node: %s\n", xmlNodeGetContent(element_node));
#endif
                element_node = element_node->next; 
                element_node = xml_find_node(element_node, element_name); // Find the next element node

            }
        }
        else {
#if debug
            fprintf(stderr, "%s", "[parse_yanglib_msg]module not found in yanglib\n");
#endif
        }
    }
    else {
#if debug
        fprintf(stderr, "%s", "[parse_yanglib_msg]element not found under module_name\n");
#endif
    }
    xmlFreeDoc(yanglib_xmldoc);
    return element_list;
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