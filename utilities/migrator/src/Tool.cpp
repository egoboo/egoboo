#include "Tool.hpp"

namespace Editor {

using namespace Standard;

Tool::Tool(const std::string& name, std::shared_ptr<FileSystem> fileSystem) :
    name(name), fileSystem(fileSystem)
{}

Tool::~Tool()
{}

const std::string& Tool::getName() const
{
    return name;
}

const std::shared_ptr<FileSystem>& Tool::getFileSystem() const
{
    return fileSystem;
}

} // namespace Editor
