#include <yangpush/libyangpush.h>
#include <libyang/libyang.h>
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

int main()
{
    /* Load example message */
    char *msg = load_file_from_disk(PATH_TO_EXAMPLE_MESSAGE);
    xmlNodePtr subscription_list_ptr;
    int sub_id;


    if (validate_subscription_started_structure((void*)msg, &subscription_list_ptr, &sub_id) == MESSAGE_STRUCTURE_INVALID) {
        exit(1);
    }

    char *filter = NULL;
    
    libyangpush_parse_xpath(subscription_list_ptr, &filter);
    if (strchr(filter, ':') != NULL) {
        filter = libyangpush_pattern_match("([^:]*)$", filter);
    }


#ifdef DEBUG
    printf("%s\n", filter);
#endif


    struct module_version *module_version = parse_yangpush_msg_model_revision_by_keyval(msg, "huawei-debug");

#ifdef DEBUG
    printf("module name: %s\n", module_version->module_name);
    printf("module version: %s\n", module_version->revision);
    printf("module version label: %s\n", module_version->revision_label);
#endif


    free(msg);
    xmlFreeNodeList(subscription_list_ptr);
}