#include "MusicLibrary.h"
#include <fstream>
#include <curl/curl.h>

std::string MusicLibrary::get_full_music_name_list(std::string NameDirectory)
{
    std::string FullName;
    try {
        std::filesystem::path fullPath = this->path / NameDirectory;

        if (!std::filesystem::exists(fullPath)) {
            return "No music found. Directory doesn't exist.";
        }

        for (auto& temp : std::filesystem::directory_iterator(fullPath)) {
            if (temp.is_regular_file()) {
                std::string extension = temp.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if (extension == ".mp3" || extension == ".wav" || extension == ".ogg" ||
                    extension == ".flac" || extension == ".m4a") {
                    FullName += temp.path().filename().string() + '\n';
                }
            }
        }

        if (FullName.empty()) {
            return "No music files found in your library.";
        }

    }
    catch (const std::exception& e) {
        return "Error accessing music library: " + std::string(e.what());
    }

    return FullName;
}

std::string MusicLibrary::get_user_directory(const std::string& userId)
{
    std::filesystem::path userPath = this->path / userId;
    if (!std::filesystem::exists(userPath)) {
        std::filesystem::create_directories(userPath);
    }
    return userPath.string();
}

// Callback функция для записи данных в файл
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::ofstream* file)
{
    size_t totalSize = size * nmemb;
    file->write(static_cast<char*>(contents), totalSize);
    return totalSize;
}

bool MusicLibrary::upload_music(const std::string& userDir, const std::string& fileUrl, const std::string& fileName)
{
    try {
        // Создаем путь для сохранения файла
        std::filesystem::path savePath = userDir / std::filesystem::path(fileName);

        // Инициализируем CURL
        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Failed to initialize CURL" << std::endl;
            return false;
        }

        // Открываем файл для записи
        std::ofstream outputFile(savePath, std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Failed to open file for writing: " << savePath << std::endl;
            curl_easy_cleanup(curl);
            return false;
        }

        // Настраиваем CURL для загрузки файла
        curl_easy_setopt(curl, CURLOPT_URL, fileUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outputFile);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MusicBot/1.0");

        // Выполняем запрос
        CURLcode res = curl_easy_perform(curl);

        // Закрываем файл
        outputFile.close();

        // Проверяем результат
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
            // Удаляем частично загруженный файл
            std::filesystem::remove(savePath);
            curl_easy_cleanup(curl);
            return false;
        }

        // Получаем HTTP код
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200) {
            std::cerr << "HTTP error: " << http_code << std::endl;
            std::filesystem::remove(savePath);
            curl_easy_cleanup(curl);
            return false;
        }

        curl_easy_cleanup(curl);

        std::cout << "File successfully downloaded: " << savePath << std::endl;
        return true;

    }
    catch (const std::exception& e) {
        std::cerr << "Error in upload_music: " << e.what() << std::endl;
        return false;
    }
}
