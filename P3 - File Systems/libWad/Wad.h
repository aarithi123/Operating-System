#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <fstream>
#include <regex>

using namespace std;

class Wad {
public:
    struct Node {
        Node* parent;
        string filePath;
        vector<Node*> children;

        Node(Node* parent, const string& filePath) : parent(parent), filePath(filePath) {}
    };

    struct DescriptorObject {
        string name;
        uint32_t offset;
        uint32_t length;
        Node* position;

        DescriptorObject(Node* position) : name(""), offset(0), length(0), position(position) {}
        DescriptorObject(const string& name, uint32_t offset, uint32_t length, Node* position)
            : name(name), offset(offset), length(length), position(position) {}
    };

    string wadPath;
    fstream currentFile;
    char magic[5];
    uint32_t numberOfDescriptors;
    uint32_t descriptorOffset;
    stack<string> directory;
    unordered_map<string, DescriptorObject*> fileMap;

public:
    Wad(const string& path);
    static Wad* loadWad(const string& path);

    string getMagic();
    bool isContent(const string& path);
    bool isDirectory(const string& path);
    int getSize(const string& path);
    int getContents(const std::string& path, char* buffer, int length, int offset = 0);
    int getDirectory(const string& path, vector<string>* directory);
    void createDirectory(const string& path);
    void createFile(const string& path);
    int writeToFile(const std::string& path, const char* buffer, int length, int offset = 0);
};
