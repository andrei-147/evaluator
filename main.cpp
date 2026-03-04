#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <utility>

bool streams_are_equal_ignore_trailing(std::ifstream& f1, std::ifstream& f2) {
    auto get_trimmed_content = [](std::ifstream& f) {
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(f, line)) {
            // Remove trailing whitespace from the line
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), line.end());

            lines.push_back(line);
        }

        // Remove completely empty lines from the end of the file 
        // (handles trailing newlines/spaces at the very end of the file)
        while (!lines.empty() && lines.back().empty()) {
            lines.pop_back();
        }
        return lines;
    };

    return get_trimmed_content(f1) == get_trimmed_content(f2);
}

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // checking for proper arguments
    if (argc < 3) {
        std::cerr << "[ERROR] Usage: evaluator </path/to/main.cpp> <target-directory>\n";
        return -1;
    }

    std::string main = argv[1];
    fs::path main_path = fs::absolute(main);
    if (main_path.extension() != ".cpp") {
    std::cerr << "[ERROR] " << main_path << " is not a C++ source file!\n";
    return -1;
    }
    std::string name = argv[2];

    // generating the path to the target folder containing all test cases
    fs::path full_path = fs::absolute(name);
    // checking that the path points to a valid file
    if (fs::exists(fs::path(full_path))) {
        std::cout << "Found path!\n";
        std::cout << full_path << '\n';
    } else {
        std::cerr << "[ERROR] Directory does not exist!\n";
        return -1;
    }

    // checks that the path is a directory, not a file
    if (fs::is_directory(fs::path(full_path))) {
        std::cout << "path is directory!\n";
    } else {
        std::cerr << "[ERROR] Target is a regular file, not a directory!\n";
        return -1;
    }

    // gets all files in target directory
    auto files = fs::directory_iterator(full_path);
    if (fs::is_empty(full_path)) {
        std::cerr << "[ERROR] Target contains no test cases!\n";
        return -1;
    }

    // declaring stuff for future use
    fs::path invalid_path = fs::path(); // an invalid path to differentiate in the hashmap
    std::unordered_map<std::string, std::pair<fs::path, fs::path>> iofiles; // hashmap of pairs of input and output file paths indexed by the id of the file

    for (auto &entry : files) { // iterating through files in target directory
        std::string entry_name = entry.path().filename();

        if (entry_name.ends_with(".in")) {
            entry_name = entry_name.substr(0, entry_name.size() - 3);
            if (!iofiles.contains(entry_name)) iofiles[entry_name].second = invalid_path;
            iofiles[entry_name].first = entry.path();

        } else if (entry_name.ends_with(".out")) {
            entry_name = entry_name.substr(0, entry_name.size() - 4);
            if (!iofiles.contains(entry_name)) iofiles[entry_name].first = invalid_path;
            iofiles[entry_name].second = entry.path();

        } else {
            std::cerr << "[ERROR] Invalid file type for test case file: " << entry_name << '\n';
        }
    }
    if (iofiles.empty()) {
        std::cerr << "[ERROR] Failed to load test cases into memory: No valid pairs found!\n";
        return -1;
    }

    fs::remove(fs::current_path() / "exec");
    std::string compile_command = "g++ -o exec " + main_path.string() + " -std=c++20";
    int errc = std::system(compile_command.c_str());
    if (errc) {
        std::cerr << "[ERROR] Failed to compile file [[" << main << "]], make sure you have spelled it correctly!\n";
        return -1;
    }

    fs::path local_in = fs::current_path() / (name + ".in");
    fs::path local_out = fs::current_path() / (name + ".out");

    for (auto& [key, value] : iofiles) {
        if (value.first.empty() || value.second.empty()) {
            std::cerr << "[SKIP] Test case [[" << key << "]] is missing either .in or .out file.\n";
            continue;
        }

        try {
            // 2. Sync the test case to the local folder
            // Use overwrite_existing so the loop doesn't crash on test case #2
            fs::copy(value.first, local_in, fs::copy_options::overwrite_existing);

            // 3. Delete old output so we don't accidentally validate old data if the current run crashes
            if (fs::exists(local_out)) fs::remove(local_out);

            auto start = std::chrono::high_resolution_clock::now();
            // 4. Run with timeout
            int status = std::system("timeout 1s ./exec");
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

            // 5. Evaluate results
            if (WIFEXITED(status)) {
                int err_code = WEXITSTATUS(status);

                if (err_code == 124) {
                    std::cout << "[TIMEOUT] [[" << key << "]] ( > 1000 ms)\n";
                } else if (err_code != 0) {
                    std::cout << "[RUNTIME ERROR] [[" << key << "]] (Exit Code: " << err_code << ")\n";
                } else {
                    // Success! Now compare files
                    std::ifstream res_stream(local_out);
                    std::ifstream exp_stream(value.second);

                    if (!res_stream.is_open()) {
                        std::cout << "[NO OUTPUT] [[" << key << "]] Program failed to create " << name << ".out\n";
                    } else if (streams_are_equal_ignore_trailing(res_stream, exp_stream)) {
                        std::cout << "[OK] [[" << key << "]] " << duration << " ms\n";
                    } else {
                        std::cout << "[WRONG ANSWER] [[" << key << "]] " << duration << " ms\n";
                    }
                }
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                std::cout << "[CRASH] [[" << key << "]] Process killed by signal " << sig;
                if (sig == 11) std::cout << " (Segmentation Fault)";
                else if (sig == 6) std::cout << " (Aborted)";
                std::cout << " after " << duration << " ms\n";
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[SYSTEM ERROR] " << e.what() << "\n";
        }
    }
    if (fs::exists(local_in)) fs::remove(local_in);
    if (fs::exists(local_out)) fs::remove(local_out);
    if (fs::exists(fs::absolute("exec"))) fs::remove(fs::absolute("exec"));
    return 0;
}
