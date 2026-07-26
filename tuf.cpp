//     BSD 2-Clause License
//
//   Copyright (c) 2026, glebuhska
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.




#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <iomanip> 
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fstream>
#include <sstream>


//void print_entries(const std::vector<FileEntry>& entries) {
//   for (const auto& e : entries) {
//        switch (e.type) {
//            case EntryType::File:
//                std::cout << std::setw(8) << e.size << " bytes  ";
//                break;
//            case EntryType::Directory:
//                std::cout << std::setw(11) << "<DIR>" << "  ";
//                break;
//            case EntryType::Other:
//                std::cout << std::setw(11) << "<OTHER>" << "  ";
//                break;
//        }
//        std::cout << e.name << '\n';
//    }
//}
//tak bilo ranshe
//This is how it used to be

namespace fs = std::filesystem;
termios orig_termios;

void disable_raw_mode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}


void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


enum class EntryType {
    File,
    Directory,
    Other
};

struct FileEntry {
    std::string name;
    EntryType type;
    uintmax_t size; // i like pineapple
};

std::vector<FileEntry> list_directory(const fs::path& dir_path) {
    std::vector<FileEntry> entries;

    if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
        std::cerr << "ls: " << dir_path << " shut up pls\n";
        return entries; 
    }

    try {
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            std::string name_str = entry.path().filename().string();

            if (!name_str.empty() && name_str[0] == '.') {
                continue;
            }

            std::error_code ec;
            auto status = entry.status(ec);

            FileEntry fe;
            fe.name = name_str;

            if (fs::is_regular_file(status)) {
                fe.type = EntryType::File;
                auto size = entry.file_size(ec);
                fe.size = ec ? 0 : size;
            } else if (fs::is_directory(status)) {
                fe.type = EntryType::Directory;
                fe.size = 0;
            } else {
                fe.type = EntryType::Other;
                fe.size = 0;
            }

            entries.push_back(fe);
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "ls error: " << e.what() << '\n';
    }

    return entries;
}

void print_entries(const std::vector<FileEntry>& entries) {
    for (const auto& e : entries) {
        switch (e.type) {
            case EntryType::File:
                std::cout << std::setw(8) << e.size << " bytes  ";
                break;
            case EntryType::Directory:
                std::cout << std::setw(11) << "<DIR>" << "  ";
                break;
            case EntryType::Other:
                std::cout << std::setw(11) << "<OTHER>" << "  ";
                break;
        }
        std::cout << e.name << '\n';
    }
}

void screen(){
	std::cout << "\033[2J" << "\033[H";
}

int get_terminal_height() {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return w.ws_row;
}

std::string read_command_line() {
    int h = get_terminal_height();
    std::cout << "\033[" << h << ";1H"; 
    std::cout << "\033[K"; 
    std::cout << ":" << std::flush;

    std::string cmd;
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\n' || c == '\r') {
            break;
        } else if (c == 127 || c == 8) { 
            if (!cmd.empty()) {
                cmd.pop_back();
                std::cout << "\b \b" << std::flush; 
            }
        } else {
            cmd += c;
            std::cout << c << std::flush;
        }
    }
    return cmd;
}

void mk(const std::string& filename) {
	std::ofstream file(filename);
	if (!file) {
		std::cerr << "mk: can't create  " << filename << "\n";
		return;
	}
	file.close();
}

int main(int argc, char* argv[]) {
    enable_raw_mode();
    bool running = true;

    while (running) {
        screen();
        fs::path target_path = (argc > 1) ? argv[1] : ".";
        auto entries = list_directory(target_path);
        print_entries(entries);

        char c;
        read(STDIN_FILENO, &c, 1);

        if (c == ':') {
            std::string cmd = read_command_line();

            if (cmd == "q") {
                running = false;
            } else if (cmd == "help") {
                screen();
                std::cout << "help - help" << std::endl;
                std::cout << "mk - add file" << std::endl;
                std::cout << "q - quit" << std::endl;
                std::cout << "\nPress any key..." << std::flush;
                char dummy;
                read(STDIN_FILENO, &dummy, 1);
            } else {
                std::istringstream iss(cmd);
                std::string action, filename;
                iss >> action >> filename;

                if (action == "mk") {
                    if (filename.empty()) {
                        std::cerr << "mk: no filename given\n";
                    } else {
                        mk(filename); // i love pineapple
                    }
                }
            }
        }
    }

    disable_raw_mode();
    return 0;
}
