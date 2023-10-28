#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <jsoncpp/json/json.h>

struct Range {
    int min;
    int max;
    int weight;
};

std::string removeInvalidSpaces(const std::string& input) {
    std::string result;
    bool inQuotes = false;

    for (char c : input) {
        if (c == '"') {
            inQuotes = !inQuotes;
        }

        if (!std::isspace(c) || inQuotes) {
            result += c;
        }
    }

    return result;
}
int generateRandomNumber(const std::vector<Range>& ranges) {
    int totalWeight = 0;
    for (const Range& range : ranges) {
        totalWeight += range.weight;
    }

    int randomWeight = std::rand() % totalWeight;

    for (const Range& range : ranges) {
        randomWeight -= range.weight;
        if (randomWeight < 0) {
            return std::rand() % (range.max - range.min + 1) + range.min;
        }
    }

    return 0; // Default return value (can be modified)
}

bool parseConfigFile(const std::string& filePath, std::map<std::string, std::vector<Range>>& config) {
    std::ifstream configFile(filePath);
    if (!configFile.is_open()) {
        std::cout << "Failed to open " << filePath << std::endl;
        return false;
    }
    std::string jsonData((std::istreambuf_iterator<char>(configFile)), std::istreambuf_iterator<char>());
    configFile.close();

    jsonData = removeInvalidSpaces(jsonData);

    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(jsonData, root)) {
        std::cout << "Failed to parse " << filePath << std::endl;
        return false;
    }

    if (!root.isObject()) {
        std::cout << "Invalid JSON format in " << filePath << std::endl;
        return false;
    }

    for (const auto& key : root.getMemberNames()) {
        const Json::Value& values = root[key];
        if (values.isArray()) {
            std::vector<Range> ranges;
            for (const Json::Value& value : values) {
              if (value.isObject() && value.isMember("min") &&
                  value.isMember("max") && value.isMember("weight")) {
                Range range;
                range.min = value["min"].asInt();
                range.max = value["max"].asInt();
                range.weight = value["weight"].asInt();
                ranges.push_back(range);
              }
            }
            config[key] = ranges;
        }
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./random_generator <config_file_path> <Rand_count>" << std::endl;
        return 1;
    }

    std::srand(std::time(nullptr));
    std::vector<Range> ranges;
    std::string configFilePath(argv[1]);
    std::map<std::string, std::vector<Range>> config;

    if (!parseConfigFile(configFilePath, config)) {
        return 1;
    }
    int count = atoi(argv[2]);
    for(int idx = 0; idx < count; idx++){
        printf("idx=%d ", idx);
        for (const auto& entry : config) {
            const std::string& key = entry.first;
            // printf("%s ", key.c_str());
            // for(uint i = 0; i< entry.second.size(); i++) {
            //     printf("{min=%d, max=%d, weight=%d} ",entry.second[i].min,
            //     entry.second[i].max, entry.second[i].weight);
            // }
            const std::string& value =
                std::to_string(generateRandomNumber(entry.second));
            std::cout << std::left << "," << key << "=" << std::setw(4) << value;
        }
        std::cout << std::endl;
    }
    return 0;
}
