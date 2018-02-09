#pragma once

#include "egolib/FileFormats/SpawnFile/SpawnFileToken.hpp"

struct ReadContext;
struct spawn_file_info_t;

class SpawnFileReaderImpl
{
public:
    SpawnFileReaderImpl();
    ~SpawnFileReaderImpl();
    bool read(ReadContext& ctxt, spawn_file_info_t& info);
    std::vector<spawn_file_info_t> read(const std::string& pathname);
};