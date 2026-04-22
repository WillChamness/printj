# Usage

```
Usage: printj [OPTION] json_string

Options:

    -c, --compact                   Format the output as compactly as possible
    -h, --help                      Print this help message and exit
    -i, --input FILE                Specify the input file
    -o, --output FILE               Specify the output file
    -q, --query KEY_OR_INDEX        Query the object or array and output the result


Example usage:

    printj '[1,2,3,4,5]'
    printj -c -i in.json -o out.json
    echo '{"asdf": 1, "hello": "world", "abcd": 2.0}' | printj -q 'hello'
    printj -q 0 '[[true,false,null],2,3]' | printj -q 1
```

# Installation

Go to the releases page, download the archive file for Windows or Linux, extract, and put the executable somewhere in the PATH for your user
