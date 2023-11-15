# Test File directory
This directory should contain all files your tests want.
The directory itself will be in the same location as the executable being tested
Tests themselves should be put into the `test_scripts` directory and should return 0 on success and nonzero on failure.

Be sure to edit or remove example.sh, otherwise you will fail the first test.

## Testing Method
The `test_scripts` and `test_files` directories will be copied into a directory with the executable to be tested, named `httpserver`.
Each file in `test_scripts` will be tested by running `bash test_scripts/<filename>`.
The return code will be checked to determine success, 0 for successs and nonzero for failure.

All tests should pass on the resources binary.
Any tests that fail on the resources binary will not be run against any other binaries.
