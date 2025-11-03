#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <iostream>

class MusicLibrary
{
public:
    MusicLibrary(std::string path) { this->path = path; };
    std::string get_full_music_name_list(std::string NameDirectory);
    bool upload_music(const std::string& userDir, const std::string& filePath, const std::string& fileName);
    std::string get_user_directory(const std::string& userId);

private:
    std::filesystem::path path;
};
