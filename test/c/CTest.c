#include <openPMD/binding/c/openPMD.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void test_write(const char *const filePath)
{
    openPMD_Series *const series =
        openPMD_Series_new_serial(filePath, openPMD_Access_CREATE, NULL);
    assert(openPMD_Series_has_value(series));
    openPMD_Series_delete(series);
}

void test_read(const char *const filePath)
{
    openPMD_Series *const series =
        openPMD_Series_new_serial(filePath, openPMD_Access_READ_ONLY, NULL);
    assert(openPMD_Series_has_value(series));
    openPMD_Series_delete(series);
}

int main(int argc, char **argv)
{
    printf("openPMD C language binding test\n");

    const char *const filePath = "/tmp/file.json";

    printf("Write file \"%s\"...\n", filePath);
    test_write(filePath);

    printf("Read file \"%s\"...\n", filePath);
    test_read(filePath);

    printf("Done.\n");
    return 0;
}
