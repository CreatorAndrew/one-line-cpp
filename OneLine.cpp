#include <boost/filesystem.hpp>
#include <sys/stat.h>
using namespace boost::filesystem;
using namespace std;

bool command_found(string command) {
    struct stat sb;
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        string delimiter = ";";
    #else
        string delimiter = ":";
    #endif
    string path = string(getenv("PATH"));
    size_t begin = 0, end = 0;
    while ((end = path.find(":", begin)) != string::npos) {
        string current_path = path.substr(begin, end - begin) + "/" + command;
        if (stat(current_path.c_str(), &sb) == 0) return true;
        begin = end + 1;
    }
    return false;
}

string drive(string path) {
    string letters[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
    string drive = "";
    for (int i = 0; i < sizeof(letters) / sizeof(letters[0]); i++) {
        drive = " " + letters[i] + ":";
        if ((int) path.find(drive) > -1) break;
    }
    return drive;
}

string replace(string orig, string from, string to) {
    string temp = orig;
    if ((int) temp.find(from) > -1) temp = temp.replace(temp.find(from), from.length(), to);
    return temp;
}

string replace_all(string orig, string from, string to) {
    string temp = orig;
    while ((int) temp.find(from) > -1) temp = replace(temp, from, to);
    return temp;
}

string substr_up_to(string orig, string substr, int times) {
    string temp = orig;
    for (int i = 0; i < times + 1; i++) if ((int) temp.find(substr) > -1) temp = temp.substr(0, temp.rfind(substr));
    temp.append(substr);
    return temp;
}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #include <Windows.h>

    string wstring_to_string(const wstring& s) {
        int len;
        int slength = (int)s.length() + 1;
        len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
        char* buf = new char[len];
        WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0);
        string r(buf);
        delete[] buf;
        return r;
    }

    int WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdline, int nShowCmd) {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
        int argc, once = 0;
        LPWSTR *arg_list = CommandLineToArgvW(GetCommandLineW(), &argc);
        vector<string> argv;
        for (int i = 0; i < argc; i++) argv.push_back(wstring_to_string(arg_list[i]));
        string os_drive = "C:/", root_dir = "C:/";
#else
    int main(int argc, char **argv) {
        int once = 1;
        string os_drive = "Z:/", root_dir = "/";
#endif
    string here = replace_all(system_complete(argv[0]).string(), "\\", "/");
    #if !(defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
        here = "Z:" + here;
    #endif
    vector<string> args;
    for (int i = 1; i < argc; i++) args.push_back(argv[i]);
    bool concat = false, end = false;
    int begin = -1;
    for (int i = 0; i < args.size(); i++) {
        if ((int) args[i].find("<concat>") > -1) {
            int count = 0;
            while ((int) args[i].find("<concat>") > -1) {
                args[i] = replace(args[i], "<concat>", "");
                concat = !concat;
                count++;
            }
            if (!concat && count % 2 != 0) end = true;
            else begin = i;
        }
        if ((concat || end) && i != begin) {
            args[begin].append(args[i]);
            args.erase(args.begin() + i);
            end = false;
            i--;
        }
        if (concat) args[begin].append(" ");
    }
    for (int i = 0; i < args.size(); i++) {
        string line = args[i], prev = "";
        int times = once;
        while ((int) line.find("../") > -1) {
            line = replace(line, "../", "");
            prev.append("../");
            times++;
        }
        if (prev.empty()) prev = "../";
        line = args[i];
        line = replace_all(replace(replace_all(line, "\\", "/"), prev, substr_up_to(here, "/", times)), "./", substr_up_to(here, "/", once)) + "/";
        vector<string> segs;
        while ((int) line.find("/") > -1) {
            segs.push_back(line.substr(0, line.rfind("/")));
            segs[segs.size() - 1] = segs[segs.size() - 1].substr(segs[segs.size() - 1].rfind("/") + 1);
            line = line.substr(0, line.rfind("/"));
            if ((int) line.find("/") == -1) line = "";
        }
        for (int j = segs.size() - 1; j >= 0; j--) {
            bool quote = false;
            if ((int) segs[j].find(" ") > -1) {
                try {
                    quote = exists(replace(replace_all(line.substr(line.rfind(":/") - 1), "\"", ""), os_drive, root_dir) + replace(segs[j], drive(segs[j]), ""));
                } catch (...) {
                    quote = command_found("\"" + segs[j] + "\"");
                }
            }
            if (quote) {
                string temp = segs[j];
                segs[j] = "\"" + replace(segs[j], drive(segs[j]), "") + "\"";
                if ((int) temp.find(drive(temp)) > -1) segs[j].append(drive(temp));
            }
            line.append(segs[j]);
            if (j > 0) line.append("/");
        }
        #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            line = replace_all(line, "/", "\\");
            line = replace_all(line, " \\", " /");
        #endif
        line = replace_all(line, os_drive, root_dir);
        system(line.c_str());
    }
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        ShowWindow(GetConsoleWindow(), SW_RESTORE);
    #endif
    exit(EXIT_SUCCESS);
}
