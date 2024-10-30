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
void element_list_print_trav(const cdada_list_t *list, const void* k, void* opaque) 
{
    (void)list;
    (void)opaque;
 	char* key = (char*)*(void**)k;
    printf("%s \n", key); 
}



int main(int argc, char* argv[])
{
    /* Load example message */
    char *msg = load_file_from_disk(PATH_TO_EXAMPLE_YANGLIB_MSG);

    if(argc != 2)
    {
        printf("[ERROR] wrong number of input param");
    }

    cdada_list_t *augmentation_list = parse_yanglib_msg(msg, argv[1], "augmented-by");
    cdada_list_t *deviation_list = parse_yanglib_msg(msg, argv[1], "deviations");

    printf("augmentation list size %d\n", cdada_list_size(augmentation_list));
    printf("deviation list size %d\n", cdada_list_size(deviation_list));

    printf("augmented-by module:\n");
    cdada_list_traverse(augmentation_list, &element_list_print_trav, NULL);

    free(msg);
}