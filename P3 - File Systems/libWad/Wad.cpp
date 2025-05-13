#include "Wad.h"

#include <iostream>      // For standard input/output operations
#include <fstream>       // To handle file streams
#include <string>        // To use std::string
#include <vector>        // To use std::vector
#include <map>           // To use std::map
#include <stack>         // To use std::stack
#include <regex>         // For regex matching patterns


//constructor
Wad::Wad(const std::string& path) {
    // open file & read header
    wadPath = path;
    currentFile.open(path, std::ios::in | std::ios::out | std::ios::binary);

    magic[4] = '\0';
    currentFile.read(magic, 4);
    currentFile.read(reinterpret_cast<char*>(&numberOfDescriptors), 4);
    currentFile.read(reinterpret_cast<char*>(&descriptorOffset), 4);

    // create root node in the lookup structures
    Node* rootNode = new Node(nullptr, "/");
    directory.push("/");
    fileMap["/"] = new DescriptorObject(rootNode);

    // scan the descriptor list
    std::regex mapPattern("E\\dM\\d");
    currentFile.seekg(descriptorOffset, std::ios::beg);

    for (int i = 0; i < numberOfDescriptors; i++) {
        // pull descriptor
        uint32_t dataOffset = 0;
        uint32_t dataLength = 0;
        char rawName[9] = {0};
        currentFile.read(reinterpret_cast<char*>(&dataOffset), 4);
        currentFile.read(reinterpret_cast<char*>(&dataLength), 4);
        currentFile.read(rawName, 8);
        std::string nameStr(rawName);

        // handle map directories
        if (std::regex_match(nameStr, mapPattern)) {
            std::string parentPath = directory.top();
            Node* parentNode = fileMap[parentPath]->position;

            std::string fullMapPath = parentPath + nameStr + "/";
            Node* mapNode = new Node(parentNode, fullMapPath);
            fileMap[fullMapPath] = new DescriptorObject(nameStr, dataOffset, dataLength, mapNode);
            parentNode->children.push_back(mapNode);

            directory.push(fullMapPath);

            // next ten descriptors belong to this map
            for (int j = 0; j < 10; ++j, ++i) {
                currentFile.read(reinterpret_cast<char*>(&dataOffset), 4);
                currentFile.read(reinterpret_cast<char*>(&dataLength), 4);
                currentFile.read(rawName, 8);
                std::string lumpName(rawName);

                std::string lumpFullPath = fullMapPath + lumpName;
                Node* lumpNode = new Node(mapNode, lumpFullPath);
                fileMap[lumpFullPath] = new DescriptorObject(lumpName, dataOffset, dataLength, lumpNode);
                mapNode->children.push_back(lumpNode);
            }

            directory.pop();
            continue;
        }

        // handle namespace START marker
        if (nameStr.find("_START") != std::string::npos) {
            std::string dirName = nameStr.substr(0, nameStr.find("_START"));
            std::string parentPath = directory.top();
            Node* parentNode = fileMap[parentPath]->position;

            std::string newDirPath = parentPath + dirName + "/";
            Node* newDirNode = new Node(parentNode, newDirPath);
            fileMap[newDirPath] = new DescriptorObject(nameStr, dataOffset, dataLength, newDirNode);
            parentNode->children.push_back(newDirNode);
            directory.push(newDirPath);
            continue;
        }

        // handle namespace END marker
        if (nameStr.find("_END") != std::string::npos) {
            if (!directory.empty())
                directory.pop();
            continue;
        }

        // fallback handling for files outside directory markers
        std::string parentPath = directory.empty() ? "/" : directory.top();
        Node* parentNode = fileMap[parentPath]->position;
        std::string fullFilePath = parentPath + nameStr;

        Node* contentNode = new Node(parentNode, fullFilePath);
        fileMap[fullFilePath] = new DescriptorObject(nameStr, dataOffset, dataLength, contentNode);
        parentNode->children.push_back(contentNode);
    }
}

