#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "parser.h"
#include "json.h"
#include "tests.c"

#ifdef _WIN32
   #include <io.h>
   #define isatty _isatty
   #define fileno _fileno
#else
   #include <unistd.h>
#endif

typedef struct {
    bool compact;
    bool help;
    bool input;
    bool output;
    bool query;
    bool run_tests;
} Flags;

char input_filename_buffer[1000];
char output_filename_buffer[1000];
char query_buffer[1000];
char *json_str = "";

void print_help() {
    printf("\n");
    printf("Usage: printj [OPTION] json_string\n");
    printf("\n");
    printf("Options:\n");
    printf("\n");
    printf("    -c, --compact                   Format the output as compactly as possible\n");
    printf("    -h, --help                      Print this help message and exit\n");
    printf("    -i, --input FILE                Specify the input file\n");
    printf("    -o, --output FILE               Specify the output file\n");
    printf("    -q, --query KEY_OR_INDEX        Query the object or array and output the result\n");
    printf("\n");
    printf("\n");
    printf("Example usage:\n");
    printf("\n");
    printf("    printj '[1,2,3,4,5]'\n");
    printf("    printj -c -i in.json -o out.json\n");
    printf("    echo '{\"asdf\": 1, \"hello\": \"world\", \"abcd\": 2.0}' | printj -q 'hello'\n");
    printf("    printj -q 0 '[[true,false,null],2,3]' | printj -q 1\n");
    printf("\n");
}

void print_help_and_exit(int exit_code) {
    print_help();
    exit(exit_code);
}

Flags parse_args(int argc, char *argv[]) {
    Flags result = {false, false, false, false, false};

    for(int i = 1; i < argc - 1; i++) { // last argument might not be flag, e.g. printj -c [1,2,3,4,5]
        char *arg = argv[i];
        bool compact = strcmp("-c", arg) == 0 || strcmp("--compact", arg) == 0;
        bool help = strcmp("-h", arg) == 0 || strcmp("--help", arg) == 0;
        bool input = strcmp("-i", arg) == 0 || strcmp("--input", arg) == 0;
        bool output = strcmp("-o", arg) == 0 || strcmp("--output", arg) == 0;
        bool query = strcmp("-q", arg) == 0 || strcmp("--query", arg) == 0;
        bool run_tests = strcmp("--run-tests", arg) == 0;

        if(!compact && !help && !input && !output && !query && !run_tests)
            print_help_and_exit(1);
        assert(compact ^ help ^ input ^ output ^ query ^ run_tests);

        // don't accept multiple of these flags
        if(input && result.input)
            print_help_and_exit(1);
        if(output && result.output)
            print_help_and_exit(1);
        if(query && result.query)
            print_help_and_exit(1);

        // copy the argument to it's appropriate buffer
        if(input || output || query) {
            char *dst_buffer = input ? input_filename_buffer 
                : output ? output_filename_buffer
                : query_buffer;
            // grab flag argument, e.g. get "in.json" from printj -i in.json
            i++;
            if(i < argc)
                strcpy(dst_buffer, argv[i]);
            else // flag was not given required arg, e.g. printj -i
                print_help_and_exit(1);
            // don't want to confuse arg as json string, e.g. don't want
            // to confuse "in.json" as input string for printj -i in.json
            if(i == argc - 1)
                json_str = NULL;
        }

        result.compact = result.compact || compact;
        result.help = result.help || help;
        result.input = result.input || input;
        result.output = result.output || output;
        result.query = result.query || query;
        result.run_tests = result.run_tests || run_tests;
    }

    // don't want to confuse flags as input strings if they are last
    // e.g. don't want to confuse "-h" as input for "printj -h"
    if(strcmp(argv[argc - 1], "-h") == 0 || strcmp(argv[argc - 1], "--help") == 0) {
        result.help = true;
        json_str = NULL;
    }
    else if(strcmp(argv[argc - 1], "-c") == 0 || strcmp(argv[argc - 1], "--compact") == 0) {
        result.compact = true;
        json_str = NULL;
    }
    else if(strcmp(argv[argc - 1], "--run-tests") == 0) {
        result.run_tests = true;
        json_str = NULL;
    }
    else
        json_str = json_str == NULL ? NULL : argv[argc - 1];

    return result;
}

