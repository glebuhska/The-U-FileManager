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

int main(int argc, char* argv[]) {
	enable_raw_mode();
	while (true) {
		// idk ;3
                screen();
                fs::path target_path = (argc > 1) ? argv[1] : ".";
                auto entries = list_directory(target_path);
                print_entries(entries);
                std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	disable_raw_mode(); //i love pineapple 
	return 0;
}
