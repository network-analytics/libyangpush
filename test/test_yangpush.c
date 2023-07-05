#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <cmocka.h>
#include "libyangpush.h"

static void test_add(void** state){
    (void) state; /* unused */

	assert_int_equal(add(3, 3), 6);
	assert_int_equal(add(3, -3), 0);
    return;
}

int main(void){
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_add),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}