Wad* Wad::loadWad(const string &path) {
    return new Wad(path);
}

string Wad::getMagic() {
    return magic;
}

bool Wad::isContent(const string& path) {
    // look up the path in the fileMap
    auto entryIt = fileMap.find(path);

    // if path not found, it's not content
    if (entryIt == fileMap.end())
        return false;

    // if descriptor has length, it's content
    DescriptorObject* descriptor = entryIt->second;
    return (descriptor->length != 0);
}

bool Wad::isDirectory(const string &path) {
    //ensure non-root paths end with '/'
    string lookup = path;
    if (lookup.size() > 1 && lookup.back() != '/')
        lookup += '/';

    //if it's not in the map, it's not a directory
    auto it = fileMap.find(lookup);
    if (it == fileMap.end())
        return false;

    //directories are represented by a DescriptorObject with length = 0
    return (it->second->length == 0);
}

int Wad::getSize(const string &path) {
    if (isContent(path) == false ){
        return -1;
    } 
    else {
        //returns the size in bytes of a file
        auto iterator = fileMap.find(path);
        DescriptorObject* sizeCheck = iterator->second;
        return sizeCheck->length;
    }
}

int Wad::getContents(const std::string& path, char* buffer, int length, int offset) {
    //reject non-file paths
    if (!isContent(path))
        return -1;

    //locate the descriptor
    auto iter = fileMap.find(path);
    DescriptorObject* desc = iter->second;

    //compute starting position and total size
    int startPos = desc->offset + offset;
    int totalSize = desc->length;

    //seek to the requested byte
    currentFile.seekg(startPos, std::ios::beg);

    //if offset is beyond EOF, nothing to read
    if (offset > totalSize)
        return 0;

    //clamp so we don't read past EOF
    int available = totalSize - offset;
    if (length > available)
        length = available;

    //read into buffer and return actual bytes read
    currentFile.read(buffer, length);
    return static_cast<int>(currentFile.gcount());
}

int Wad::getDirectory(const string &path, vector<string> *directory) {
    //only proceed if 'path' names a directory
    if (!isDirectory(path))
        return -1;

    //normalize: ensure non-root paths end with '/'
    string lookup = path;
    if (lookup.size() > 1 && lookup.back() != '/')
        lookup += '/';

    //find the directory descriptor and its Node
    auto mapIt = fileMap.find(lookup);
    if (mapIt == fileMap.end())
        return -1;
    Node* dirNode = mapIt->second->position;

    //get each child entry (stripping the parent prefix and any slashes)
    int count = static_cast<int>(dirNode->children.size());
    for (Node* child : dirNode->children) {
        string entry = child->filePath;
        // remove the parent path portion
        string name = entry.substr(lookup.size());
        // trim leading slash if present
        if (!name.empty() && name.front() == '/')
            name.erase(0, 1);
        // trim trailing slash if present
        if (!name.empty() && name.back() == '/')
            name.pop_back();
        directory->push_back(name);
    }

    return count;
}

