#include <libyangpush.h>
#include <tool.h>
#include <libyang/libyang.h>
#include <nc_client.h>
#include <cdada/map.h>
#include <cdada/list.h>
#include "example.h"

/* A helper function for loading file from disk */
char* load_file_from_disk(char *filename)
{
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

int connect_netconf(struct ly_ctx **ctx, struct nc_session **session)
{
    int rc = 0;
    nc_client_init();
    nc_client_set_schema_searchpath(YANG_MODULE_CONTEXT_SEARCH_PATH);
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

void trav_create_schema(const cdada_map_t* traversed_map, const void* key, void* val, void* user_define_data)
{
    (void) key;
    (void) traversed_map;
    struct subscription_filter_info *value = val;
    struct ly_ctx *netconf_context = (struct ly_ctx*)user_define_data;

    if (value->module_num != 0) {
        cdada_map_t *module_set = cdada_map_create(unsigned long);
        cdada_list_t *reg_list = cdada_list_create(unsigned long);
        for (int i = 0; i < value->module_num; i++) {
            struct lys_module *subscribed_module = NULL;
            if (value->filter_type == MODULE_NAME) {
                subscribed_module = ly_ctx_get_module_implemented(netconf_context, value->filter[i]);
            }
            else if (value->filter_type == MODULE_NAMESPACE) {
                subscribed_module = ly_ctx_get_module_implemented_ns(netconf_context, value->filter[i]);
            }
            libyangpush_find_all_dependency(subscribed_module, module_set, reg_list);
        }
        struct schema_info *schema_info_ptr = malloc(sizeof(struct schema_info));
        schema_info_ptr->schema_registry_address = YANG_SCHEMA_REGISTRY;
        schema_info_ptr->schema_subject_prefix = SUBJECT_PREFIX;
        schema_info_ptr->version = "1";
        schema_info_ptr->module_set = module_set;
        cdada_list_traverse(reg_list, libyangpush_trav_list_register_schema, schema_info_ptr);
        printf("=>schema id %d\n", schema_info_ptr->schema_id);
        cdada_map_traverse(module_set, libyangpush_trav_clear_module_set_map, NULL);
        cdada_map_destroy(module_set);
        cdada_list_destroy(reg_list);
        free(schema_info_ptr);
    }
    return;
}

int main()
{
    /* Connect to NETCONF server */
    struct ly_ctx *module_context;
    if (ly_ctx_new(YANG_MODULE_CONTEXT_SEARCH_PATH, 1, &module_context) != LY_SUCCESS) {
        fprintf(stderr, "%s", "context creation error\n");
        exit(0);
    }
    struct nc_session *session = NULL;
    if (connect_netconf(&module_context, &session)) {
        exit(0);
    }

    /* Load example message */
    char *msg = load_file_from_disk(PATH_TO_EXAMPLE_MESSAGE);
    xmlNodePtr subscription_list_ptr;
    int sub_id;

    if (validate_message_structure((void*)msg, &subscription_list_ptr, &sub_id) == MESSAGE_STRUCTURE_INVALID) {
        exit(1);
    }

    cdada_map_t *subscription_filters = cdada_map_create(int);
    libyangpush_parse_subscription_filter(subscription_list_ptr, subscription_filters);
    if (cdada_map_empty(subscription_filters) != 1) {
        cdada_map_traverse(subscription_filters, trav_create_schema, module_context);
    }
    cdada_map_traverse(subscription_filters, libyangpush_trav_clear_subscription_filter_map, NULL);
    cdada_map_destroy(subscription_filters);
    free(msg);
    xmlFreeNode(subscription_list_ptr);
    ly_ctx_destroy(module_context);
    nc_session_free(session, NULL);
    nc_client_destroy();
}