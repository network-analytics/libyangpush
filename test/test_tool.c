#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <cmocka.h>
#include "tool.h"

static void test_subtract(void** state){
    (void) state; /* unused */

	assert_int_equal(subtract(3, 3), 0);
	assert_int_equal(subtract(3, -3), 6);
    return;
}

int main(void){
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_subtract),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}