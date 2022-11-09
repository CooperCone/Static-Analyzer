# Static Analyzer

## Description

This is a C programming language static analyzer tailored for the BARR C coding standards. It is a command line utility that will detect issues and report them to the user. It is not a linter, so it will not fix the problems for you. The goal of this project is to be able to detect most BARR C rules as well as some other security rules such as NULL pointer dereferences.

## Install

Currently, this project only works for Linux. I also haven't set up a configure and install script, so this is going to be a scuffed install that will be changed in the future.

1. Clone the repository to a directory of your choice.

    ```
    cd <path_to_repo>
    git clone <repository path> static-analyzer
    ```

2. Build the project

    ```
    cd static-analyzer
    make
    ```

3. Add a symlink for the analyzer executable to a bin path of your choice.

    ```
    cd /usr/bin/
    ln -s <path_to_repo>/analyzer /usr/bin/analyzer
    ```

## Usage

Currently, this executable only takes command line args for the files you want to analyze. It can analyze as many files as you want to give it.

However, this project will only analyze files that are already preprocessed.

Let's say that you want to analyze a file: `src.c`

1. First, preprocess your file

    `cpp src.c > src.i`

    **i** is the canonical extension for a preprocessed file

2. Then, run the analyzer

    `analyzer src.i`

## Output

After running your analyzer, you may see some output that looks like this:

`Error: test/helloWorld.c:3 - 1.3.b - Open curly bracket must be alone on its line`

This message is broken up into a couple parts:

1. Error: this tells us that it's an error. The idea is that I will notify you how pressing an issue is. If there's a rule violation that I'm not confident about, I'll tell you its a warning. Currently, this isn't implemented, so all issues will be Errors.

2. File: This tells you the file and line that occured. In the example, the file is **test/helloWorld.c** and the error is on line **3**

3. Rule Violated: This shows what BARR C rule was violated. In this case, it was rule **1.3.b**

4. Message: Ideally, this will give you a descriptive message about the issue. Some are better than others.

## Configuration

This project has a configuration parser that is currently a work in progress. Right now, there are only two configurations that you'll want to modify. The config file is stored in config/config.cfg. Ideally, I'd like to let you specify multiple config files and switch them out, but I haven't implemented that yet.

### Base Format

My configuration format is similar to JSON and has 3 types of values:
1. String - Base value

    `myString`

2. Map - Key : Value pair

    `myKey: <Value>`

3. List - Comma separated list of values

    ```
    [
        <Value1>,
        <Value2>
    ]
    ```

A base configuration file is one or more values

These rules can be combined together. Here is an example configuration file:

```
myKey: {
    [ x, y, z ]
}

myOtherKey: {
    [
        name: data,
        name2: data2,
        [ stuff1, stuff2, stuff3 ]
    ]
}
```

### Specific Rules

1. Let's say that you want to ignore a specific rule. Maybe you don't like how I implemented the rule. Maybe you just want to stick it to BARR C.

    The rule you want to modify has the following format:

    `Map<ignoreRules, List<String>>`

    An example is below:

    ```
    ignoreRules: [
        1.3.b,
        1.2.a
    ]
    ```

    This example will ignore all violations for rules 1.3.b and 1.2.a.

2. Let's say that you are receiving a lot of errors in code that you didn't write.

    The rule you want to modify has the following format:

    `Map<ignorePaths, List<String>>`

    An example is below:

    ```
    ignorePaths: [
        /usr/include/,
        /usr/lib
    ]

    This example will ignore any file in /usr/include or /usr/lib.

    The path can be as specific or general as you want. You can even specify a specific file: **/path/to/file/src.c**

## Issues

There is an **issues** file that has a list of known issues with the analyzer. If you have any other issues, just let me know or maybe add an issue on github.

## Currently Implemented Rules

I have a file called **rules** that shows all the rules I have implemented and all the ones I plan to implement. If a rule has an asterisk in front of it, then I've implemented it.
