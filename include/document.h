#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <utility>
#include <stdexcept>
#include <regex>
#include <algorithm>
using namespace rapidjson;
using namespace std;

const std::string NOTHING = "__NON_EXIST__";
const std::string EVENT_PAIR = "pairs";
const std::string EVENT_OBJECT_REMOVE = "object:remove";
const std::string EVENT_OBJECT_ADD = "object:add";
const std::string EVENT_ARRAY_REMOVE = "array:remove";
const std::string EVENT_ARRAY_ADD = "array:add";
const std::string EVENT_VALUE_CHANGE = "value_changes";

namespace Linus
{
    namespace jsondiff
    {
        std::string ValueToString(const rapidjson::Value& value);
        std::vector<std::string> KeysFromObject(const rapidjson::Value& value);
        class TreeLevel
        {
            public:
                const rapidjson::Value& left;
                const rapidjson::Value& right;
                std::string left_path;
                std::string right_path;
                std::string up;

                TreeLevel(const rapidjson::Value& left_input, const rapidjson::Value& right_input,\
                 string path_left, string path_right, string up_level);

                std::string get_type();
                std::string to_info();
                std::string get_key();
        };
        class JsonDiffer
        {
            public:
                const double SIMILARITY_THRESHOLD;
                const rapidjson::Value& left;
                const rapidjson::Value& right;
                std::map<std::string, double> cache;
                std::map<std::string, std::vector<std::string>> records;
                bool advanced_mode;
                JsonDiffer(const rapidjson::Value& left_input, const rapidjson::Value& right_input, bool advanced, double similarity_threshold);
                void report(std::string event, Linus::jsondiff::TreeLevel level);
                std::map<std::string, std::vector<std::string>> to_info();
                double compare_array(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_array_fast(Linus::jsondiff::TreeLevel level, bool drill);
                std::map<unsigned int, unsigned int> LCS(Linus::jsondiff::TreeLevel level);
                double compare_array_advanced(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_object(Linus::jsondiff::TreeLevel level, bool drill);
                double compare_primitive(Linus::jsondiff::TreeLevel level, bool drill);
                double _diff_level(Linus::jsondiff::TreeLevel level, bool drill);
                double diff_level(Linus::jsondiff::TreeLevel level, bool drill);
                bool diff();
        };
    }
}