#include <yangpush/libyangpush.h>
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

void element_list_clear_trav(const cdada_list_t *list, const void* k, void* opaque) 
{
    (void)list;
    (void)opaque;
 	char* key = (char*)*(void**)k;
    free(key);
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
    // nc_client_ssh_add_keypair(SSH_PUBLIC_KEY, SSH_PRIVATE_KEY);
    nc_client_ssh_set_auth_pref(NC_SSH_AUTH_PASSWORD, 4);

    *session = nc_connect_ssh(SSH_ADDRESS, SSH_PORT, *ctx);
    if (!(*session)) {
        rc = 1;
        fprintf(stderr, "%s", "Couldn't connect to the server\n");
    }

    return rc;
}

/* return 0 if sucessfully sent, otherwise 1*/
int receive_rpc(struct nc_session *session, struct lyd_node **envp, struct lyd_node **op, 
        int msg_id, NC_RPC_TYPE rpc_type, struct nc_rpc *rpc, signed int timeout){
    int r = 0, rc = 0;

    /* receive the server's reply with the expected message ID
     * as separate rpc-reply NETCONF envelopes and the parsed YANG output itself, if any */
    r = nc_recv_reply(session, rpc, msg_id, timeout, envp, op);

    switch(r){
        case NC_MSG_REPLY:
#ifdef debug
            printf("[RECEIVE_RPC]success\n");
#endif
            break;
        case NC_MSG_WOULDBLOCK:
#ifdef debug
            printf("[RECEIVE_RPC]Timeout elapsed\n");
#endif
            break;
        case NC_MSG_ERROR:
#ifdef debug
            printf("[RECEIVE_RPC]Reading fail\n");
#endif
            break;
        case NC_MSG_NOTIF:
#ifdef debug
            printf("[RECEIVE_RPC]Notifications msg is read, call nc_recv_notif() instead to get the notif\n");
#endif
            r = nc_recv_notif(session, timeout, envp, op);
            if (r == 0) {
                return 0;
            }
            break;
        case NC_MSG_REPLY_ERR_MSGID:
#ifdef debug
            printf("[RECEIVE_RPC]Reply with missing or wrong msg-id\n");
#endif
            return 0;
            break;
        default:
            break;
    }
    cleanup:
    return rc;
}

int generate_get_yanglib_rpc(int msg_id, char *module_name, struct nc_session *session, struct nc_rpc **rpc) {
    char get_xpath[100];
    int rc = 0;

    sprintf(get_xpath, "/ietf-yang-library:yang-library/module-set/module[name='%s']/*", module_name);
    printf("xpath: %s\n", get_xpath);
    *rpc = nc_rpc_get(get_xpath, NC_WD_UNKNOWN, NC_PARAMTYPE_CONST);

    /* if the rpc is successfully generated */
    if (!*rpc) {
        return 0;
    }

    /* send the RPC on the session and remember NETCONF message ID */
    rc = nc_send_rpc(session, *rpc, 100, &msg_id);
    if (rc != NC_MSG_RPC) {
        return 0;
    }

    return msg_id;
}

/* Send the get-schema request */
char* send_and_receive_rpc(struct nc_session *session, char* module_name){
    int msg_id = 0, rc = 0, r= 0 ;
    struct nc_rpc *rpc = NULL;
    struct lyd_node *envp = NULL, *op;
    char* reply = NULL;

    //send get-schema rpc
    msg_id = generate_get_yanglib_rpc(msg_id, module_name, session, &rpc);
    if(!rpc)
        printf("rpc incorrect");

    //receive the reply from netconf server
    rc = receive_rpc(session, &envp, &op, msg_id, NC_RPC_GET, rpc, 1000);

    if (!op) {
        r = lyd_print_file(stdout, envp, LYD_XML, 0);
        r = lyd_print_mem(&reply, envp, LYD_XML, 0);
    } else {
        r = lyd_print_file(stdout, op, LYD_XML, 0);
        if (r) {
            goto cleanup;
        }
        r = lyd_print_file(stdout, envp, LYD_XML, 0);
        r = lyd_print_mem(&reply, op, LYD_XML, 0);
    }

    cleanup:
    lyd_free_all(envp);
    lyd_free_all(op);
    nc_rpc_free(rpc);
    return reply;
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
        cdada_list_rtraverse(reg_list, libyangpush_trav_list_register_schema, schema_info_ptr);
#ifdef DEBUG
        printf("=>schema id %d\n", schema_info_ptr->schema_id);
#endif
        cdada_map_traverse(module_set, libyangpush_trav_clear_module_set_map, NULL);
        cdada_map_destroy(module_set);
        cdada_list_destroy(reg_list);
        free(schema_info_ptr);
    }
    return;
}

void element_list_print_trav(const cdada_list_t *list, const void* k, void* opaque) 
{
    (void)list;
    (void)opaque;
 	char* key = (char*)*(void**)k;
    printf("%s \n", key); 
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


    if (validate_subscription_started_structure((void*)msg, &subscription_list_ptr, &sub_id) == MESSAGE_STRUCTURE_INVALID) {
        exit(1);
    }

    char *filter = NULL;
    libyangpush_parse_xpath(subscription_list_ptr, &filter);
#ifdef DEBUG
    printf("filter %s\n", filter);
#endif
    struct lys_module* subscribed_module = ly_ctx_get_module(module_context, filter, NULL);
    char* yanglib = send_and_receive_rpc(session, filter);
#ifdef DEBUG
    printf("yanglib \n %s\n", yanglib);
#endif

    cdada_list_t *augmentation_list = parse_yanglib_msg(yanglib, filter, "augmented-by");
    cdada_list_t *deviation_list = parse_yanglib_msg(yanglib, filter, "deviations");

    printf("augmentation list size %d\n", cdada_list_size(augmentation_list));
    printf("deviation list size %d\n", cdada_list_size(deviation_list));

    printf("augmented-by module:\n");
    cdada_list_traverse(augmentation_list, &element_list_print_trav, NULL);

cleanup:
    free(msg);
    free(filter);
    free(yanglib);
    cdada_list_traverse(augmentation_list, &element_list_clear_trav, NULL); 
    cdada_list_traverse(deviation_list, &element_list_clear_trav, NULL); 
    cdada_list_destroy(augmentation_list);
    cdada_list_destroy(deviation_list);
    xmlFreeNodeList(subscription_list_ptr);
    ly_ctx_destroy(module_context);
    nc_session_free(session, NULL);
    nc_client_destroy();
}