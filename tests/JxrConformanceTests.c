/* Self-contained conformance runner for the managed-port reference profile. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "JxrManagedBitIO.h"
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#endif

typedef int (*JxrTest)(void);
typedef struct { const char* name; JxrTest run; } JxrTestCase;

static int file_contains(const char* path, const char* text)
{
    FILE* f = fopen(path, "rb");
    char data[4096]; size_t n;
    if (!f) return 0;
    n = fread(data, 1, sizeof(data) - 1, f); fclose(f);
    data[n] = 0;
    return strstr(data, text) != NULL;
}

static int files_equal(const char* left, const char* right)
{
    FILE* a = fopen(left, "rb"), *b = fopen(right, "rb");
    int ca, cb;
    if (!a || !b) { if (a) fclose(a); if (b) fclose(b); return 0; }
    do { ca = fgetc(a); cb = fgetc(b); } while (ca == cb && ca != EOF);
    fclose(a); fclose(b); return ca == cb;
}

static int test_smoke(void) { return 1; }

static int test_bit_io_vectors(void)
{
    U8 data[8] = {0}; U32 value; JxrBitWriter writer; JxrBitReader reader;
    JxrBitWriterInit(&writer, data, sizeof(data));
    if (!JxrBitWriterWrite(&writer, 5, 3) || !JxrBitWriterWrite(&writer, 17, 5) ||
        !JxrBitWriterWrite(&writer, 0xabcd, 16) || !JxrBitWriterWrite(&writer, 15, 4) ||
        !JxrBitWriterFlush(&writer) || JxrBitWriterBytes(&writer) != 4 ||
        data[0] != 0xb1 || data[1] != 0xab || data[2] != 0xcd || data[3] != 0xf0) return 0;
    JxrBitReaderInit(&reader, data, 4);
    return JxrBitReaderRead(&reader, 3, &value) && value == 5 &&
        JxrBitReaderRead(&reader, 5, &value) && value == 17 &&
        JxrBitReaderRead(&reader, 16, &value) && value == 0xabcd &&
        JxrBitReaderRead(&reader, 4, &value) && value == 15 &&
        !JxrBitReaderRead(&reader, 16, &value);
}

static int test_minimal_fixture(void)
{
    return files_equal("minimal-profile/minimal-gray-16x16.bmp",
                       "minimal-profile/minimal-gray-16x16-restored.bmp") &&
           files_equal("minimal-profile/minimal-gray-16x16.jxr",
                       "minimal-profile/trace/encoder-bitstream.jxr") &&
           files_equal("minimal-profile/trace/encoder-bitstream.jxr",
                       "minimal-profile/trace/decoder-bitstream.jxr");
}

static int test_minimal_round_trip(void)
{
    int result;
    result = system("cmd /c if not exist tests\\work mkdir tests\\work & "
        "jxrencoderdecoder\\Release\\JXREncApp\\x64\\JXREncApp.exe -i minimal-profile\\minimal-gray-16x16.bmp -o tests\\work\\minimal.jxr -c 2 -d 0 -q 1 -l 0 -f -X tests\\work\\trace & "
        "jxrencoderdecoder\\Release\\JXRDecApp\\x64\\JXRDecApp.exe -i tests\\work\\minimal.jxr -o tests\\work\\minimal.bmp -c 2 -a 0 -p 0 -X tests\\work\\trace");
    return result == 0 &&
        files_equal("minimal-profile/minimal-gray-16x16.bmp", "tests/work/minimal.bmp") &&
        files_equal("minimal-profile/minimal-gray-16x16.jxr", "tests/work/minimal.jxr") &&
        files_equal("tests/work/trace/encoder-bitstream.jxr", "tests/work/trace/decoder-bitstream.jxr") &&
        file_contains("tests/work/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_count\": 10") &&
        file_contains("tests/work/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_count\": 214") &&
        file_contains("tests/work/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_count\": 1958");
}

static int test_bit_ranges(void)
{
    return file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_start\": 1320") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_end\": 1330") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_start\": 1330") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_end\": 1544") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_start\": 1544") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_end\": 3502");
}

static int test_entropy_trace(void)
{
    return file_contains("minimal-profile/trace/decoder-mb-000-000-after_ac_prediction.json", "\"cbp\": 65535") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-dc.json", "\"bit_count\": 10") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-lp.json", "\"bit_count\": 214") &&
           file_contains("minimal-profile/trace/encoder-mb-000-000-bitstream-hp.json", "\"bit_count\": 1958");
}

int main(int argc, char** argv)
{
    size_t i; int failed = 0;
    JxrTestCase tests[] = {
        { "smoke", test_smoke },
        { "bit_io_vectors", test_bit_io_vectors },
        { "minimal_fixture", test_minimal_fixture },
        { "minimal_round_trip", test_minimal_round_trip },
        { "bit_ranges", test_bit_ranges },
        { "entropy_trace", test_entropy_trace }
    };
    const char* selected = argc == 2 ? argv[1] : NULL;
#ifdef _WIN32
    /* The solution starts this executable from jxrencoderdecoder/. */
    if (_access("minimal-profile", 0) != 0) _chdir("..");
#endif
    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        if (selected && strcmp(selected, tests[i].name)) continue;
        if (tests[i].run()) printf("PASS %s\n", tests[i].name);
        else { printf("FAIL %s\n", tests[i].name); failed = 1; }
    }
    if (selected && !failed) {
        int found = 0; for (i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i) if (!strcmp(selected, tests[i].name)) found = 1;
        if (!found) { printf("FAIL unknown test %s\n", selected); failed = 1; }
    }
    return failed;
}