void Wad::createDirectory(const string& path)
{
    // Return if path is invalid or root
    if (path.empty() || path == "/")                     
        return;

    // Trim leading and trailing slashes
    string trimmedPath = path.substr(1);                        
    if (trimmedPath.back() == '/') 
        trimmedPath.pop_back();             

    // Walk through each directory level and check if it exists
    string currentPath = "/";
    while (trimmedPath.find('/') != string::npos)               
    {
        size_t slashPos = trimmedPath.find('/');
        string nextDir = trimmedPath.substr(0, slashPos + 1);        
        trimmedPath.erase(0, slashPos + 1);                        
        currentPath += nextDir;

        // If any parent directory doesn't exist, abort
        if (fileMap.find(currentPath) == fileMap.end())    
            return;
    }

    // Final directory segment must be at most 2 characters
    if (trimmedPath.size() > 2)
        return;

    // Get parent descriptor and node
    auto parentIt = fileMap.find(currentPath);
    if (parentIt == fileMap.end()) return;                    

    DescriptorObject* parentDesc = parentIt->second;
    Node* parentNode = parentDesc->position;

    // Determine the descriptor name we will insert after
    string anchorName;
    if (!parentNode->children.empty()) {
        string lastChildPath = parentNode->children.back()->filePath;
        anchorName = fileMap[lastChildPath]->name;
    } else {
        anchorName = parentDesc->name;
    }

    // If anchor is a _START marker, check whether to use _START or _END
    if (anchorName.find("_START") != string::npos) {
        string baseName = anchorName.substr(0, anchorName.size() - 6); // remove "_START"
        
        // Extract just the directory name from currentPath (e.g., "ad" from "/Ex/ad/")
        string lastDirName = currentPath;
        if (lastDirName.back() == '/') lastDirName.pop_back();
        size_t lastSlash = lastDirName.find_last_of('/');
        lastDirName = lastDirName.substr(lastSlash + 1);

        anchorName = (baseName == lastDirName) ? baseName + "_START" : baseName + "_END";
    }

    // Search for the anchor name in the file's descriptor table
    const int ENTRY_SIZE = 16;
    streamoff seekPos = descriptorOffset + 8;
    char nameBuffer[9] = {};
    nameBuffer[8] = '\0';

    currentFile.seekg(seekPos, ios::beg);
    currentFile.read(nameBuffer, 8);

    while (std::string(nameBuffer) != anchorName) {
        seekPos += ENTRY_SIZE;
        currentFile.seekg(seekPos, ios::beg);
        currentFile.read(nameBuffer, 8);
    }

    // Insert new descriptors after this anchor
    streamoff insertPoint = seekPos + 8;

    // Backup all data that comes after the insert point
    currentFile.seekg(0, ios::end);
    streamoff fileEnd = currentFile.tellg();
    size_t bytesToMove = static_cast<size_t>(fileEnd - insertPoint);
    vector<char> trailingBytes(bytesToMove);
    currentFile.seekg(insertPoint, ios::beg);
    currentFile.read(trailingBytes.data(), bytesToMove);

    // Reopen file to switch to write mode
    currentFile.clear();
    currentFile.close();
    currentFile.open(wadPath, ios::in | ios::out | ios::binary);
    currentFile.seekp(insertPoint, ios::beg);

    // Write new directory START and END markers
    uint32_t zeroVal = 0;
    string startName = trimmedPath + "_START";
    string endName = trimmedPath + "_END";

    currentFile.write(reinterpret_cast<char*>(&zeroVal), 4);
    currentFile.write(reinterpret_cast<char*>(&zeroVal), 4);
    currentFile.write(startName.c_str(), 8);

    currentFile.write(reinterpret_cast<char*>(&zeroVal), 4);
    currentFile.write(reinterpret_cast<char*>(&zeroVal), 4);
    currentFile.write(endName.c_str(), 8);

    // Reinsert previously backed up data
    if (!trailingBytes.empty())
        currentFile.write(trailingBytes.data(), trailingBytes.size());

    // Add the new directory to the in-memory tree structure
    string normalizedPath = (path.back() == '/') ? path : path + '/';
    Node* newDirNode = new Node(parentNode, normalizedPath);
    parentNode->children.push_back(newDirNode);
    DescriptorObject* newDirDesc = new DescriptorObject(startName, 0, 0, newDirNode);
    fileMap.emplace(normalizedPath, newDirDesc);

    // Update descriptor count in WAD header
    numberOfDescriptors += 2;
    currentFile.seekp(4, ios::beg);
    currentFile.write(reinterpret_cast<char*>(&numberOfDescriptors), 4);
}

