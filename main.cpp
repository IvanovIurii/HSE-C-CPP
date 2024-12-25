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

static std::set<int> mySet;

std::string getMimeType(std::string text)
{
    char buffer[5];
    text.copy(buffer, 4);
    buffer[4] = '\0';

    for (const auto &sig : signatures)
    {
        if (strcmp(buffer, (const char *)sig.signature) == 0)
        {
            // todo: use enums
            if (sig.mime_type == "image/jpeg")
                return "jpeg";
            if (sig.mime_type == "image/png")
                return "png";
        }
    }

    return "unknown";
}

// todo: should return string path of the file
int downloadImage(std::string name)
{
    try
    {
        cpr::Response r = cpr::Get(cpr::Url{ENDPOINT_URL});

        // checks if it was already saved
        std::hash<std::string> hash_fn;
        size_t hash_value = hash_fn(r.text);

        if (mySet.find(hash_value) == mySet.end())
        {
            mySet.insert(hash_value);
            std::cout << hash_value << " has been inserted into the set." << std::endl;
        }
        else
        {
            std::cout << hash_value << " already exists in the set" << std::endl;
            return 0; // should be warn, not 0 and not 1
        }

        auto mime_type = getMimeType(r.text);
        if (mime_type == "unknown")
        {
            throw std::runtime_error("Unknown mime type: " + mime_type);
        }
        auto result_name = BASE_DIR + name + "." + mime_type;

        std::ofstream outfile(result_name, std::ios::binary);
        if (outfile.is_open())
        {
            outfile.write(r.text.c_str(), r.text.size());
            outfile.close();
            std::cout << "Image downloaded successfully" << std::endl;

            return 0;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Something went wrong: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main()
{
    auto dir_path = BASE_DIR;
    fs::create_directory(dir_path);

    //
    std::vector<std::future<int>> futures; // return std::string names
    for (int i = 0; i < CATS_AMOUNT; i++)
    {
        futures.emplace_back(std::async(std::launch::async, downloadImage, std::to_string(i)));
    }

    std::vector<int> imagePaths;
    for (auto &f : futures)
    {
        imagePaths.push_back(f.get());
    }
    // todo: std::names and use above instead of image_paths

    std::vector<fs::path> image_paths;

    for (const auto &entry : fs::directory_iterator(dir_path))
    {
        image_paths.push_back(entry.path());
    }

    std::string archive_path = "kotichi_archive.zip";

    try
    {
        zip_t *zip_file = zip_open(archive_path.c_str(), ZIP_CREATE, NULL);
        if (!zip_file)
        {
            std::cerr << "Error: Failed to create ZIP archive." << std::endl;
            return 1;
        }

        // Add images to the archive
        for (const auto &image_path : image_paths)
        {
            auto src = zip_source_file(zip_file, image_path.string().c_str(), 0, 0);
            if (zip_file_add(zip_file, image_path.string().c_str(), src, 0) < 0)
            {
                zip_source_free(src);
                throw std::runtime_error("Failed to add file to zip: " + std::string(zip_strerror(zip_file)));
            }
        }

        zip_close(zip_file);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating archive: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Archive created successfully: " << archive_path << std::endl;

    std::ifstream file(archive_path, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open archive file." << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    // Create the POST request
    cpr::Response r = cpr::Post(
        cpr::Url{ENDPOINT_URL},
        cpr::Body{buffer.str()},
        cpr::Header{{"Content-Type", "application/zip"}});

    if (r.status_code == 200)
    {
        std::cout << "Archive uploaded successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error uploading archive: " << r.status_code << " - " << r.error.message << std::endl;
    }

    return 0;
}