JsonValue query_obj_or_arr(JsonValue value) {
    JsonValue result;
    bool found = false;
    if(value.type == JSON_OBJECT) {
        JsonObject *obj = value.data.object;
        for(unsigned int i = 0; i < obj->length; i++) {
            if(strcmp(obj->keys[i], query_buffer) == 0) {
                found = true;
                result = obj->values[i];
            }
        }
    }
    else if(value.type == JSON_ARRAY) {
        errno = 0;
        char *endptr;
        long target_index = strtol(query_buffer, &endptr, 10);
        if(errno == 0 || endptr != query_buffer) {
            JsonArray *arr = value.data.array;
            if(0 <= target_index && target_index < (long)arr->length) {
                found = true;
                result = arr->values[target_index];
            }
        }
    }

    if(!found)
        exit(0);
    else
        return result;
}

void run_tests() {
    test_string();
    test_numbers();
    test_booleans();
    test_null();
    test_unnested_array();
    test_unnested_object();
    test_nested_array();
    test_nested_object();
    test_empty_string();
    test_empty_array();
    test_empty_object();
    test_untrimmed_input_string();
    test_string_escaped_characters();
    test_unclosed_string();
    test_unclosed_array();
    test_unclosed_object();
    test_array_trailing_comma();
    test_object_trailing_comma();
    test_object_key_unclosed();
    test_array_missing_comma();
    test_object_missing_comma();
    test_object_missing_colon();
    test_empty_input_string();
}

char *read_piped_stdin() {
    unsigned int length = 0;
    unsigned int capacity = 512;
    char *buffer = malloc(sizeof(char) * capacity);

    if(buffer == NULL) {
        fputs("unrecoverable error: cannot allocate memory to read from piped input\n", stderr);
        exit(1);
    }

    if(!isatty(fileno(stdin))) { // input is piped or redirected
        char c = getchar();
        while(c != EOF) {
            if(length >= capacity) {
                capacity = 2 * length;
                buffer = realloc(buffer, sizeof(char) * capacity);
                if(buffer == NULL) {
                    fputs("unrecoverable error: cannot allocate memory to read from piped input\n", stderr);
                    exit(1);
                }
            }

            buffer[length] = c;
            length++;
            c = getchar();
        }

        // in case length == capacity
        buffer = realloc(buffer, sizeof(char) * (capacity + 1));
        if(buffer == NULL) {
            fputs("unrecoverable error: cannot allocate memory to read from piped input\n", stderr);
            exit(1);
        }
        buffer[length] = '\0';
    }
    else {
        buffer[0] = '\0';
    }

    return buffer;
}

int main(int argc, char *argv[]) {
    char *stdin_buffer = read_piped_stdin();

    if(argc <= 1 && strlen(stdin_buffer) == 0) {
        print_help();
        return 1;
    }

    Flags flags = parse_args(argc, argv);
    json_str = strlen(stdin_buffer) > 0 ? stdin_buffer : json_str;
    
    if(flags.run_tests) {
        run_tests();
        return 0;
    }
    if(flags.help) {
        print_help();
        return 0;
    }

    EitherErrJson *either;

    if(flags.input)
        either = json_load(input_filename_buffer);
    else if(json_str != NULL)
        either = json_loads(json_str);
    else {
        print_help();
        return 1;
    }

    if(is_error(either)) {
        printf("error: %s\n", parser_error_reason(either));
        deinit_json(either);
        return 1;
    }
    JsonValue value = unwrap_json_or_halt(either);

    if(flags.query) {
        value = query_obj_or_arr(value);
    }

    if(flags.output) {
        FILE *file = fopen(output_filename_buffer, "w");
        if(flags.compact)
            json_dump(value, file);
        else
            json_pdump(value, file);
        fclose(file);
    }
    else {
        if(flags.compact)
            json_println(value);
        else
            json_pprintln(value);
    }

    deinit_json(either);
    free(stdin_buffer);
    return 0;
}
