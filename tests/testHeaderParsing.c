#include <assert.h>
#include "CuTest.h"

void TestDetectingSimpleHeader(CuTest* tc){

}

CuSuite* CuGetSuite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestDetectingSimpleHeader);
    return suite;
}