void Wad::createFile(const string &path)
{
    // Reject empty paths or those that don't start with '/'
    if (path.empty() || path[0] != '/')
        return;

    // Remove the leading '/' and store the rest of the path
    string fileName = path.substr(1);

    // Remove trailing slash if present (we're creating a file, not a directory)
    if (!fileName.empty() && fileName.back() == '/')
        fileName.pop_back();

    // Traverse down to locate the parent directory of the file
    string parentPath = "/";
    while (fileName.find('/') != string::npos)
    {
        size_t slashPos = fileName.find('/');
        string nextSegment = fileName.substr(0, slashPos + 1);
        string segmentName = nextSegment.substr(0, nextSegment.size() - 1); // exclude trailing slash

        // Cannot create files inside map directories
        if (regex_match(segmentName, regex("E\\dM\\d")))
            return;

        fileName.erase(0, slashPos + 1);         // remove this segment from fileName
        parentPath += nextSegment;

        if (fileMap.find(parentPath) == fileMap.end()) // if directory does not exist
            return;
    }

    // Reject file creation if name is invalid or matches map name pattern
    if (fileName.size() > 8 || regex_match(fileName, regex("E\\dM\\d")))
        return;

    // Locate the parent node and descriptor
    DescriptorObject* parentDescriptor = fileMap[parentPath];
    Node* parentNode = parentDescriptor->position;

    // Determine the anchor descriptor name for where to insert the new file
    string anchorName;
    if (!parentNode->children.empty())
    {
        string lastChildPath = parentNode->children.back()->filePath;
        anchorName = fileMap[lastChildPath]->name;
    }
    else
    {
        anchorName = parentDescriptor->name;
    }

    // Adjust anchor name if it's a _START marker to match original file structure
    if (anchorName.find("_START") != string::npos)
    {
        string baseName = anchorName.substr(0, anchorName.size() - 6);  // strip "_START"
        string parentDirName = parentPath;

        if (parentDirName.back() == '/')
            parentDirName.pop_back();

        // Extract the last directory name
        size_t lastSlash = parentDirName.find_last_of('/');
        string finalSegment;
        if (lastSlash != string::npos)
            finalSegment = parentDirName.substr(lastSlash + 1);
        else
            finalSegment = parentDirName;

        if (baseName == finalSegment)
            anchorName = baseName + "_START";
        else
            anchorName = baseName + "_END";
    }

    // Search descriptor list for anchorName
    const int DESCRIPTOR_SIZE = 16;
    streamoff seekPosition = descriptorOffset + 8;  // names begin at offset+8
    char nameBuffer[9] = {0};

    currentFile.seekg(seekPosition, ios::beg);
    currentFile.read(nameBuffer, 8);

    while (std::string(nameBuffer) != anchorName)
    {
        seekPosition += DESCRIPTOR_SIZE;
        currentFile.seekg(seekPosition, ios::beg);
        currentFile.read(nameBuffer, 8);
    }

    // Move past the anchor name to insert the new file
    seekPosition += 8;

    // Save all remaining descriptors after the anchor
    currentFile.seekg(0, ios::end);
    streamoff fileEnd = currentFile.tellg();
    size_t remainingLength = static_cast<size_t>(fileEnd - seekPosition);
    vector<char> remainingData(remainingLength);
    currentFile.seekg(seekPosition, ios::beg);
    currentFile.read(remainingData.data(), remainingLength);

    // Reopen file in write mode at the insert position
    currentFile.clear();
    currentFile.close();
    currentFile.open(wadPath, ios::in | ios::out | ios::binary);
    currentFile.seekp(seekPosition, ios::beg);

    // Write the new file descriptor (with placeholder length)
    uint32_t offsetZero = 0;
    uint32_t lengthPlaceholder = 0xFFFFFFFF;
    currentFile.write(reinterpret_cast<char*>(&offsetZero), 4);
    currentFile.write(reinterpret_cast<char*>(&lengthPlaceholder), 4);
    currentFile.write(fileName.c_str(), 8);

    // Restore any data that followed the anchor
    if (!remainingData.empty())
        currentFile.write(remainingData.data(), remainingData.size());

    // Build the full path for the file and insert into in-memory tree
    string fullPath = parentPath;
    if (fullPath.back() != '/')
        fullPath += '/';
    fullPath += fileName;

    Node* newFileNode = new Node(parentNode, fullPath);
    parentNode->children.push_back(newFileNode);
    DescriptorObject* newDescriptor = new DescriptorObject(fileName, 0, -1, newFileNode);
    fileMap.emplace(fullPath, newDescriptor);

    // Update descriptor count in the header
    numberOfDescriptors++;
    currentFile.seekp(4, ios::beg);
    currentFile.write(reinterpret_cast<char*>(&numberOfDescriptors), 4);
}

