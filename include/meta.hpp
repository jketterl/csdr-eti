#pragma once

#include <string>
#include <map>
#include <csdr/source.hpp>
#include <variant>

namespace Csdr::Eti {

    using datatype = std::variant<std::string, uint64_t, int64_t, double>;

    class Serializer {
        public:
            virtual ~Serializer() = default;
            virtual std::string serialize(std::map<std::string, datatype> data) = 0;
            virtual std::string serializeProgrammes(std::map<uint16_t, std::string> data) = 0;
    };

    class MetaWriter {
        public:
            explicit MetaWriter(Serializer* serializer);
            virtual ~MetaWriter();
            virtual void sendMetaData(std::map<std::string, datatype> data) = 0;
            virtual void sendProgrammes(std::map<uint16_t, std::string> programmes) = 0;
        protected:
            Serializer* serializer;
    };

    class PipelineMetaWriter: public MetaWriter, public Csdr::Source<unsigned char> {
        public:
            explicit PipelineMetaWriter(Serializer* serializer);
            void sendMetaData(std::map<std::string, datatype> data) override;
            void sendProgrammes(std::map<uint16_t, std::string> programmes) override;
        private:
            void sendString(std::string str);
    };

}