#include "meta.hpp"
#include <cstring>

#include <iostream>

using namespace Csdr::Eti;

MetaWriter::MetaWriter(Serializer *serializer):
    serializer(serializer)
{}

MetaWriter::~MetaWriter() {
    delete serializer;
}

PipelineMetaWriter::PipelineMetaWriter(Serializer *serializer): MetaWriter(serializer) {}

void PipelineMetaWriter::sendMetaData(std::map<std::string, std::string> data) {
    std::string metaString = serializer->serialize(data);
    // can't write...
    if (writer->writeable() < metaString.length()) return;
    std::memcpy(writer->getWritePointer(), metaString.c_str(), metaString.length());
    writer->advance(metaString.length());
}