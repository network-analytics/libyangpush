#include <stdio.h>
#include <sys/stat.h>
#include <libyang/libyang.h>
#include <libyang/tree_schema.h>
#include <cdada/map.h>
#include <cdada/list.h>
#include <libxml/tree.h>
#include <nc_client.h>
#include "libyangpush.h"
#include "tool.h"
#include "demo.h"

char* load_msg(char *filename){
    char* text;

    FILE *fptr = fopen(filename, "r");
    fseek(fptr, 0, SEEK_END);
    int flen = ftell(fptr); //file length
    fseek(fptr, 0, SEEK_SET); 

    text = (char*)calloc((flen+1), sizeof(char));
    fread(text, flen, 1, fptr); 
    fclose(fptr);

    return text;
}

int connect_netconf(struct ly_ctx **ctx, struct nc_session **session){
    int rc = 0;
    nc_client_init();
    nc_client_set_schema_searchpath(CONTEXT_SEARCH_PATH);
    /* set the client SSH username to always be used when connecting to the server */
    if(nc_client_ssh_set_username(SSH_USERNAME)){
        rc = 1;
        fprintf(stderr, "%s", "Couldn't set the SSH username\n");
    }
    nc_client_ssh_add_keypair(SSH_PUBLIC_KEY, SSH_PRIVATE_KEY);
    nc_client_ssh_set_auth_pref(NC_SSH_AUTH_PUBLICKEY, 4);

    *session = nc_connect_ssh(SSH_ADDRESS, SSH_PORT, *ctx);
    if (!(*session)) {
        rc = 1;
        fprintf(stderr, "%s", "Couldn't connect to the server\n");
    }

    return rc;
}

/* Look for the node with key as the elem_name in xml tree */
xmlNodePtr xml_find_node(xmlNodePtr node, char* elem_name){
    xmlNodePtr result = NULL;
    while(node != NULL){
        if((node->type == XML_ELEMENT_NODE) && (!xmlStrcmp(node->name, (const xmlChar*)elem_name)))
            return node;

        result = xml_find_node(node->children, elem_name);

        if(result!= NULL){
            return result;
        }
            
        node = node->next; 
    }
    return NULL;
}

void validate_msg_structure(xmlDocPtr xml_msg, xmlNodePtr *datastore_contents, int *sub_id)
{
    xmlNodePtr current_node, prev_node;

    prev_node = xmlDocGetRootElement(xml_msg);
    
    current_node = xml_find_node(prev_node, "notification");
    if(current_node != NULL){
        prev_node = current_node;
        current_node = xml_find_node(prev_node, "push-update");
        if(current_node != NULL){
            prev_node = current_node;
            current_node = xml_find_node(prev_node, "id");
            if(current_node != NULL){
                prev_node = current_node;
                current_node = xml_find_node(prev_node, "datastore-contents");
                if(current_node != NULL){
                    *datastore_contents = current_node;
                    char *id = (char*)xmlNodeGetContent(prev_node);
                    sscanf(id, "%d", sub_id);
                }
                else {
                    fprintf(stderr, "%s", "no datastore-content found\n");
                    goto cleanup;
                } 
            }
            else {
                fprintf(stderr, "%s", "no subscription id found\n");
                goto cleanup;
            }
        }
        else {
            fprintf(stderr, "%s", "not a push-update\n");
            goto cleanup;
        }
    }
    else {
        fprintf(stderr, "%s", "not a notification message\n");
        goto cleanup;
    }
cleanup:
    return;
}

void find_dependency_n_create_schema(struct ly_ctx *test_ctx, struct lys_module *test_module)
{
    
    cdada_map_t* test1_module_set = cdada_map_create(unsigned long);
    cdada_list_t* test1_reg_list = cdada_list_create(unsigned long);

    //Test
    libyangpush_find_import(test_module->parsed->imports, test1_module_set, test1_reg_list);
    libyangpush_find_include(test_module->parsed->includes, test1_module_set, test1_reg_list);

    struct module_info *test_module_if_ptr = libyangpush_load_module_into_map(test1_module_set, test1_reg_list, test_module); /* parent module needs to be inserted into map to avoid
                                                                          their reverse dependency identifying them as an import*/
    libyangpush_create_schema(test_module_if_ptr, test1_module_set, djb2(test_module->name));

    libyangpush_find_reverse_dep(test_module->augmented_by, test1_module_set, test1_reg_list, test_module->name);
    printf("cdada list size %d\n", cdada_list_size(test1_reg_list));
    cdada_list_traverse(test1_reg_list, &libyangpush_trav_copy_list, test_module_if_ptr->dependency_list);

    cdada_list_rtraverse(test1_reg_list, &libyangpush_trav_list_n_clear_dep_list, test1_module_set);
    
    cdada_map_traverse(test1_module_set, &libyangpush_trav_clear_map, NULL);
    cdada_map_destroy(test1_module_set);
    cdada_list_destroy(test1_reg_list);
}

void demo(char *msg)
{
    struct ly_ctx *test_ctx;
    if(ly_ctx_new(CONTEXT_SEARCH_PATH, 1, &test_ctx) != LY_SUCCESS){
        fprintf(stderr, "%s", "context creation error\n");
        return;
    }
    struct nc_session *session = NULL;
    connect_netconf(&test_ctx, &session);
    nc_session_free(session, NULL);
    nc_client_destroy();

    xmlDocPtr xmlmsg;
    xmlNodePtr datastore_content, datastore_xpath;
    xmlmsg = xmlParseDoc((xmlChar*)msg);
    int id = 0;
    validate_msg_structure(xmlmsg, &datastore_content, &id);

    char *parse_result;
    datastore_xpath = xml_find_node(datastore_content, "datastore-xpath-filter");
    int return_type = libyangpush_parse_xpath(datastore_xpath, &parse_result);
    struct lys_module *module = NULL;
    switch(return_type){
        case XPATH_PARSED_FAILED:
            fprintf(stderr, "%s", "Xpath parse fail\n");
            goto cleanup;
        case XPATH_NAMESPACE_FOUND:
            module = ly_ctx_get_module_implemented_ns(test_ctx, parse_result);
            if(module == NULL){
                fprintf(stderr, "%s", "[libyangpush_receiver]module null\n");
                goto cleanup;
            }
            break;
        case XPATH_MODULE_NAME_FOUND:
            module = ly_ctx_get_module_implemented(test_ctx, parse_result);
            if(module == NULL){
                fprintf(stderr, "%s", "[libyangpush_receiver]module null\n");
                goto cleanup;
            }
            break;
    }
    find_dependency_n_create_schema(test_ctx, module);
    
cleanup:
    free(parse_result);
    ly_ctx_destroy(test_ctx);
    xmlFreeDoc(xmlmsg);
    return;
}

int main(int argc, char **argv) // load msg from file
{
    if(argc != 2){
        fprintf(stderr, "%s\n", "wrong number of input parameters");
    }

    int id = 0;
    char* msg = load_msg(argv[1]);

    demo(msg);
    free(msg);
    return 0;
}

