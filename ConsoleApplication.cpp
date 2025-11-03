#include <tgbot/tgbot.h>
#include <iostream>
#include "MusicLibrary.h"
#include <algorithm>
#include <vector>
#include <filesystem>
#include <map>

using namespace std;

string MusicPth = "Music/";
MusicLibrary Music(MusicPth);

struct UserState {
    bool waitingForFile = false;
    string currentAction;
};

map<int64_t, UserState> userStates;

int main()
{
    if (!filesystem::exists(MusicPth)) {
        filesystem::create_directory(MusicPth);
    }

    TgBot::Bot bot("мой токен");

    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        try {
            string userDir = MusicPth + to_string(message->chat->id);

            if (!filesystem::exists(userDir)) {
                filesystem::create_directory(userDir);
                cout << "Created directory for user: " << message->chat->id << endl;
            }

            // Сбрасываем состояние пользователя
            userStates[message->chat->id] = UserState();

            TgBot::ReplyKeyboardMarkup::Ptr Keyboard(new TgBot::ReplyKeyboardMarkup);
            Keyboard->resizeKeyboard = true;
            Keyboard->oneTimeKeyboard = true;

            vector<vector<TgBot::KeyboardButton::Ptr>> buttons;

            vector<TgBot::KeyboardButton::Ptr> row1;
            TgBot::KeyboardButton::Ptr GetMusic(new TgBot::KeyboardButton);
            GetMusic->text = "GetMusic";
            row1.push_back(GetMusic);

            TgBot::KeyboardButton::Ptr SendMusic(new TgBot::KeyboardButton);
            SendMusic->text = "SendMusic";
            row1.push_back(SendMusic);

            buttons.push_back(row1);
            Keyboard->keyboard = buttons;

            bot.getApi().sendMessage(message->chat->id, "Hello! Welcome to Music Bot 🎵", false, 0, Keyboard);

        }
        catch (const exception& e) {
            cerr << "Error: " << e.what() << endl;
            bot.getApi().sendMessage(message->chat->id, "Error creating your music directory");
        }
        });

    bot.getEvents().onCommand("help", [&bot](TgBot::Message::Ptr message) {
        string helpText = "Available commands:\n"
            "/start - Start the bot\n"
            "/help - Show help\n\n"
            "Buttons:\n"
            "GetMusic - Show your music library\n"
            "SendMusic - Upload music files";
        bot.getApi().sendMessage(message->chat->id, helpText);
        });

    bot.getEvents().onNonCommandMessage([&bot](TgBot::Message::Ptr message) {
        try {
            int64_t chatId = message->chat->id;

            // Если это текстовое сообщение (не файл)
            if (!message->document && !message->audio) {
                if (message->text == "GetMusic") {
                    string fullName = Music.get_full_music_name_list(to_string(chatId));
                    if (fullName.find("No music") != string::npos) {
                        bot.getApi().sendMessage(chatId, fullName);
                    }
                    else {
                        bot.getApi().sendMessage(chatId, "Your music library:\n" + fullName);
                    }
                }
                else if (message->text == "SendMusic") {
                    userStates[chatId] = { true, "upload_music" };
                    bot.getApi().sendMessage(chatId, "Please send me a music file (MP3, WAV, OGG)");
                }
                else if (userStates[chatId].waitingForFile) {
                    bot.getApi().sendMessage(chatId, "Please send a music file, not text.");
                }
                else {
                    bot.getApi().sendMessage(chatId, "Unknown command. Use /help for assistance.");
                }
            }

        }
        catch (const exception& e) {
            cerr << "Error processing message: " << e.what() << endl;
            bot.getApi().sendMessage(message->chat->id, "Error processing your request");
        }
        });

    // Обработка загрузки файлов - ДОЛЖНА БЫТЬ ПЕРВОЙ!
    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        try {
            int64_t chatId = message->chat->id;
            string userId = to_string(chatId);

            if (message->document || message->audio) {
                string fileName;
                string fileId;

                if (message->document) {
                    fileName = message->document->fileName;
                    fileId = message->document->fileId;
                }
                else if (message->audio) {
                    fileName = "audio_" + to_string(message->audio->fileSize) + ".mp3";
                    fileId = message->audio->fileId;
                }

                // Проверяем, ожидаем ли мы файл от пользователя
                if (!userStates[chatId].waitingForFile) {
                    bot.getApi().sendMessage(chatId, "Please use the 'SendMusic' button first to upload files.");
                    return;
                }

                // Проверяем расширение файла (для документов)
                if (message->document) {
                    size_t dotPos = fileName.find_last_of(".");
                    if (dotPos != string::npos) {
                        string extension = fileName.substr(dotPos + 1);
                        transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                        if (extension != "mp3" && extension != "wav" && extension != "ogg" &&
                            extension != "flac" && extension != "m4a") {
                            bot.getApi().sendMessage(chatId, "❌ Please send only music files (MP3, WAV, OGG, FLAC, M4A)");
                            return;
                        }
                    }
                }

                bot.getApi().sendMessage(chatId, "🎵 Downloading: " + fileName + "...");

                try {
                    // Получаем информацию о файле
                    TgBot::File::Ptr file = bot.getApi().getFile(fileId);
                    string fileUrl = "https://api.telegram.org/file/bot" + bot.getToken() + "/" + file->filePath;

                    // Получаем путь к директории пользователя
                    string userDir = Music.get_user_directory(userId);

                    // Загружаем файл
                    if (Music.upload_music(userDir, fileUrl, fileName)) {
                        bot.getApi().sendMessage(chatId, "✅ File successfully uploaded: " + fileName);
                    }
                    else {
                        bot.getApi().sendMessage(chatId, "❌ Failed to upload file: " + fileName);
                    }

                }
                catch (const exception& e) {
                    cerr << "Error downloading file: " << e.what() << endl;
                    bot.getApi().sendMessage(chatId, "❌ Error downloading file: " + string(e.what()));
                }

                // Сбрасываем состояние пользователя
                userStates[chatId] = UserState();
            }

        }
        catch (const exception& e) {
            cerr << "Error processing file: " << e.what() << endl;
            bot.getApi().sendMessage(message->chat->id, "Error processing your file");
        }
        });

    try {
        cout << "Bot username: " << bot.getApi().getMe()->username << endl;
        cout << "Bot started successfully!" << endl;

        TgBot::TgLongPoll LongPoll(bot);
        while (true) {
            try {
                LongPoll.start();
            }
            catch (const exception& e) {
                cerr << "Long poll error: " << e.what() << endl;
                this_thread::sleep_for(chrono::seconds(5));
            }
        }
    }
    catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
