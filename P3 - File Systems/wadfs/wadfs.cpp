#include <iostream>
#include <string>
#include <unistd.h>
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>
#include "../libWad/Wad.h"

#include <vector>
#include <unistd.h>

using namespace std;

static int getattr_callback(const char *path, struct stat *stbuf) {
    Wad* wadInstance = (Wad*)fuse_get_context()->private_data;

    memset(stbuf, 0, sizeof(struct stat));

    std::string filePath = path;

    //hndle directories
    if (wadInstance->isDirectory(filePath)) {
        stbuf->st_mode = S_IFDIR | 0777; 
        stbuf->st_nlink = 2;             
        stbuf->st_uid = fuse_get_context()->uid; 
        stbuf->st_gid = fuse_get_context()->gid; 
        stbuf->st_size = 4096;           
        return 0; 
    }

    //handle regular files
    auto it = wadInstance->fileMap.find(filePath);
    if (it == wadInstance->fileMap.end()) {
        return -ENOENT; // File or directory not found
    }

    Wad::DescriptorObject* descriptor = it->second;

    stbuf->st_mode = S_IFREG | 0777; 
    stbuf->st_nlink = 1;             
    stbuf->st_uid = fuse_get_context()->uid; 
    stbuf->st_gid = fuse_get_context()->gid; 
    stbuf->st_size = descriptor->length;     
    return 0; 
}


// Creates new files
static int do_mknod(const char *path, mode_t mode, dev_t rdev) {
    Wad* wadInstance = (Wad*)fuse_get_context()->private_data;

    if (mode == 0) {
        mode = 0777; //default full access
    }

    //handle the creation process
    wadInstance->createFile(path);

    //update metadata
    uid_t uid = fuse_get_context()->uid; 
    gid_t gid = fuse_get_context()->gid; 

    //set ownership and permissions
    chmod(path, mode);      
    chown(path, uid, gid);  

    return 0; 
}

//creates new directories
static int do_mkdir(const char *path, mode_t mode) {
    Wad* wadInstance = (Wad*)fuse_get_context()->private_data;

    if (mode == 0) {
        mode = 0777; //default full access
    }

    //handle the directory creation process
    wadInstance->createDirectory(path);

    //update metadata for the directory
    uid_t uid = fuse_get_context()->uid; 
    gid_t gid = fuse_get_context()->gid; 

    //apply ownership and permissions to the directory
    chmod(path, mode);      
    chown(path, uid, gid);  

    return 0; 
}

//handles reading file content
static int read_callback(const char *path, char *buf, size_t size, off_t offset, fuse_file_info *info) {
    Wad* wadInstance = (Wad*)fuse_get_context()->private_data;

    //validate the path and buffer
    if (!path || !buf) {
        return -EINVAL; 
    }

    //get user ID and group ID
    uid_t uid = fuse_get_context()->uid; 
    gid_t gid = fuse_get_context()->gid; 

    //check if the path refers to a valid file
    if (!wadInstance->isContent(path)) {
        return -ENOENT;
    }

    //read file content
    ssize_t bytesRead = wadInstance->getContents(path, buf, size, offset);

    if (bytesRead < 0) {
        return -EIO;
    }

    chmod(path, 0644);      
    chown(path, uid, gid);  
    return bytesRead;
}

//allows writing to files
static int do_write(const char *path, const char *buffer, size_t size, off_t offset, fuse_file_info *info) {
    Wad* wadInstance = (Wad*)fuse_get_context()->private_data;

    //validate path and buffer
    if (!path || !buffer) {
        return -EINVAL; 
    }

    uid_t uid = fuse_get_context()->uid; 
    gid_t gid = fuse_get_context()->gid; 

    //write data to the file
    ssize_t bytesWritten = wadInstance->writeToFile(path, buffer, size, offset);

    if (bytesWritten < 0) {
        return -EIO;
    }

    chmod(path, 0644);      
    chown(path, uid, gid); 

    return bytesWritten;
}


//lists the contents of a directory
static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info *fi) {
    Wad* wadInstance = (Wad*)fuse_get_context()->private_data;

    //validate the path
    if (!path) {
        return -EINVAL;
    }

    //get user and group IDs
    uid_t uid = fuse_get_context()->uid; 
    gid_t gid = fuse_get_context()->gid; 

    //check if the path is a directory
    if (!wadInstance->isDirectory(path)) {
        return -ENOTDIR;
    }

    // get directory contents
    std::vector<std::string> directoryEntries;
    int elements = wadInstance->getDirectory(path, &directoryEntries);

    if (elements < 0) {
        return -EIO;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    //add entries from the Wad object
    for (const std::string &entry : directoryEntries) {
        filler(buf, entry.c_str(), NULL, 0);
    }

    chmod(path, 0755);      
    chown(path, uid, gid);  

    return 0; 
}


static struct fuse_operations operations = {
    .getattr = getattr_callback,
    .mknod = do_mknod,
    .mkdir = do_mkdir,
    .read = read_callback,
    .write = do_write,
    .readdir = readdir_callback,
};

int main(int argc, char* argv[]) {
    if(argc < 3) {
        std::cout << "Not enough arguments." << std::endl;
        exit(EXIT_SUCCESS);
    }

    std::string wadPath = argv[argc - 2];

    // Relative path
    if(wadPath.at(0) != '/') {
        wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
    }

    Wad* myWad = Wad::loadWad(wadPath);

    argv[argc - 2] = argv[argc - 1];
    argc--;

    return fuse_main(argc, argv, &operations, myWad);
}



