#include <boost/filesystem.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #include <Windows.h>

    std::string wstring_to_string(const std::wstring& s) {
        int len;
        int slength = (int)s.length() + 1;
        len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0); 
        char* buf = new char[len];
        WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0); 
        std::string r(buf);
        delete[] buf;
        return r;
    }
#endif

bool command_found(std::string command) {
    struct stat sb;
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        std::string delimiter = ";";
    #elif __unix__ || __APPLE__
        std::string delimiter = ":";
    #endif
    std::string path = std::string(getenv("PATH"));
    size_t start_pos = 0, end_pos = 0;
    while ((end_pos = path.find(":", start_pos)) != std::string::npos) {
        std::string current_path = path.substr(start_pos, end_pos - start_pos) + "/" + command;
        if (stat(current_path.c_str(), &sb) == 0) return true;
        start_pos = end_pos + 1;
    }
    return false;
}

std::string replace(std::string original, std::string substring, std::string replacement) {
    std::string temp = original;
    if ((int) temp.find(substring) > -1) temp = temp.replace(temp.find(substring), substring.length(), replacement);
    return temp;
}

std::string replace_all(std::string original, std::string substring, std::string replacement) {
    std::string temp = original;
    while ((int) temp.find(substring) > -1) temp = replace(temp, substring, replacement);
    return temp;
}

std::string substring_before(std::string original, std::string substring, int count) {
    std::string temp = original;
    for (int i = 0; i < count + 1; i++) if ((int) temp.find(substring) > -1) temp = temp.substr(0, temp.rfind(substring));
    temp.append("/");
    return temp;
}

std::string drive_letter(std::string path) {
    std::string letters[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
    std::string drive_letter = "";
    for (int i = 0; i < 26; i++) {
        drive_letter = " " + letters[i] + ":";
        if ((int) path.find(drive_letter) > -1) break;
    }
    return drive_letter;
}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    int WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdLine, int nShowCmd) {
#else
    int main(int argc, char **argv) {
#endif
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        int argc, start_count = 0;
        LPWSTR *arg_list = CommandLineToArgvW(GetCommandLineW(), &argc);
        std::vector<std::string> argv;
        for (int i = 0; i < argc; i++) argv.push_back(wstring_to_string(arg_list[i]));
        std::string os_drive = "C:/", root_directory = "C:/";
    #elif __unix__ || __APPLE__
        int start_count = 1;
        std::string os_drive = "Z:/", root_directory = "/";
    #endif
    std::string executable = argv[0];
    executable = replace_all(boost::filesystem::system_complete(executable).string(), "\\", "/");
    #if __unix__ || __APPLE__
        executable = "Z:" + executable;
    #endif
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) args.push_back(argv[i]);
    bool append = false, end_append = false;
    int append_index = -1;
    for (int i = 0; i < args.size(); i++) {
        if ((int) args[i].find("<concat>") > -1) {
            int count = 0;
            while ((int) args[i].find("<concat>") > -1) {
                args[i] = replace(args[i], "<concat>", "");
                append = !append;
                count++;
            }
            if (!append && count % 2 != 0) end_append = true;
            else append_index = i;
        }
        if ((append || end_append) && i != append_index) {
            args[append_index].append(args[i]);
            args.erase(args.begin() + i);
            end_append = false;
            i--;
        }
        if (append) args[append_index].append(" ");
    }
    for (int i = 0; i < args.size(); i++) {
        std::string line = args[i], previous = "";
        int count = start_count;
        while ((int) line.find("../") > -1) {
            line = replace(line, "../", "");
            previous.append("../");
            count++;
        }
        if (previous.empty()) previous = "../";
        line = args[i];
        line = replace_all(replace_all(replace(line, previous, substring_before(executable, "/", count)), "./", substring_before(executable, "/", start_count))
                                     + "/", "\\", "/");
        std::vector<std::string> segments;
        while ((int) line.find("/") > -1) {
            segments.push_back(line.substr(0, line.rfind("/")));
            segments[segments.size() - 1] = segments[segments.size() - 1].substr(segments[segments.size() - 1].rfind("/") + 1);
            line = line.substr(0, line.rfind("/"));
            if ((int) line.find("/") == -1) line = "";
        }
        for (int j = segments.size() - 1; j >= 0 ; j--) {
            bool quote = false;
            if ((int) segments[j].find(" ") > -1) {
                try {
                    quote = boost::filesystem::exists(replace(replace_all(line.substr(line.rfind(":/") - 1), "\"", ""), os_drive, root_directory)
                                                    + replace(segments[j], drive_letter(segments[j]), ""));
                } catch (...) {
                    quote = command_found("\"" + segments[j] + "\"");
                }
            }
            if (quote) {
                std::string temp_segment = segments[j];
                segments[j] = "\"" + replace(segments[j], drive_letter(segments[j]), "") + "\"";
                if ((int) temp_segment.find(drive_letter(temp_segment)) > -1) segments[j].append(drive_letter(temp_segment));
            }
            line.append(segments[j]);
            if (j > 0) line.append("/");
        }
        #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            line = replace_all(line, "/", "\\");
            line = replace_all(line, " \\", " /");
            ShowWindow(GetConsoleWindow(), SW_HIDE);
        #endif
        line = replace_all(line, os_drive, root_directory);
        if ((int) line.rfind("/") == line.size() - 1) line = line.substr(0, line.rfind("/"));
        system(line.c_str());
    }
    exit(EXIT_SUCCESS);
}
