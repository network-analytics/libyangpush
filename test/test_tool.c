#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <cmocka.h>
#include "tool.h"

static void test_djb2(void** state){
    assert_int_equal(djb2("helloworld"), 0x72711934fffdad81);
    assert_int_not_equal(djb2("libyangpush"), 0);
    return;
}

int main(void){
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_djb2),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}