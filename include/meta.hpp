#pragma once

#include <string>
#include <map>
#include <csdr/source.hpp>

namespace Csdr::Eti {

    class Serializer {
        public:
            virtual ~Serializer() = default;
            virtual std::string serialize(std::map<std::string, std::string> data) = 0;
    };

    class MetaWriter {
        public:
            explicit MetaWriter(Serializer* serializer);
            virtual ~MetaWriter();
            virtual void sendMetaData(std::map<std::string, std::string> data) = 0;
        protected:
            Serializer* serializer;
    };

    class PipelineMetaWriter: public MetaWriter, public Csdr::Source<unsigned char> {
        public:
            explicit PipelineMetaWriter(Serializer* serializer);
            void sendMetaData(std::map<std::string, std::string> data) override;
    };

}