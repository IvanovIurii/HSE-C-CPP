#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <future>
#include <cpr/cpr.h>
#include <array>
#include <set>
#include <iterator>
#include <functional>
#include <zip.h>
#include <sstream>
#include <chrono>
#include <mutex>

#define CATS_AMOUNT 12
#define BATCH_SIZE 100
#define BASE_DIR "./kotichi/"
#define ARCHIVE_NAME "kotichi.zip"

#define ENDPOINT_URL "http://localhost:8080/cat"

namespace fs = std::filesystem;

struct FileSignature
{
    std::string mime_type;
    unsigned char signature[4];
};

const std::vector<FileSignature> signatures = {
    {"image/jpeg", {0xFF, 0xD8, 0xFF, 0xE0}},
    {"image/png", {0x89, 0x50, 0x4E, 0x47}},
    // image/plain or text ignored
};

std::string getMimeType(std::string text)
{
    char buffer[5];
    text.copy(buffer, 4);
    buffer[4] = '\0';

    for (const auto &sig : signatures)
    {
        if (strcmp(buffer, (const char *)sig.signature) == 0)
        {
            // todo: use enums + switch case
            if (sig.mime_type == "image/jpeg")
                return "jpeg";
            if (sig.mime_type == "image/png")
                return "png";
        }
    }

    return "unknown";
}

std::string saveImage(std::string file_name, std::string data)
{
    std::ofstream outfile(file_name, std::ios::binary);
    if (outfile.is_open())
    {
        outfile.write(data.c_str(), data.size());
        outfile.close();

        return file_name;
    }
    throw std::runtime_error("Failed to save iamge: " + file_name);
}

std::string downloadImage(std::string name)
{
    try
    {
        cpr::Response response;
        response = cpr::Get(cpr::Url{ENDPOINT_URL});

        std::string data = response.text;
        auto mime_type = getMimeType(data);
        if (mime_type == "unknown")
        {
            throw std::runtime_error("Unknown mime type");
        }

        auto file_name = BASE_DIR + name + "." + mime_type;
        return saveImage(file_name, data);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Something went wrong: " + std::string(e.what()));
    }
}

std::string getFileStreamAsString(std::string file_name)
{
    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file: " + file_name);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

void uploadArchive()
{
    auto payload = getFileStreamAsString(ARCHIVE_NAME);

    cpr::Response r = cpr::Post(
        cpr::Url{ENDPOINT_URL},
        cpr::Body{payload},
        cpr::Header{{"Content-Type", "application/zip"}});

    if (r.status_code == 200)
    {
        std::cout << "Archive uploaded successfully" << std::endl;
    }
    else
    {
        throw std::runtime_error("Failed to upload zip archive: " + std::to_string(r.status_code) + " + " + r.error.message);
    }
}

std::vector<std::string> downloadImagesAsync()
{

    std::set<std::string> cats_set;
    std::vector<std::string> image_paths;

    int batch_counter = 0;
    do
    {
        std::vector<std::future<std::string>> futures;
        for (int i = 0; i < BATCH_SIZE; i++)
        {
            auto name = std::to_string(batch_counter) + std::to_string(i);
            futures.emplace_back(std::async(std::launch::async, downloadImage, name));
        }

        for (auto &f : futures)
        {
            auto file_name = f.get();
            auto data = getFileStreamAsString(file_name);
            if (cats_set.find(data) == cats_set.end())
            {
                cats_set.insert(data);
                image_paths.push_back(file_name);
            }
        }
        batch_counter++;
    } while (cats_set.size() < CATS_AMOUNT);

    return image_paths;
}

int archiveImagesZip(std::vector<std::string> &image_paths)
{
    zip_t *zip_file = zip_open(ARCHIVE_NAME, ZIP_CREATE, NULL);
    if (!zip_file)
    {
        throw std::runtime_error("Failed to create zip archive");
    }

    for (const auto &image_path : image_paths)
    {
        auto src = zip_source_file(zip_file, image_path.c_str(), 0, 0);
        if (zip_file_add(zip_file, image_path.c_str(), src, 0) < 0)
        {
            zip_source_free(src);
            throw std::runtime_error("Failed to add file to zip archive: " + std::string(zip_strerror(zip_file)));
        }
    }

    zip_close(zip_file);

    std::cout << "Archive has been created successfully: " << ARCHIVE_NAME << std::endl;

    return 0;
}

int main()
{
    auto start = std::chrono::high_resolution_clock::now();

    auto dir_path = BASE_DIR;
    fs::create_directory(dir_path);

    auto image_paths = downloadImagesAsync();

    archiveImagesZip(image_paths);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = duration_cast<std::chrono::seconds>(end - start);

    std::cout << "Elapsed Time: " << duration.count() << " sec" << std::endl;

    uploadArchive();

    exit(EXIT_SUCCESS);
}