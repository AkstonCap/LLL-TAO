# Coding Style Guide

This library and framework is designed to make your life easier. As such, we want to extend the same services to those that develop the framework. Following are the coding style guides, to ensure that you can learn from the last, and make very clean, well documented code that can be easily maintainable.


## Variable Naming Declarations

The variable names have a prefix that defines the type. This helps us not lose time in searching back to the declaration of said object, keeping the coding process efficient and precise.


### Primitive Types

* (n)Variable ex. nVar - (n) denotes an integer (signed / unsigned).
* (d)Variable ex. dVar - (d) denotes a floating point number.
* (f)Variable ex. fVar - (f) denotes a boolean flag.
* (p)Variable ex. pVar - (p) denotes the variable is a pointer.
* (s)Variable ex. sVar - (s) denotes the variable is static.
* {j}Variable ex. jVar - (j) denotes the variable is a json container.
* (r)Variable ex. rVar - (r) denotes this variable is a reference.

### STL Types

* (str)Variable ex. strVar - (str) denotes the type is a std::string.
* (v)Variable   ex. vArr     - (v)   denotes the type is a std::vector.
* (map)Variable ex. mapVar - (map) denotes the type is a std::map
* (set)Variable ex. setVar - (set) denotes the type is a std::set

### TAO Types
* (obj)Variable ex. objVar - (obj) denotes the type is TAO::Register::Object, if single word use full typename i.e. object
* (ste)Variable ex. steVar - (ste) denotes the type is TAO::Register::State, if single word use full typename i.e. state
* (txn)Variable ex. txnVar - (txn) denotes the type is TAO::Ledger::Transaction, if single word reduced typename i.e. tx

## Namespaces

Always use namespaces to keep the code well ordered and organized. The guide to using namespaces is based around the folder the source files are in. Always declare a new namespace every time you increment a folder, to ensure it is easy to find objects, and there are no duplicate declarations.


## Indentation and Formatting

This section involves how to format the code due to carriage return

* Avoid K&R style brackets. Put opening and closing brackets on their own lines.

* Use tab length 4 and replace tabs with spaces

* We use a maximum line length of 132 characters


## Types to avoid

There are certain types that cause more problems than they solve. Following is a list of types to be warned of using.

* Avoid floating points in objects and ALL consensus critical code. Always use cv::softfloat or cv::softdouble when floating points
are required.


## Security Precautions

* memcpy - this is known to have buffer overflow attack vulnerabilities. Use std::copy instead of memcpy in all instances.
