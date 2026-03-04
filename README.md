# Evaluator

An open-source C++ solution analyzer for the Romanian Olympiad of Informatics, developed to provide immediate feedback on solution performance.
(The only reason this exists is because I didn't want to wait 3 days for the problems to appear on a platform)

### Usage

```bash
evaluator </path/to/main.cpp> <target-directory>

```

---

### Configuration Requirements

**Target Directory**
The `target-directory` must contain the test cases (both `.in` and `.out` files). This directory should be named after the expected input and output filenames required by the problem.

* **Example:** For a problem named `unuzero`, the program expects `unuzero.in` as input and produces `unuzero.out` as output. In this case, your directory containing the test cases should be named `unuzero`.

**Test Case Naming Convention**
Inside the target directory, files must follow a consistent ID-based naming scheme:

* `test-case-id.in`: The actual input file provided to the program.
* `test-case-id.out`: The expected output file used for validation.

---

### Operational Details

* **Automation:** The evaluator automatically compiles the source code, matches input/output pairs by their ID, and handles the necessary file renaming to match the problem's requirements.
* **Safety:** Execution is wrapped in a timeout to prevent infinite loops from hanging the system.
* **Validation:** The analyzer performs a comparison of the produced output against the expected result, ignoring trailing whitespace to ensure fair grading.
