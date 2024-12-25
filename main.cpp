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

#define CATS_AMOUNT 12
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

static std::set<int> cats_hashes_set;

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

bool isDuplicate(std::string data)
{
    std::hash<std::string> hash_fn;
    size_t hash_value = hash_fn(data);

    // todo: here probably we have to sync because of critical section
    if (cats_hashes_set.find(hash_value) == cats_hashes_set.end())
    {
        cats_hashes_set.insert(hash_value);
        return false;
    }
    return true;
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
}

std::string downloadImage(std::string name)
{
    try
    {
        cpr::Response response;
        do
        {
            response = cpr::Get(cpr::Url{ENDPOINT_URL});
        } while (isDuplicate(response.text));

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

int uploadArchive()
{
    std::ifstream file(ARCHIVE_NAME, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Could not open archive file" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    cpr::Response r = cpr::Post(
        cpr::Url{ENDPOINT_URL},
        cpr::Body{buffer.str()},
        cpr::Header{{"Content-Type", "application/zip"}});

    if (r.status_code == 200)
    {
        std::cout << "Archive uploaded successfully" << std::endl;
        exit(EXIT_SUCCESS);
    }
    else
    {
        std::cerr << "Failed to upload zip archive: " << r.status_code << " - " << r.error.message << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::vector<std::string> downloadImagesAsync()
{
    std::vector<std::future<std::string>> futures;
    for (int i = 0; i < CATS_AMOUNT; i++)
    {
        futures.emplace_back(std::async(std::launch::async, downloadImage, std::to_string(i)));
    }

    std::vector<std::string> image_paths;
    for (auto &f : futures)
    {
        image_paths.push_back(f.get());
    }

    return image_paths;
}

int archiveImagesZip(std::vector<std::string> &image_paths)
{
    try
    {
        zip_t *zip_file = zip_open(ARCHIVE_NAME, ZIP_CREATE, NULL);
        if (!zip_file)
        {
            std::cerr << "Failed to create zip archive" << std::endl;
            exit(EXIT_FAILURE);
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
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating archive: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Archive has been created successfully: " << ARCHIVE_NAME << std::endl;

    return 0;
}

// todo: add endpoint url as a parameter
int main()
{
    auto dir_path = BASE_DIR;
    fs::create_directory(dir_path);

    auto image_paths = downloadImagesAsync();

    archiveImagesZip(image_paths);
    uploadArchive();

    exit(EXIT_SUCCESS);
}