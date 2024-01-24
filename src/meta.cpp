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
    this->sendString(serializer->serialize(data));
}

void PipelineMetaWriter::sendProgrammes(std::map<uint16_t, std::string> programmes) {
    this->sendString(serializer->serializeProgrammes(programmes));
}

void PipelineMetaWriter::sendString(std::string str) {
    // can't write...
    if (writer->writeable() < str.length()) return;
    std::memcpy(writer->getWritePointer(), str.c_str(), str.length());
    writer->advance(str.length());
}