int Wad::writeToFile(const string& path, const char* buffer, int length, int offset)
{
    // Cannot write to a directory
    if (isDirectory(path))
        return -1;

    // Make sure the file exists in the map
    auto fileEntry = fileMap.find(path);
    if (fileEntry == fileMap.end())
        return -1;

    DescriptorObject* fileDescriptor = fileEntry->second;
    const uint32_t PLACEHOLDER = 0xFFFFFFFF;

    // Files that already have real data are immutable
    if (fileDescriptor->length != 0 && fileDescriptor->length != PLACEHOLDER)
        return 0;

    // New data will be written at the start of the current descriptor list
    uint32_t writeOffset = descriptorOffset;
    uint32_t writeLength = length;

    // Read and store the current descriptor table from the file
    const int DESCRIPTOR_SIZE = 16;
    vector<char> descriptorTable(DESCRIPTOR_SIZE * numberOfDescriptors);

    currentFile.seekg(descriptorOffset, ios::beg);
    currentFile.read(descriptorTable.data(), descriptorTable.size());

    // Write the actual content to the location before the descriptor list
    string dataToWrite(buffer, buffer + length);
    currentFile.seekp(writeOffset, ios::beg);

    int maxWritableBytes = std::min<int>(length, dataToWrite.size() - offset);
    currentFile.write(dataToWrite.data() + offset, maxWritableBytes);

    // Seek past the new data to start writing the updated descriptor table
    currentFile.seekp(writeOffset + writeLength, ios::beg);

    bool descriptorPatched = false;

    // Go through the original descriptor table and write it back
    for (size_t i = 0; i < descriptorTable.size(); i += DESCRIPTOR_SIZE)
    {
        uint32_t existingOffset = *reinterpret_cast<uint32_t*>(descriptorTable.data() + i);
        uint32_t existingLength = *reinterpret_cast<uint32_t*>(descriptorTable.data() + i + 4);
        char* namePointer = descriptorTable.data() + i + 8;

        // Update the descriptor that matches the file and has a placeholder length
        if (!descriptorPatched &&
            std::string(namePointer, 8) == fileDescriptor->name &&
            existingLength == PLACEHOLDER)
        {
            existingOffset = writeOffset;
            existingLength = writeLength;
            descriptorPatched = true;
        }

        // Write the descriptor back to the file
        currentFile.write(reinterpret_cast<char*>(&existingOffset), 4);
        currentFile.write(reinterpret_cast<char*>(&existingLength), 4);
        currentFile.write(namePointer, 8);
    }

    // Update in-memory metadata for this file
    fileDescriptor->offset = writeOffset;
    fileDescriptor->length = writeLength;

    // Slide the descriptorOffset forward to account for the new data written
    descriptorOffset += writeLength;

    // Update the descriptor offset in the WAD header
    currentFile.seekp(8, ios::beg);
    currentFile.write(reinterpret_cast<char*>(&descriptorOffset), 4);

    // Make sure everything is saved to disk
    currentFile.flush();

    return maxWritableBytes;
}

