#include <nc_client.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define YANG_SCHEMA_REGISTRY "http://127.0.0.1:5000/subjects"
#define SUBJECT_PREFIX "subject_"

#define YANG_MODULE_CONTEXT_SEARCH_PATH "../example/modules"

#define SSH_ADDRESS "127.0.0.1"
#define SSH_PORT 8830

/* SSH 'password' authentication */
#define SSH_USERNAME "root"
#define SSH_PASSWORD "admin"

/* SSH 'public key' authentication */
#define SSH_PUBLIC_KEY "~/.ssh/id_rsa.pub"
#define SSH_PRIVATE_KEY "~/.ssh/id_rsa"

#define PATH_TO_EXAMPLE_MESSAGE "../resources/push-update.xml"